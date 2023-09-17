/*
 * SWRNGRandomSeqGenerator.cpp
 * Ver 2.3
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2023 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements an algorithm for generating unique sequence numbers for a range.
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <RandomSeqGenerator.h>

namespace swiftrng {
/**
 * Allocate memory
 *
 * @param deviceNumber - SwiftRNG device number, 0 - first device
 * @param  uint32_t range - sequence range
 */
RandomSeqGenerator::RandomSeqGenerator(int deviceNumber, uint32_t range) {
	m_device_number = deviceNumber;
	m_range = range;
	m_number_buffer_1 = new (std::nothrow) int32_t[range];
	if (m_number_buffer_1 != nullptr) {
		m_number_buffer_2 = new (std::nothrow) int32_t[range];
		if (m_number_buffer_2 != nullptr) {
			mn_random_buffer = new (std::nothrow) int32_t[range + 1];
			if (mn_random_buffer != nullptr) {
				m_is_memory_allocated = true;
				init(range);
			}
		}
	}
}

/**
 * Open SwiftRNG device
 *
 * @return int - 0 for successful operation
 */
int RandomSeqGenerator::open_device() {
	int status;
	if (!m_is_device_open) {
		status = swrngInitializeContext(&ctxt);
		if (status != SWRNG_SUCCESS) {
			return status;
		}

		// Open SwiftRNG device if available
		status = swrngOpen(&ctxt, m_device_number);
		if (status != SWRNG_SUCCESS) {
			m_error_log_oss << swrngGetLastErrorMessage(&ctxt);
			return status;
		}
		m_is_device_open = true;
		return status;
	}
	return SWRNG_SUCCESS;
}

void RandomSeqGenerator::clear_error_log() {
	m_error_log_oss.str("");
	m_error_log_oss.clear();
}

/**
 * Generate a sequence of random numbers within the range, up to the specified limit `size`
 *
 * @param uint32_t *dest - destination buffer
 * @param uint32_t size - how many numbers to generate within the range
 * @return int - 0 when successfully generated
 */
int RandomSeqGenerator::generateSequence(uint32_t *dest, uint32_t size) {
	int status;
	if (!m_is_memory_allocated) {
		// Unsuccessful object initialization
		return -1;
	}
	clear_error_log();
	status = open_device();
	if (status != SWRNG_SUCCESS) {
		return status;
	}
	init(m_range);
	while(m_current_number_buffer_size > 0 && m_dest_idx < size) {
		status = swrngGetEntropyEx(&ctxt, (uint8_t*)mn_random_buffer, size * 4);
		if (status != 0) {
			m_error_log_oss << swrngGetLastErrorMessage(&ctxt);
			return status;
		}
		iterate(dest, size);
		defragment();
	}
	return 0;
}

/**
 * Mark random numbers for the range up to the specified limit `size`
 *
 * @param uint32_t *dest - destination buffer
 * @param uint32_t size - how many numbers to generate within the range
 */
void RandomSeqGenerator::iterate(uint32_t *dest, uint32_t size) {
	for (uint32_t i = 0; i < size && m_dest_idx < size; i++) {
		uint32_t idx = mn_random_buffer[i] % m_current_number_buffer_size;
		if (m_current_number_buffer[idx] != -1) {
			dest[m_dest_idx++] = m_current_number_buffer[idx];
			m_current_number_buffer[idx] = -1;
		}
	}
}

/**
 * Remove selected numbers (marked as -1) from the buffer
 * and leave only those that were not pulled out yet
 *
 */
void RandomSeqGenerator::defragment() {
	uint32_t newCurNumBufferSize = 0;
	for (uint32_t i = 0; i < m_current_number_buffer_size; i++) {
		if (m_current_number_buffer[i] != -1) {
			m_other_current_number_buffer[newCurNumBufferSize++] = m_current_number_buffer[i];
		}
	}
	m_current_number_buffer_size = newCurNumBufferSize;
	int32_t *swap = m_current_number_buffer;
	m_current_number_buffer = m_other_current_number_buffer;
	m_other_current_number_buffer = swap;
}

/**
 * Initialize the range generator
 *
 * @param uint32_t *range
 */
void RandomSeqGenerator::init(uint32_t range) {
	m_current_number_buffer = m_number_buffer_1;
	m_other_current_number_buffer = m_number_buffer_2;
	for (uint32_t i = 0; i < range; i++) {
		m_current_number_buffer[i] = i + 1;
	}
	m_current_number_buffer_size = range;
	m_dest_idx = 0;
}

/**
 * Free up the heap memory allocated
 */
RandomSeqGenerator::~RandomSeqGenerator() {
	if (m_number_buffer_1 != nullptr) {
		delete [] m_number_buffer_1;
	}
	if (m_number_buffer_2 != nullptr) {
		delete [] m_number_buffer_2;
	}
	if (mn_random_buffer != nullptr) {
		delete [] mn_random_buffer;
	}
	if (m_is_device_open) {
		swrngClose(&ctxt);
	}
	swrngDestroyContext(&ctxt);
}

} /* namespace swiftrng */
