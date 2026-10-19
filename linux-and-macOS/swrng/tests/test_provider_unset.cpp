/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 The provider is loaded from the config file, but the config does not name it as the random provider. OpenSSL must
 keep using its own random source for RAND_bytes().
 */

/**
 *    @file test_provider_unset.cpp
 *
 *    @brief Loading the provider does not by itself replace OpenSSL's random bytes.
 */
#include <vector>

#include <openssl/provider.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

static bool s_configured = pts::use_config(true, false);

TEST(the_provider_is_loaded) {
	REQUIRE(s_configured);
	CHECK_EQ(OSSL_PROVIDER_available(nullptr, "swiftrng"), 1);
}

TEST(random_bytes_still_come_from_the_default_source) {
	std::vector<unsigned char> got(64, 0);
	std::vector<unsigned char> stream(64, 0);
	swrng_provider::SimStream expected;
	expected.fill(stream.data(), stream.size());
	REQUIRE(RAND_bytes(got.data(), static_cast<int>(got.size())) == 1);
	CHECK(got != stream);
}
