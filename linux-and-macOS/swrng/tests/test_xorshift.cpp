/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the xorshift64 post-processing in SwiftRngApi. It is the xorshift64* algorithm; the expected
 values were generated with an independent Python implementation of it.
 */

/**
 *    @file test_xorshift.cpp
 *
 *    @brief Known answer and buffer handling tests for the xorshift64 post-processing.
 */
#include <set>
#include "private_access.h"
#include "test_framework.h"

using swiftrng::SwiftRngApi;

struct Xorshift64Vector {
	uint64_t raw;
	uint64_t processed;
};

static const Xorshift64Vector c_vectors[] = {
	{0x1212121212121212ULL, 0x2322d6d77d8b7b55ULL},
	{0x0000000000000001ULL, 0x47e4ce4b896cdd1dULL},
	{0xffffffffffffffffULL, 0xf92cc9e5c6000000ULL},
	{0x0123456789abcdefULL, 0x7c9482472cb6708cULL},
	{0xdeadbeefcafebabeULL, 0x7d2957bf9f0a80f4ULL},
	{0x8000000000000000ULL, 0xf1053ead1d000000ULL},
	{0x0000000000000000ULL, 0x0000000000000000ULL},
};

TEST(xorshift64_matches_known_answers) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	for (const Xorshift64Vector &vector : c_vectors) {
		CHECK_EQ((unsigned long long)api.xorshift64_postProcessWord(vector.raw), (unsigned long long)vector.processed);
	}
}

TEST(xorshift64_built_in_self_test_passes) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	CHECK_EQ(api.xorshift64_selfTest(), 0);
}

TEST(xorshift64_buffer_is_processed_word_by_word) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	std::vector<uint64_t> words;
	for (const Xorshift64Vector &vector : c_vectors) {
		words.push_back(vector.raw);
	}
	api.xorshift64_postProcess((uint8_t *)words.data(), (int)(words.size() * sizeof(uint64_t)));
	for (size_t i = 0; i < words.size(); ++i) {
		CHECK_EQ((unsigned long long)words[i], (unsigned long long)c_vectors[i].processed);
	}
}

TEST(xorshift64_leaves_a_trailing_partial_word_untouched) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	// 19 bytes: two complete 8 byte words are processed, the last 3 bytes are not
	uint64_t storage[3] = {c_vectors[0].raw, c_vectors[1].raw, 0x0000000000abcdefULL};
	uint8_t *bytes = (uint8_t *)storage;
	uint8_t tail_before[3] = {bytes[16], bytes[17], bytes[18]};
	api.xorshift64_postProcess(bytes, 19);
	CHECK_EQ((unsigned long long)storage[0], (unsigned long long)c_vectors[0].processed);
	CHECK_EQ((unsigned long long)storage[1], (unsigned long long)c_vectors[1].processed);
	CHECK(bytes[16] == tail_before[0] && bytes[17] == tail_before[1] && bytes[18] == tail_before[2]);
}

TEST(xorshift64_zero_length_changes_nothing) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	uint64_t word = c_vectors[3].raw;
	api.xorshift64_postProcess((uint8_t *)&word, 0);
	api.xorshift64_postProcess((uint8_t *)&word, 7);
	CHECK_EQ((unsigned long long)word, (unsigned long long)c_vectors[3].raw);
}

TEST(xorshift64_gives_different_words_different_results) {
	// The step is a bijection on 64 bit words, so distinct inputs must never collide
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	std::set<uint64_t> seen;
	for (uint64_t i = 1; i <= 50000; ++i) {
		seen.insert(api.xorshift64_postProcessWord(i * UINT64_C(0x9E3779B97F4A7C15)));
	}
	CHECK_EQ(seen.size(), (size_t)50000);
}

TEST(xorshift64_spreads_small_differences_across_the_word) {
	// Neighbouring inputs should not give neighbouring outputs: about half of the 64 bits should differ
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	long total_bits = 0;
	const int samples = 2000;
	for (int i = 1; i <= samples; ++i) {
		uint64_t a = api.xorshift64_postProcessWord((uint64_t)i);
		uint64_t b = api.xorshift64_postProcessWord((uint64_t)i + 1);
		total_bits += __builtin_popcountll(a ^ b);
	}
	double average = (double)total_bits / samples;
	CHECK(average > 28.0 && average < 36.0);
}
