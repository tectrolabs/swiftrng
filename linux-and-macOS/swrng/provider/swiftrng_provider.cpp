/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.
 */

/**
 *    @file swiftrng_provider.cpp
 *
 *    @brief OpenSSL provider that makes RAND_bytes() return random bytes from a SwiftRNG device.
 *
 *    OpenSSL 4.0 removed engines, so the older eng_swiftrng engine cannot be used with it. This provider
 *    replaces it. It implements the one provider function that OpenSSL calls for RAND_bytes() and
 *    RAND_priv_bytes() when a provider is named as the random source (OSSL_FUNC_PROVIDER_RANDOM_BYTES), and no
 *    cryptographic algorithms.
 *
 *    Use it by loading the provider and naming it as the random provider in the OpenSSL config file:
 *
 *        openssl_conf = openssl_init
 *        [openssl_init]
 *        random = random_sect
 *        providers = provider_sect
 *        [provider_sect]
 *        swiftrng = swiftrng_sect
 *        [swiftrng_sect]
 *        module = /path/to/swiftrng.so
 *        activate = 1
 *        [random_sect]
 *        random_provider = swiftrng
 *
 *    Keep 'random' before 'providers' in [openssl_init]. With OpenSSL 4.0.3, listing 'providers' first makes the
 *    provider get unloaded again as soon as random_provider is read, and RAND_bytes() then uses OpenSSL's own source.
 *
 *    If the provider cannot start (for example when no SwiftRNG device is found), OpenSSL falls back to its
 *    built-in random source for RAND_bytes(). Applications that must use the device should check that the
 *    provider is loaded, for example with OSSL_PROVIDER_available().
 */
#include <climits>
#include <cstring>
#include <mutex>
#include <new>

#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/rand.h>

#include "rng_backend.h"

#ifndef OSSL_FUNC_PROVIDER_RANDOM_BYTES
#error "This OpenSSL has no provider random bytes support (OSSL_FUNC_PROVIDER_RANDOM_BYTES). Use OpenSSL 3.5.x with it, or 4.0 and later."
#endif

namespace {

const char c_provider_name[] = "SwiftRNG";
const char c_provider_version[] = "1.0";
const char c_provider_buildinfo[] = "SwiftRNG random provider";

struct ProviderContext {
	swrng_provider::RngBackend *backend {nullptr};
	std::mutex lock; // OpenSSL may ask for bytes from many threads, the device has one stream
};

OSSL_FUNC_provider_teardown_fn swrng_teardown;
OSSL_FUNC_provider_gettable_params_fn swrng_gettable_params;
OSSL_FUNC_provider_get_params_fn swrng_get_params;
OSSL_FUNC_provider_query_operation_fn swrng_query_operation;
OSSL_FUNC_provider_random_bytes_fn swrng_random_bytes;

const OSSL_PARAM c_gettable_params[] = {
	OSSL_PARAM_utf8_ptr(OSSL_PROV_PARAM_NAME, nullptr, 0),
	OSSL_PARAM_utf8_ptr(OSSL_PROV_PARAM_VERSION, nullptr, 0),
	OSSL_PARAM_utf8_ptr(OSSL_PROV_PARAM_BUILDINFO, nullptr, 0),
	OSSL_PARAM_int(OSSL_PROV_PARAM_STATUS, nullptr),
	OSSL_PARAM_END
};

const OSSL_DISPATCH c_dispatch_table[] = {
	{OSSL_FUNC_PROVIDER_TEARDOWN, (void (*)(void))swrng_teardown},
	{OSSL_FUNC_PROVIDER_GETTABLE_PARAMS, (void (*)(void))swrng_gettable_params},
	{OSSL_FUNC_PROVIDER_GET_PARAMS, (void (*)(void))swrng_get_params},
	{OSSL_FUNC_PROVIDER_QUERY_OPERATION, (void (*)(void))swrng_query_operation},
	{OSSL_FUNC_PROVIDER_RANDOM_BYTES, (void (*)(void))swrng_random_bytes},
	{0, nullptr}
};

void swrng_teardown(void *provctx) {
	ProviderContext *ctx = static_cast<ProviderContext *>(provctx);
	if (ctx == nullptr) {
		return;
	}
	delete ctx->backend;
	delete ctx;
}

const OSSL_PARAM *swrng_gettable_params(void *) {
	return c_gettable_params;
}

int swrng_get_params(void *, OSSL_PARAM params[]) {
	OSSL_PARAM *p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_NAME);
	if (p != nullptr && !OSSL_PARAM_set_utf8_ptr(p, c_provider_name)) {
		return 0;
	}
	p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_VERSION);
	if (p != nullptr && !OSSL_PARAM_set_utf8_ptr(p, c_provider_version)) {
		return 0;
	}
	p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_BUILDINFO);
	if (p != nullptr && !OSSL_PARAM_set_utf8_ptr(p, c_provider_buildinfo)) {
		return 0;
	}
	p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_STATUS);
	if (p != nullptr && !OSSL_PARAM_set_int(p, 1)) {
		return 0;
	}
	return 1;
}

// This provider offers no algorithms: it only supplies random bytes
const OSSL_ALGORITHM *swrng_query_operation(void *, int, int *no_cache) {
	*no_cache = 0;
	return nullptr;
}

// Called by RAND_bytes() (which == OSSL_PROV_RANDOM_PUBLIC) and RAND_priv_bytes() (OSSL_PROV_RANDOM_PRIVATE).
// Both are served from the same hardware stream. The device output does not depend on the requested strength.
int swrng_random_bytes(void *provctx, int, void *buf, size_t n, unsigned int) {
	ProviderContext *ctx = static_cast<ProviderContext *>(provctx);
	if (ctx == nullptr || ctx->backend == nullptr || (buf == nullptr && n != 0)) {
		return 0;
	}
	if (n == 0) {
		return 1;
	}
	std::lock_guard<std::mutex> guard(ctx->lock);
	return ctx->backend->read(static_cast<unsigned char *>(buf), n) ? 1 : 0;
}

} // namespace

extern "C" OSSL_provider_init_fn OSSL_provider_init;

extern "C" int OSSL_provider_init(const OSSL_CORE_HANDLE *, const OSSL_DISPATCH *, const OSSL_DISPATCH **out,
		void **provctx) {
	ProviderContext *ctx = new (std::nothrow) ProviderContext;
	if (ctx == nullptr) {
		return 0;
	}
	ctx->backend = swrng_provider::create_backend();
	if (ctx->backend == nullptr || !ctx->backend->open()) {
		swrng_teardown(ctx);
		return 0;
	}
	*provctx = ctx;
	*out = c_dispatch_table;
	return 1;
}
