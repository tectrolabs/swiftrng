/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.
 */

/**
 *    @file rng_backend.h
 *
 *    @brief Where the SwiftRNG OpenSSL provider gets its random bytes from.
 *
 *    The provider logic does not know about the device. It asks a backend for bytes. The real backend
 *    (rng_backend_device.cpp) reads a SwiftRNG device. A stand-in backend (rng_backend_sim.cpp) produces a
 *    known byte stream, so the provider can be tested without hardware.
 */
#ifndef SWIFTRNG_PROVIDER_RNG_BACKEND_H_
#define SWIFTRNG_PROVIDER_RNG_BACKEND_H_

#include <cstddef>

namespace swrng_provider {

class RngBackend {
public:
	virtual ~RngBackend() = default;

	// Prepare the source. Returns false if it cannot be used.
	virtual bool open() = 0;

	// Fill 'buffer' with exactly 'size' random bytes. Returns false on failure.
	virtual bool read(unsigned char *buffer, size_t size) = 0;
};

// Defined by exactly one backend source file. The caller owns the returned object.
RngBackend *create_backend();

} // namespace swrng_provider

#endif /* SWIFTRNG_PROVIDER_RNG_BACKEND_H_ */
