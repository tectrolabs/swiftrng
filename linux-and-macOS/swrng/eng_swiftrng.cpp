/**
 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.

 Contributers:
 	 Quantum Leap Research LLC
 	 TectroLabs L.L.C. https://tectrolabs.com
*/

/**
 *    @file eng_swiftrng.cpp
 *    @date 12/28/2023
 *    @version 1.0
 *
 *    @brief Sets a RAND engine for OPENSSL using SwiftRNG API
 */

#include <openssl/opensslconf.h>
#include <openssl/engine.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

#include <SwiftRngApi.h>
#include <string>
#include <iostream>

static swiftrng::SwiftRngApi api;
static const std::string engine_e_swft_id {"swiftrng"};
static const std::string engine_e_swft_name {"SWiftRNG RAND engine"};

/**
 * Open SwiftRNG device
 *
 * @return 0 - successful operation, otherwise the error code
 */
static int setupRNG() {
	int status = api.open(0);
	if (status != SWRNG_SUCCESS) {
		std::cerr << api.get_last_error_log() << std::endl;
	}
	return status;
}

/**
 * Retrieve random bytes from device
 *
 * @param outputBuffer - pointer to entropy destination
 * @param numGenBytes - number of entropy bytes to retrieve
 *
 * @return 1 - successful operation
 */
static int get_random_bytes(unsigned char *outputBuffer, int numGenBytes) {
	int status = api.get_entropy_ex(outputBuffer, numGenBytes);
	if (status != SWRNG_SUCCESS) {
		std::cerr
				<< "Failed to retrieve entropy from SwiftRNG device, error code: "
				<< status << std::endl;
		return 0;
	}
	return 1;
}

static int random_status(void) {
	return 1;
}

static RAND_METHOD swft_meth = {
		NULL, 				/* seed */
		get_random_bytes,
		NULL, 				/* cleanup */
		NULL,				/* add */
		get_random_bytes,
		random_status
};

static int swft_init(ENGINE *e) {
	const RAND_METHOD *tmp_meth;
	int status;

	// Override the default RAND method compiled into OpenSSL with get_random_bytes()
	tmp_meth = ENGINE_get_RAND(e);
	RAND_set_rand_method(tmp_meth);

	status = setupRNG();
	if (status != SWRNG_SUCCESS) {
		return 0;
	}
	return 1;
}

static int bind(ENGINE *e, const char *id) {
	if (!ENGINE_set_id(e, engine_e_swft_id.c_str())
			|| !ENGINE_set_name(e, engine_e_swft_name.c_str())
			|| !ENGINE_set_flags(e, ENGINE_FLAGS_NO_REGISTER_ALL)
			|| !ENGINE_set_init_function(e, swft_init)
			|| !ENGINE_set_RAND(e, &swft_meth))
		return 0;

	return 1;
}

extern "C" int bind_engine(ENGINE *e, const char *id, const dynamic_fns *fns) {
	if (ENGINE_get_static_state() == fns->static_state)
		goto skip_cbs;
	CRYPTO_set_mem_functions(fns->mem_fns.malloc_fn, fns->mem_fns.realloc_fn,
			fns->mem_fns.free_fn);
	skip_cbs: if (!bind(e, id))
		return 0;

	return 1;
}

extern "C" unsigned long v_check(unsigned long v) {
	if (v >= OSSL_DYNAMIC_OLDEST)
		return OSSL_DYNAMIC_VERSION ;
	return 0;
}

