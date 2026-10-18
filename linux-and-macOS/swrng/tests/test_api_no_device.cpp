/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for SwiftRngApi and its C interface (swrngapi.h) when no device has been opened.

 These tests never open a device, list devices or call anything that reaches the USB bus, so they are safe
 to run with a SwiftRNG plugged in. They check that every operation refuses to run and reports why.
 */

/**
 *    @file test_api_no_device.cpp
 *
 *    @brief Error handling tests for the C++ and C APIs when no device is open.
 */
#include <climits>
#include <cstring>
#include <SwiftRngApi.h>
#include <swrngapi.h>
#include "test_framework.h"

using swiftrng::SwiftRngApi;

// --- SwiftRngApi (C++) ----------------------------------------------------------------------------------

TEST(api_new_object_is_not_open) {
	SwiftRngApi api;
	CHECK_EQ(api.is_open(), 0);
}

TEST(api_entropy_request_fails_when_not_open) {
	SwiftRngApi api;
	unsigned char buffer[32] = {0};
	CHECK_EQ(api.get_entropy(buffer, 32), -ENODEV);
	CHECK_EQ(api.get_entropy_ex(buffer, 32), -ENODEV);
}

TEST(api_entropy_request_with_an_invalid_length_is_refused) {
	SwiftRngApi api;
	unsigned char buffer[32] = {0};
	CHECK_EQ(api.get_entropy(buffer, -1), -EPERM);
	CHECK_EQ(api.get_entropy(buffer, LONG_MAX), -EPERM);
	CHECK_EQ(api.get_entropy_ex(buffer, 0), -EPERM);
	CHECK_EQ(api.get_entropy_ex(buffer, -5), -EPERM);
}

TEST(api_device_operations_fail_when_not_open) {
	SwiftRngApi api;
	NoiseSourceRawData raw;
	FrequencyTables tables;
	DeviceModel model;
	DeviceVersion version;
	DeviceSerialNumber serial;
	double version_number = 0;
	int value = 0;
	CHECK_EQ(api.get_raw_data_block(&raw, 0), -1);
	CHECK_EQ(api.get_frequency_tables(&tables), -1);
	CHECK_EQ(api.get_model(&model), -1);
	CHECK_EQ(api.get_version(&version), -1);
	CHECK_EQ(api.get_version_number(&version_number), -1);
	CHECK_EQ(api.get_serial_number(&serial), -1);
	CHECK_EQ(api.set_power_profile(5), -1);
	CHECK_EQ(api.run_device_diagnostics(), -1);
	CHECK_EQ(api.get_post_processing_status(&value), -1);
	CHECK_EQ(api.get_statistical_tests_status(&value), -1);
	CHECK_EQ(api.get_post_processing_method(&value), -1);
	CHECK_EQ(api.get_embedded_correction_method(&value), -1);
}

TEST(api_configuration_changes_fail_when_not_open) {
	SwiftRngApi api;
	CHECK_EQ(api.disable_post_processing(), -1);
	CHECK_EQ(api.enable_post_processing(0), -1);
	CHECK_EQ(api.disable_statistical_tests(), -1);
	CHECK_EQ(api.enable_statistical_tests(), -1);
}

TEST(api_failed_operation_leaves_an_error_message) {
	SwiftRngApi api;
	DeviceModel model;
	REQUIRE(api.get_model(&model) == -1);
	const char *message = api.get_last_error_message();
	REQUIRE(message != nullptr);
	CHECK(std::strlen(message) > 0);
	CHECK(!api.get_last_error_log().empty());
}

TEST(api_statistics_start_at_zero) {
	SwiftRngApi api;
	api.reset_statistics();
	DeviceStatistics *stats = api.generate_device_statistics();
	REQUIRE(stats != nullptr);
	CHECK_EQ((long long)stats->numGenBytes, 0LL);
	CHECK_EQ((long long)stats->totalRetries, 0LL);
}

TEST(api_close_without_open_does_not_fail_badly) {
	SwiftRngApi api;
	api.close();
	CHECK_EQ(api.is_open(), 0);
}

// --- C interface ----------------------------------------------------------------------------------------

TEST(c_api_rejects_a_null_context) {
	CHECK_EQ(swrngInitializeContext(nullptr), -1);
	CHECK_EQ(swrngDestroyContext(nullptr), -1);
	CHECK_EQ(swrngIsOpen(nullptr), -1);
	CHECK_EQ(swrngClose(nullptr), -1);
}

TEST(c_api_rejects_a_context_that_was_never_initialized) {
	SwrngContext ctxt;
	std::memset(&ctxt, 0, sizeof(ctxt));
	unsigned char buffer[16] = {0};
	CHECK_EQ(swrngIsOpen(&ctxt), -1);
	CHECK_EQ(swrngOpen(&ctxt, 0), -1);
	CHECK_EQ(swrngGetEntropy(&ctxt, buffer, 16), -1);
	CHECK_EQ(swrngDestroyContext(&ctxt), -1);
}

TEST(c_api_context_lifecycle) {
	SwrngContext ctxt;
	REQUIRE(swrngInitializeContext(&ctxt) == 0);
	// swrngIsOpen() returns 0 when the device is open and -1 when it is not
	CHECK_EQ(swrngIsOpen(&ctxt), -1);
	CHECK_EQ(swrngDestroyContext(&ctxt), 0);
	// A destroyed context is no longer valid, so a second destroy is refused
	CHECK_EQ(swrngDestroyContext(&ctxt), -1);
}

TEST(c_api_open_rejects_a_negative_device_number) {
	SwrngContext ctxt;
	REQUIRE(swrngInitializeContext(&ctxt) == 0);
	CHECK_EQ(swrngOpen(&ctxt, -1), -1);
	CHECK_EQ(swrngDestroyContext(&ctxt), 0);
}

TEST(c_api_operations_fail_when_not_open) {
	SwrngContext ctxt;
	REQUIRE(swrngInitializeContext(&ctxt) == 0);
	unsigned char buffer[32] = {0};
	DeviceModel model;
	int value = 0;
	CHECK_EQ(swrngGetEntropy(&ctxt, buffer, 32), -ENODEV);
	CHECK_EQ(swrngGetModel(&ctxt, &model), -1);
	CHECK_EQ(swrngSetPowerProfile(&ctxt, 3), -1);
	CHECK_EQ(swrngRunDeviceDiagnostics(&ctxt), -1);
	CHECK_EQ(swrngGetPostProcessingStatus(&ctxt, &value), -1);
	CHECK_EQ(swrngEnablePostProcessing(&ctxt, 0), -1);
	CHECK_EQ(swrngDisableStatisticalTests(&ctxt), -1);
	CHECK_EQ(swrngDestroyContext(&ctxt), 0);
}

TEST(c_api_failed_operation_leaves_an_error_message) {
	SwrngContext ctxt;
	REQUIRE(swrngInitializeContext(&ctxt) == 0);
	DeviceModel model;
	REQUIRE(swrngGetModel(&ctxt, &model) == -1);
	const char *message = swrngGetLastErrorMessage(&ctxt);
	REQUIRE(message != nullptr);
	CHECK(std::strlen(message) > 0);
	swrngDestroyContext(&ctxt);
}
