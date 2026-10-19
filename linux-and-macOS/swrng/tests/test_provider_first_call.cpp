/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 The very first RAND_bytes() call of a process, made before OpenSSL has loaded its config file, is answered by
 OpenSSL's built-in source: the config is loaded during that call, and the provider named in it takes over only
 afterwards. Every later call is served by the provider. This was seen with OpenSSL 4.0.3. An application that must
 get all its random bytes from the device should load the config first, for example with
 OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL).
 */

/**
 *    @file test_provider_first_call.cpp
 *
 *    @brief Documents that the first RAND_bytes() call of a process may not come from the provider.
 */
#include <vector>

#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

static bool s_configured = pts::use_config(true, true);

// The first call of the process, with nothing loaded yet. It must stay the first RAND_bytes() call in this program.
static bool s_first_call_ok = false;
static std::vector<unsigned char> s_first_bytes(8, 0);

TEST_KNOWN_BUG(the_very_first_random_bytes_come_from_the_provider,
		"OpenSSL 4.0.3 loads its config during the first RAND_bytes() call and answers that call from its own source") {
	REQUIRE(s_configured);
	s_first_call_ok = RAND_bytes(s_first_bytes.data(), static_cast<int>(s_first_bytes.size())) == 1;
	REQUIRE(s_first_call_ok);
	swrng_provider::SimStream stream;
	CHECK_EQ(stream.next_word(), (uint64_t)[&]() {
		uint64_t word = 0;
		for (int b = 7; b >= 0; --b) {
			word = (word << 8) | s_first_bytes[static_cast<size_t>(b)];
		}
		return word;
	}());
}

TEST(later_random_bytes_come_from_the_provider) {
	REQUIRE(s_first_call_ok);
	// Whether or not the first call used a stream word, the provider's stream is now in step with some word 0 to 3
	swrng_provider::SimStream stream;
	std::vector<unsigned char> got(8, 0);
	REQUIRE(RAND_bytes(got.data(), static_cast<int>(got.size())) == 1);
	uint64_t received = 0;
	for (int b = 7; b >= 0; --b) {
		received = (received << 8) | got[static_cast<size_t>(b)];
	}
	int offset = -1;
	for (int i = 0; i < 4; ++i) {
		if (stream.next_word() == received) {
			offset = i;
			break;
		}
	}
	REQUIRE(offset >= 0);

	// From here on every byte must match the stream, call after call
	for (int round = 0; round < 10; ++round) {
		std::vector<unsigned char> next(8, 0);
		REQUIRE(RAND_bytes(next.data(), static_cast<int>(next.size())) == 1);
		uint64_t word = 0;
		for (int b = 7; b >= 0; --b) {
			word = (word << 8) | next[static_cast<size_t>(b)];
		}
		CHECK_EQ(word, stream.next_word());
	}
}
