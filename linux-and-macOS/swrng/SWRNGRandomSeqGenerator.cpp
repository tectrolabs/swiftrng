#include "stdafx.h"

/*
 * SWRNGRandomSeqGenerator.cpp
 * Ver 2.1
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class implements an algorithm for generating unique sequence numbers for a range.

 This class may only be used in conjunction with TectroLabs devices.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "SWRNGRandomSeqGenerator.h"

/**
 * Allocate memory
 *
 * @param deviceNumber - SwiftRNG device number, 0 - first device
 * @param  uint32_t range - sequence range
 */
SWRNGRandomSeqGenerator::SWRNGRandomSeqGenerator(int deviceNumber, uint32_t range) {
	isDeviceOpen = false;
	this->deviceNumber = deviceNumber;
	lastErrorMessage[0] = '\0';
	this->range = range;
	numberBuffer1 = new int32_t[range];
	if (numberBuffer1 != 0) {
		numberBuffer2 = new int32_t[range];
		if (numberBuffer2 != 0) {
			rndBuffer = new int32_t[range + 1];
			if (rndBuffer != 0) {
				isMemoryAllocated = true;
			}
		}
	}
	init(range);
}

/**
 * Open SwiftRNG device
 *
 * @return int - 0 when device successfully open
 */
int SWRNGRandomSeqGenerator::openDevice() {
	int status;
	if (!isDeviceOpen) {
		status = swrngInitializeContext(&ctxt);
		if (status != SWRNG_SUCCESS) {
		#ifdef _WIN32
			strcpy_s(lastErrorMessage, swrngGetLastErrorMessage(&ctxt));
		#else
			strcpy(lastErrorMessage, swrngGetLastErrorMessage(&ctxt));
		#endif
			return(status);
		}

		// Open SwiftRNG device if available
		status = swrngOpen(&ctxt, deviceNumber);
		if (status != SWRNG_SUCCESS) {
		#ifdef _WIN32
			strcpy_s(lastErrorMessage, swrngGetLastErrorMessage(&ctxt));
		#else
			strcpy(lastErrorMessage, swrngGetLastErrorMessage(&ctxt));
		#endif
			return(status);
		}
		isDeviceOpen = true;
		return status;
	}
	return SWRNG_SUCCESS;
}

/**
 * Generate a sequence of random numbers within the range up to the specified limit (size)
 *
 * @param uint32_t *dest - destination buffer
 * @param uint32_t size - how many numbers to generate within the range
 * @return int - 0 when successfully generated
 */
int SWRNGRandomSeqGenerator::generateSequence(uint32_t *dest, uint32_t size) {
	int status;
	if (!isMemoryAllocated) {
		// Unsuccessful initialization
		return -1;
	}
	status = openDevice();
	if (status != SWRNG_SUCCESS) {
		return status;
	}
	init(range);
	while(currentNumberBufferSize > 0 && destIdx < size) {
		status = swrngGetEntropyEx(&ctxt, (uint8_t*)rndBuffer, size * 4);
		if (status != 0) {
		#ifdef _WIN32
			strcpy_s(lastErrorMessage, swrngGetLastErrorMessage(&ctxt));
		#else
			strcpy(lastErrorMessage, swrngGetLastErrorMessage(&ctxt));
		#endif
			return status;
		}
		iterate(dest, size);
		defragment();
	}
	return 0;
}

/**
 * Mark random numbers for the range up to the specified limit (size)
 *
 * @param uint32_t *dest - destination buffer
 * @param uint32_t size - how many numbers to generate within the range
 */
void SWRNGRandomSeqGenerator::iterate(uint32_t *dest, uint32_t size) {
	for (uint32_t i = 0; i < size && destIdx < size; i++) {
		uint32_t idx = rndBuffer[i] % currentNumberBufferSize;
		if (currentNumberBuffer[idx] != -1) {
			dest[destIdx++] = currentNumberBuffer[idx];
			currentNumberBuffer[idx] = -1;
		}
	}
}

/**
 * Remove selected numbers (marked as -1) from the buffer
 * and leave only those that were not pulled out yet
 *
 */
void SWRNGRandomSeqGenerator::defragment() {
	uint32_t newCurNumBufferSize = 0;
	for (uint32_t i = 0; i < currentNumberBufferSize; i++) {
		if (currentNumberBuffer[i] != -1) {
			otherCurrentNumberBuffer[newCurNumBufferSize++] = currentNumberBuffer[i];
		}
	}
	currentNumberBufferSize = newCurNumBufferSize;
	int32_t *swap = currentNumberBuffer;
	currentNumberBuffer = otherCurrentNumberBuffer;
	otherCurrentNumberBuffer = swap;
}

/**
 * Initialize the generator
 *
 * @param uint32_t *range
 */
void SWRNGRandomSeqGenerator::init(uint32_t range) {
	currentNumberBuffer = numberBuffer1;
	otherCurrentNumberBuffer = numberBuffer2;
	for (uint32_t i = 0; i < range; i++) {
		currentNumberBuffer[i] = i + 1;
	}
	currentNumberBufferSize = range;
	destIdx = 0;
}

/**
 * Retrieve last error message
 *
 * @return char* - pointer to the last error message
 */
char* SWRNGRandomSeqGenerator::getLastErrorMessage() {
	return lastErrorMessage;
}

/**
 * Free up the heap memory allocated for buffers
 */
SWRNGRandomSeqGenerator::~SWRNGRandomSeqGenerator() {
	if (numberBuffer1 != 0) {
		delete [] numberBuffer1;
		numberBuffer1 = 0;
	}
	if (numberBuffer2 != 0) {
		delete [] numberBuffer2;
		numberBuffer2 = 0;
	}
	if (rndBuffer != 0) {
		delete [] rndBuffer;
		rndBuffer = 0;
	}
	isMemoryAllocated = false;
	if (isDeviceOpen) {
		swrngClose(&ctxt);
		isDeviceOpen = false;
	}
}
