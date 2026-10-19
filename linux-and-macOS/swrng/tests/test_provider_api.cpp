/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Tests of the SwiftRNG OpenSSL provider when an application loads it and names it as the random provider in code
 (OSSL_PROVIDER_load and RAND_set1_random_provider), with no config file naming it.
 */

/**
 *    @file test_provider_api.cpp
 *
 *    @brief Loading the provider and selecting it as the random source through the C API.
 */
#include <cstring>
#include <vector>

#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/provider.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

// An OpenSSL config that does not mention the provider, and the folder OpenSSL should search for modules
static bool s_configured = pts::use_config(false, false) && setenv("OPENSSL_MODULES", pts::provider_dir().c_str(), 1) == 0;

static OSSL_PROVIDER *s_provider = nullptr;

static std::string module_name() {
	std::string file = pts::provider_path();
	std::string name = file.substr(file.find_last_of('/') + 1);
	return name.substr(0, name.find_last_of('.'));
}

TEST(without_the_provider_random_bytes_do_not_come_from_the_stream) {
	REQUIRE(s_configured);
	std::vector<unsigned char> got(64, 0);
	std::vector<unsigned char> stream(64, 0);
	swrng_provider::SimStream expected;
	expected.fill(stream.data(), stream.size());
	REQUIRE(RAND_bytes(got.data(), static_cast<int>(got.size())) == 1);
	CHECK(got != stream);
}

TEST(the_provider_loads_by_module_name) {
	s_provider = OSSL_PROVIDER_load(nullptr, module_name().c_str());
	REQUIRE(s_provider != nullptr);
}

TEST(the_provider_reports_its_name_version_and_status) {
	REQUIRE(s_provider != nullptr);
	const char *name = nullptr;
	const char *version = nullptr;
	int status = 0;
	OSSL_PARAM params[] = {
		OSSL_PARAM_utf8_ptr(OSSL_PROV_PARAM_NAME, &name, 0),
		OSSL_PARAM_utf8_ptr(OSSL_PROV_PARAM_VERSION, &version, 0),
		OSSL_PARAM_int(OSSL_PROV_PARAM_STATUS, &status),
		OSSL_PARAM_END
	};
	REQUIRE(OSSL_PROVIDER_get_params(s_provider, params) == 1);
	REQUIRE(name != nullptr && version != nullptr);
	CHECK_EQ(std::string(name), "SwiftRNG");
	CHECK(std::strlen(version) > 0);
	CHECK_EQ(status, 1);
}

TEST(loading_the_provider_alone_does_not_change_where_random_bytes_come_from) {
	REQUIRE(s_provider != nullptr);
	std::vector<unsigned char> got(64, 0);
	std::vector<unsigned char> stream(64, 0);
	swrng_provider::SimStream expected;
	expected.fill(stream.data(), stream.size());
	REQUIRE(RAND_bytes(got.data(), static_cast<int>(got.size())) == 1);
	CHECK(got != stream);
}

TEST(rand_bytes_come_from_the_provider_after_it_is_selected) {
	REQUIRE(s_provider != nullptr);
	REQUIRE(RAND_set1_random_provider(nullptr, s_provider) == 1);
	swrng_provider::SimStream expected;
	std::vector<unsigned char> got(128, 0);
	std::vector<unsigned char> want(128, 0);
	REQUIRE(RAND_bytes(got.data(), static_cast<int>(got.size())) == 1);
	expected.fill(want.data(), want.size());
	CHECK(got == want);

	// The private variant is served from the same stream, which continues
	REQUIRE(RAND_priv_bytes(got.data(), static_cast<int>(got.size())) == 1);
	expected.fill(want.data(), want.size());
	CHECK(got == want);
}

TEST(the_provider_unloads) {
	REQUIRE(s_provider != nullptr);
	CHECK_EQ(OSSL_PROVIDER_unload(s_provider), 1);
	s_provider = nullptr;
}
