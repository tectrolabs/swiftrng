/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the 'Repetition Count' (RCT) and 'Adaptive Proportion' (APT) health tests in SwiftRngApi.
 They inspect the block of bytes read from the device (m_buff_rnd_in, 16000 bytes). The status byte of a test
 is set after more than m_num_failures_threshold (4) failures: 1 for RCT and 2 for APT.
 */

/**
 *    @file test_health.cpp
 *
 *    @brief Detection and non-detection tests for the SwiftRNG health tests.
 */
#include "private_access.h"
#include "test_framework.h"

using swiftrng::SwiftRngApi;

static const int c_block_bytes = 16000;

// Fixed, repeatable well mixed bytes (xorshift64*)
static void fill_pseudo_random(char *dest, int size, uint64_t seed) {
	uint64_t x = seed;
	for (int i = 0; i < size; ++i) {
		x ^= x >> 12;
		x ^= x << 25;
		x ^= x >> 27;
		dest[i] = (char)((x * UINT64_C(2685821657736338717)) >> 56);
	}
}

// open() initialises the health test state before the first block; the tests do the same
static void init_health(SwiftRngApi &api) {
	api.rct_initialize();
	api.apt_initialize();
}

// Start a block as the device read does: reset the per-block state, load the bytes, run the tests
static void test_block(SwiftRngApi &api) {
	api.rct_restart();
	api.apt_restart();
	api.test_samples();
}

TEST(health_new_api_reports_no_failures) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	init_health(api);
	CHECK_EQ((int)api.m_rct.statusByte, 0);
	CHECK_EQ((int)api.m_apt.statusByte, 0);
	uint16_t rct = 99;
	uint16_t apt = 99;
	CHECK_EQ(api.get_max_rct_failures_per_block(&rct), 0);
	CHECK_EQ(api.get_max_apt_failures_per_block(&apt), 0);
	CHECK_EQ((int)rct, 0);
	CHECK_EQ((int)apt, 0);
}

TEST(health_random_blocks_pass) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	init_health(api);
	for (int block = 0; block < 20; ++block) {
		fill_pseudo_random(api.m_buff_rnd_in, c_block_bytes, 0x1234567ULL + (uint64_t)block);
		test_block(api);
	}
	CHECK_EQ((int)api.m_rct.statusByte, 0);
	CHECK_EQ((int)api.m_apt.statusByte, 0);
}

TEST(health_stuck_block_fails_the_repetition_count_test) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	init_health(api);
	std::memset(api.m_buff_rnd_in, 0x5a, c_block_bytes);
	test_block(api);
	CHECK_EQ((int)api.m_rct.statusByte, 1);
	uint16_t rct = 0;
	REQUIRE(api.get_max_rct_failures_per_block(&rct) == 0);
	CHECK(rct > 4);
}

TEST(health_runs_shorter_than_the_cutoff_pass) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	init_health(api);
	// Runs of 4 equal bytes stay below the repetition cutoff of 5
	for (int i = 0; i < c_block_bytes; ++i) {
		api.m_buff_rnd_in[i] = (char)((i / 4) % 251);
	}
	test_block(api);
	CHECK_EQ((int)api.m_rct.statusByte, 0);
	uint16_t rct = 99;
	REQUIRE(api.get_max_rct_failures_per_block(&rct) == 0);
	CHECK_EQ((int)rct, 0);
}

TEST(health_overrepresented_value_fails_the_adaptive_proportion_test) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	init_health(api);
	// Alternating values never repeat back to back, but the first value of each 64 sample window
	// recurs about 32 times, far above the cutoff of 5
	for (int i = 0; i < c_block_bytes; ++i) {
		api.m_buff_rnd_in[i] = (i % 2 == 0) ? (char)0xA5 : (char)0x3C;
	}
	test_block(api);
	CHECK_EQ((int)api.m_apt.statusByte, 2);
	CHECK_EQ((int)api.m_rct.statusByte, 0);
	uint16_t apt = 0;
	REQUIRE(api.get_max_apt_failures_per_block(&apt) == 0);
	CHECK(apt > 4);
}

TEST(health_a_failing_block_is_remembered_by_later_good_blocks) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	init_health(api);
	std::memset(api.m_buff_rnd_in, 0x00, c_block_bytes);
	test_block(api);
	REQUIRE(api.m_rct.statusByte == 1);
	fill_pseudo_random(api.m_buff_rnd_in, c_block_bytes, 777);
	test_block(api);
	CHECK_EQ((int)api.m_rct.statusByte, 1);
}

TEST(health_rct_and_apt_start_with_the_documented_settings) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	init_health(api);
	CHECK_EQ((int)api.m_rct.maxRepetitions, 5);
	CHECK_EQ((int)api.m_rct.signature, 1);
	CHECK_EQ((int)api.m_apt.windowSize, 64);
	CHECK_EQ((int)api.m_apt.cutoffValue, 5);
	CHECK_EQ((int)api.m_apt.signature, 2);
	CHECK_EQ((int)api.m_num_failures_threshold, 4);
}
