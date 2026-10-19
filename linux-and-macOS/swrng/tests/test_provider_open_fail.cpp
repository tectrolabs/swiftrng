/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 The provider cannot start (here, the stand-in reports that no device was found). This documents what OpenSSL
 does then: the provider is not loaded, and RAND_bytes() quietly uses OpenSSL's own random source.
 Applications that must use the device have to check that the provider is available.
 */

/**
 *    @file test_provider_open_fail.cpp
 *
 *    @brief Behavior when the device cannot be opened at provider start.
 */
#include <vector>

#include <openssl/provider.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

static bool s_configured = setenv("SWRNG_SIM_OPEN_FAIL", "1", 1) == 0 && pts::use_config(true, true);

TEST(the_provider_is_not_available_when_the_device_cannot_be_opened) {
	REQUIRE(s_configured);
	CHECK_EQ(OSSL_PROVIDER_available(nullptr, "swiftrng"), 0);
}

TEST(rand_bytes_falls_back_to_the_default_source) {
	std::vector<unsigned char> got(64, 0);
	std::vector<unsigned char> stream(64, 0);
	swrng_provider::SimStream expected;
	expected.fill(stream.data(), stream.size());
	CHECK_EQ(RAND_bytes(got.data(), static_cast<int>(got.size())), 1);
	CHECK(got != stream);
}
