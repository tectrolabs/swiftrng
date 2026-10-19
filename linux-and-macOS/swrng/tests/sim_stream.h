/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

/**
 *    @file sim_stream.h
 *
 *    @brief A repeatable byte stream, used in place of a SwiftRNG device when testing the provider.
 *
 *    The stream is xorshift64* run from a fixed seed. Its bytes are the little-endian bytes of each 64-bit
 *    word in turn, so any caller can recompute the exact bytes the stand-in backend hands out.
 */
#ifndef SWIFTRNG_PROVIDER_SIM_STREAM_H_
#define SWIFTRNG_PROVIDER_SIM_STREAM_H_

#include <cstddef>
#include <cstdint>

namespace swrng_provider {

class SimStream {
public:
	static const uint64_t c_seed = UINT64_C(0x5357524e470001);

	SimStream() : m_state(c_seed) {}

	// The next 64-bit word of the stream
	uint64_t next_word() {
		m_state ^= m_state >> 12;
		m_state ^= m_state << 25;
		m_state ^= m_state >> 27;
		return m_state * UINT64_C(2685821657736338717);
	}

	// Fill 'buffer' with the next 'size' bytes of the stream (little-endian, continuing across calls)
	void fill(unsigned char *buffer, size_t size) {
		for (size_t i = 0; i < size; ++i) {
			if (m_available == 0) {
				m_word = next_word();
				m_available = 8;
			}
			buffer[i] = static_cast<unsigned char>(m_word & 0xff);
			m_word >>= 8;
			--m_available;
		}
	}

private:
	uint64_t m_state;
	uint64_t m_word {0};
	int m_available {0};
};

} // namespace swrng_provider

#endif /* SWIFTRNG_PROVIDER_SIM_STREAM_H_ */
