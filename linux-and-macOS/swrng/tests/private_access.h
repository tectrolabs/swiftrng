/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 SwiftRngApi keeps its post-processing algorithms (SHA-256, SHA-512, xorshift64) and its health tests in
 private methods, because they are internal steps of reading the device. They are the code that handles every
 byte the device produces, so the unit tests call them directly.

 This header gives the tests that access, without changing the library: every system header that
 SwiftRngApi.h includes is included first, and only then is 'private' redefined while SwiftRngApi.h is read.
 */

/**
 *    @file private_access.h
 *
 *    @brief Includes SwiftRngApi.h with its private members accessible, for unit tests only.
 */
#ifndef SWIFTRNG_TESTS_PRIVATE_ACCESS_H_
#define SWIFTRNG_TESTS_PRIVATE_ACCESS_H_

// Everything SwiftRngApi.h includes, so none of it is read with 'private' redefined
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <ApiStructs.h>
#if defined __FreeBSD__
	#include <libusb.h>
#else
	#include <libusb-1.0/libusb.h>
#endif
#include <USBSerialDevice.h>

#define private public
#include <SwiftRngApi.h>
#undef private

#endif /* SWIFTRNG_TESTS_PRIVATE_ACCESS_H_ */
