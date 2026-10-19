/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 The provider is the random source, but the device (here, the stand-in) cannot deliver bytes. RAND_bytes() must
 report the failure and must never fall back silently to other random bytes.
 */

/**
 *    @file test_provider_read_fail.cpp
 *
 *    @brief A failing device makes RAND_bytes() and RAND_priv_bytes() fail.
 */
#include <vector>

#include <openssl/provider.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

static bool s_configured = setenv("SWRNG_SIM_READ_FAIL", "1", 1) == 0 && pts::use_config(true, true);

TEST(the_provider_still_loads) {
	REQUIRE(s_configured);
	CHECK_EQ(OSSL_PROVIDER_available(nullptr, "swiftrng"), 1);
}

TEST(rand_bytes_fails_when_the_device_fails) {
	std::vector<unsigned char> buffer(32, 0);
	CHECK_EQ(RAND_bytes(buffer.data(), static_cast<int>(buffer.size())), 0);
}

TEST(rand_priv_bytes_fails_when_the_device_fails) {
	std::vector<unsigned char> buffer(32, 0);
	CHECK_EQ(RAND_priv_bytes(buffer.data(), static_cast<int>(buffer.size())), 0);
}

TEST(a_zero_length_request_still_succeeds) {
	unsigned char byte = 0;
	CHECK_EQ(RAND_bytes(&byte, 0), 1);
}
