/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Helpers for the OpenSSL provider tests. The tests load a stand-in build of the provider (the provider code with a
 repeatable byte stream in place of the device), so they need no hardware. OpenSSL reads its configuration only
 once, when it is first used, so each test program sets up its configuration before anything else and every
 configuration needs its own program.
 */

/**
 *    @file provider_test_support.h
 *
 *    @brief Writes OpenSSL config files that load the stand-in provider, and points OpenSSL at them.
 */
#ifndef SWIFTRNG_TESTS_PROVIDER_TEST_SUPPORT_H_
#define SWIFTRNG_TESTS_PROVIDER_TEST_SUPPORT_H_

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>

#include "sim_stream.h"

#ifndef SWRNG_SIM_PROVIDER_PATH
#error "SWRNG_SIM_PROVIDER_PATH must be defined to the path of the stand-in provider module"
#endif

namespace pts {

inline std::string &temp_dir() {
	static std::string dir;
	return dir;
}

inline std::string &config_path() {
	static std::string path;
	return path;
}

inline void cleanup() {
	if (!config_path().empty()) {
		std::remove(config_path().c_str());
	}
	if (!temp_dir().empty()) {
		rmdir(temp_dir().c_str());
	}
}

inline std::string provider_path() {
	return SWRNG_SIM_PROVIDER_PATH;
}

inline std::string provider_dir() {
	std::string path = provider_path();
	return path.substr(0, path.find_last_of('/'));
}

// Writes the config file and points OPENSSL_CONF at it. Call before any OpenSSL function.
//   load_provider        load the stand-in provider as "swiftrng"
//   name_random_provider name it as the random provider (random_provider = swiftrng)
inline bool use_config(bool load_provider, bool name_random_provider) {
	char name[] = "/tmp/swrng_provider_test_XXXXXX";
	if (mkdtemp(name) == nullptr) {
		return false;
	}
	temp_dir() = name;
	config_path() = temp_dir() + "/openssl.cnf";

	std::ofstream cnf(config_path());
	// 'random' must come before 'providers': with OpenSSL 4.0.3 a provider that is already loaded when
	// random_provider is read gets unloaded again, so the name has to be recorded first.
	cnf << "openssl_conf = openssl_init\n"
		<< "[openssl_init]\n";
	if (name_random_provider) {
		cnf << "random = random_sect\n";
	}
	if (load_provider) {
		cnf << "providers = provider_sect\n";
	}
	cnf << "[provider_sect]\n"
		<< "default = default_sect\n"
		<< "swiftrng = swiftrng_sect\n"
		<< "[default_sect]\n"
		<< "activate = 1\n"
		<< "[swiftrng_sect]\n"
		<< "module = " << provider_path() << "\n"
		<< "activate = 1\n"
		<< "[random_sect]\n"
		<< "random_provider = swiftrng\n";
	cnf.close();

	std::atexit(cleanup);
	return setenv("OPENSSL_CONF", config_path().c_str(), 1) == 0;
}

} // namespace pts

#endif /* SWIFTRNG_TESTS_PROVIDER_TEST_SUPPORT_H_ */
