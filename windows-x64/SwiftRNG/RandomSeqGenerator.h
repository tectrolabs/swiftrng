/*
 * RandomSeqGenerator.h
 * Ver 2.4
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2025 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements an algorithm for generating unique sequence numbers for a range.
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef SWRNGRANDOMSEQGENERATOR_H_
#define SWRNGRANDOMSEQGENERATOR_H_

#include <swrngapi.h>
#include <string>
#include <cstring>
#include <sstream>
#include <cstdint>

namespace swiftrng {

class RandomSeqGenerator {
public:
	RandomSeqGenerator(int deviceNumber, uint32_t range);
	int generateSequence(uint32_t *dest, uint32_t size);
	std::string getLastErrorMessage() const {return m_error_log_oss.str();}
	virtual ~RandomSeqGenerator();

private:
	void clear_error_log();
	void init(uint32_t range);
	void iterate(uint32_t *dest, uint32_t size);
	void defragment();
	int open_device();

private:
	std::ostringstream m_error_log_oss;

	int32_t *m_number_buffer_1;

	int32_t *m_number_buffer_2;

	int32_t *mn_random_buffer;

	int32_t *m_current_number_buffer;

	int32_t *m_other_current_number_buffer;

	uint32_t m_current_number_buffer_size;

	uint32_t m_range;

	SwrngContext ctxt;

	int m_device_number;

	bool m_is_memory_allocated {false};

	bool m_is_device_open {false};

	uint32_t m_dest_idx;

};


} /* namespace swiftrng */

#endif /* SWRNGRANDOMSEQGENERATOR_H_ */

