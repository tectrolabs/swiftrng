/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the cluster API (swrng-cl-api.h) when no cluster has been opened.

 swrngCLOpen() searches for devices and starts download threads for any valid cluster size, so these tests
 never call it with a valid size. They only use it with sizes that are rejected before any device is touched.
 */

/**
 *    @file test_cl_api.cpp
 *
 *    @brief Context and error handling tests for the SwiftRNG cluster API.
 */
#include <cerrno>
#include <cstring>
#include <swrng-cl-api.h>
#include "test_framework.h"

TEST(cluster_rejects_a_null_context) {
	CHECK_EQ(swrngInitializeCLContext(nullptr), -1);
}

TEST(cluster_new_context_is_closed_and_empty) {
	SwrngCLContext ctxt;
	REQUIRE(swrngInitializeCLContext(&ctxt) == 0);
	CHECK_EQ(swrngIsCLOpen(&ctxt), 0);
	CHECK_EQ(swrngGetCLSize(&ctxt), 0);
	CHECK_EQ(swrngGetCLFailoverEventCount(&ctxt), 0L);
	CHECK_EQ(swrngGetCLResizeAttemptCount(&ctxt), 0L);
}

TEST(cluster_context_that_was_never_initialized_reports_closed) {
	SwrngCLContext ctxt;
	std::memset(&ctxt, 0, sizeof(ctxt));
	CHECK_EQ(swrngIsCLOpen(&ctxt), 0);
	CHECK_EQ(swrngGetCLSize(&ctxt), 0);
	const char *message = swrngGetCLLastErrorMessage(&ctxt);
	REQUIRE(message != nullptr);
	CHECK(std::strlen(message) > 0);
}

TEST(cluster_entropy_request_fails_when_not_open) {
	SwrngCLContext ctxt;
	REQUIRE(swrngInitializeCLContext(&ctxt) == 0);
	unsigned char buffer[32] = {0};
	CHECK_EQ(swrngGetCLEntropy(&ctxt, buffer, 32), -ENODEV);
	const char *message = swrngGetCLLastErrorMessage(&ctxt);
	REQUIRE(message != nullptr);
	CHECK(std::strlen(message) > 0);
}

TEST(cluster_close_fails_when_not_open) {
	SwrngCLContext ctxt;
	REQUIRE(swrngInitializeCLContext(&ctxt) == 0);
	CHECK_EQ(swrngCLClose(&ctxt), -1);
}

TEST(cluster_open_rejects_invalid_sizes) {
	SwrngCLContext ctxt;
	REQUIRE(swrngInitializeCLContext(&ctxt) == 0);
	CHECK_EQ(swrngCLOpen(&ctxt, 0), -1);
	CHECK_EQ(swrngCLOpen(&ctxt, -1), -1);
	CHECK_EQ(swrngCLOpen(&ctxt, 11), -1);
	CHECK_EQ(swrngCLOpen(&ctxt, 1000), -1);
	CHECK_EQ(swrngIsCLOpen(&ctxt), 0);
	const char *message = swrngGetCLLastErrorMessage(&ctxt);
	REQUIRE(message != nullptr);
	CHECK(std::strlen(message) > 0);
}
