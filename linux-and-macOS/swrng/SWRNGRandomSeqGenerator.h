/*
 * SWRNGRandomSeqGenerator.h
 * Ver 2.1
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class implements an algorithm for generating unique sequence numbers for a range.

 This class may only be used in conjunction with TectroLabs devices.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef SWRNGRANDOMSEQGENERATOR_H_
#define SWRNGRANDOMSEQGENERATOR_H_

#include <stdint.h>
#include "swrngapi.h"

class SWRNGRandomSeqGenerator {
public:
	SWRNGRandomSeqGenerator(int deviceNumber, uint32_t range);
	int generateSequence(uint32_t *dest, uint32_t size);
	char* getLastErrorMessage();
	virtual ~SWRNGRandomSeqGenerator();

private:
	int32_t *numberBuffer1;
	int32_t *numberBuffer2;
	int32_t *rndBuffer;
	int32_t *currentNumberBuffer;
	int32_t *otherCurrentNumberBuffer;
	uint32_t currentNumberBufferSize;
	uint32_t range;
	SwrngContext ctxt;
	int		deviceNumber;
	bool	isMemoryAllocated;
	bool	isDeviceOpen;
	uint32_t destIdx;
	char lastErrorMessage[512];

private:
	void init(uint32_t range);
	void iterate(uint32_t *dest, uint32_t size);
	void defragment();
	int openDevice();
};

#endif /* SWRNGRANDOMSEQGENERATOR_H_ */
