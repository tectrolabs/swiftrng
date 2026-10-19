/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Tests of the SwiftRNG OpenSSL provider when it is named as the random provider in the OpenSSL config file.
 The stand-in provider hands out a known byte stream, so every byte RAND_bytes() returns can be checked.
 */

/**
 *    @file test_provider_config.cpp
 *
 *    @brief RAND_bytes() and RAND_priv_bytes() are served by the provider named in the config file.
 */
#include <vector>

#include <openssl/provider.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

// Runs before main(), so before OpenSSL reads its configuration
static bool s_configured = pts::use_config(true, true);

// The stream the provider should be handing out. It advances in step with the provider's own.
static swrng_provider::SimStream s_expected;

static bool next_bytes_match(size_t n, bool private_bytes) {
	std::vector<unsigned char> got(n, 0);
	std::vector<unsigned char> want(n, 0);
	int ok = private_bytes ? RAND_priv_bytes(got.data(), static_cast<int>(n)) : RAND_bytes(got.data(), static_cast<int>(n));
	s_expected.fill(want.data(), n);
	return ok == 1 && got == want;
}

TEST(provider_config_was_written) {
	CHECK(s_configured);
}

TEST(provider_is_loaded_from_the_config) {
	CHECK_EQ(OSSL_PROVIDER_available(nullptr, "swiftrng"), 1);
}

TEST(rand_bytes_come_from_the_provider) {
	CHECK(next_bytes_match(64, false));
}

TEST(rand_priv_bytes_come_from_the_provider) {
	CHECK(next_bytes_match(64, true));
}

TEST(rand_bytes_ex_with_a_strength_comes_from_the_provider) {
	std::vector<unsigned char> got(48, 0);
	std::vector<unsigned char> want(48, 0);
	REQUIRE(RAND_bytes_ex(nullptr, got.data(), got.size(), 256) == 1);
	s_expected.fill(want.data(), want.size());
	CHECK(got == want);
}

TEST(consecutive_requests_continue_the_stream) {
	for (int i = 0; i < 20; ++i) {
		CHECK(next_bytes_match(100, i % 2 == 1));
	}
}

TEST(odd_sized_requests_are_served_exactly) {
	const size_t sizes[] = {1, 3, 7, 9, 15, 17, 255, 257};
	for (size_t n : sizes) {
		CHECK(next_bytes_match(n, false));
	}
}

TEST(a_zero_length_request_succeeds_and_uses_no_bytes) {
	unsigned char byte = 0;
	CHECK_EQ(RAND_bytes(&byte, 0), 1);
	CHECK(next_bytes_match(16, false));
}

TEST(a_large_request_is_served_in_full) {
	CHECK(next_bytes_match(1000000, false));
}
