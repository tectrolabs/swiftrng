/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Many threads call RAND_bytes() at once while the provider is the random source. The device has a single byte
 stream, so the provider must hand each byte out once and never mix two requests.
 */

/**
 *    @file test_provider_threads.cpp
 *
 *    @brief Concurrent use of the provider: every word of the stream is delivered exactly once.
 */
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <thread>
#include <unordered_map>
#include <vector>

#include <openssl/crypto.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

static bool s_configured = pts::use_config(true, true);

TEST(concurrent_requests_receive_each_word_of_the_stream_exactly_once) {
	REQUIRE(s_configured);
	// Load the OpenSSL config explicitly before the first RAND_bytes() call. Otherwise OpenSSL loads it during
	// that first call, which it answers from its own source (see test_provider_first_call.cpp).
	REQUIRE(OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr) == 1);
	const int threads = 8;
	const int requests = 200;
	const size_t request_bytes = 800; // a multiple of 8, so every request holds whole stream words
	const size_t words_per_request = request_bytes / 8;

	std::vector<std::vector<uint64_t>> received(threads);
	std::atomic<int> failures{0};
	std::vector<std::thread> workers;
	for (int t = 0; t < threads; ++t) {
		workers.emplace_back([&, t]() {
			std::vector<unsigned char> buffer(request_bytes);
			for (int r = 0; r < requests; ++r) {
				if (RAND_bytes(buffer.data(), static_cast<int>(buffer.size())) != 1) {
					failures++;
					return;
				}
				for (size_t w = 0; w < words_per_request; ++w) {
					uint64_t word = 0;
					for (int b = 7; b >= 0; --b) {
						word = (word << 8) | buffer[w * 8 + static_cast<size_t>(b)];
					}
					received[static_cast<size_t>(t)].push_back(word);
				}
			}
		});
	}
	for (std::thread &worker : workers) {
		worker.join();
	}
	CHECK_EQ(failures.load(), 0);

	std::vector<uint64_t> all;
	for (const auto &list : received) {
		all.insert(all.end(), list.begin(), list.end());
	}
	const size_t total_words = static_cast<size_t>(threads) * requests * words_per_request;
	REQUIRE(all.size() == total_words);
	std::sort(all.begin(), all.end());

	// The first total_words words of the stream, sorted, must be exactly what the threads received
	swrng_provider::SimStream stream;
	std::vector<uint64_t> expected;
	for (size_t i = 0; i < total_words; ++i) {
		expected.push_back(stream.next_word());
	}
	std::sort(expected.begin(), expected.end());
	bool same = all == expected;
	if (!same) {
		// Say what is wrong: repeated words (two requests got the same bytes), words that are not in the stream
		// at all (bytes mixed together), or stream words that nobody received (consumed elsewhere)
		swrng_provider::SimStream wider;
		std::unordered_map<uint64_t, size_t> index_of;
		for (size_t i = 0; i < total_words * 2; ++i) {
			index_of[wider.next_word()] = i;
		}
		size_t repeated = 0, foreign = 0, highest = 0;
		for (size_t i = 0; i < all.size(); ++i) {
			if (i > 0 && all[i] == all[i - 1]) {
				++repeated;
			}
			auto found = index_of.find(all[i]);
			if (found == index_of.end()) {
				++foreign;
			} else if (found->second > highest) {
				highest = found->second;
			}
		}
		std::fprintf(stderr, "    %zu words received, %zu repeated, %zu not from the stream, highest stream index %zu (expected below %zu)\n",
				all.size(), repeated, foreign, highest, total_words);
	}
	CHECK(same);
}
