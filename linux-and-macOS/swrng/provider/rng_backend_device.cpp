/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.
 */

/**
 *    @file rng_backend_device.cpp
 *
 *    @brief Provider backend that reads random bytes from the first connected SwiftRNG device.
 */
#include <climits>
#include <iostream>

#include <SwiftRngApi.h>

#include "rng_backend.h"

namespace swrng_provider {

namespace {

class DeviceBackend : public RngBackend {
public:
	bool open() override {
		int status = m_api.open(0);
		if (status != SWRNG_SUCCESS) {
			std::cerr << "SwiftRNG provider: " << m_api.get_last_error_log() << std::endl;
			return false;
		}
		return true;
	}

	bool read(unsigned char *buffer, size_t size) override {
		if (size > static_cast<size_t>(LONG_MAX)) {
			return false;
		}
		int status = m_api.get_entropy_ex(buffer, static_cast<long>(size));
		if (status != SWRNG_SUCCESS) {
			std::cerr << "SwiftRNG provider: failed to read from the device, error code " << status << std::endl;
			return false;
		}
		return true;
	}

private:
	swiftrng::SwiftRngApi m_api;
};

} // namespace

RngBackend *create_backend() {
	return new (std::nothrow) DeviceBackend;
}

} // namespace swrng_provider
