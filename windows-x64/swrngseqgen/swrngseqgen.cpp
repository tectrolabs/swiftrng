 /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2025 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may only be used in conjunction with TectroLabs devices.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
 * swrngseqgen.cpp
 * Ver. 2.7
 *
 * @brief A program for generating random sequences of unique integer numbers based on true random bytes
 * produced by a SwiftRNG device.
 *
 *
 */

#include <iostream>
#include <string>
#include <cmath>

#include <RandomSeqGenerator.h>

using namespace swiftrng;

// SwiftRNG device number, 0 - first device
static int deviceNumber;

// When generating random sequence, this is the smallest value
static int32_t minNumber;

// When generating random sequence, this is the maximum value
static int32_t maxNumber;

// Only show specified amount of numbers in sequence
static uint32_t numberCount;

// Random Sequence range
static uint32_t range;

// How many times to repeat the sequence generation
static int repeatCount = 1;

static const uint32_t maxSequenceRange = 10000000;
static uint32_t deviceBytesBuffer[maxSequenceRange];
static int64_t  sequenceBuffer[maxSequenceRange];

//
// Function prototypes
//
int generateSequences();
void printRandomSequence(const int64_t *buffer);

/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {

	std::cout << "-------------------------------------------------------------------------------------------------" << std::endl;
	std::cout << "--- A program for generating random sequences of unique integer numbers using SwiftRNG device ---" << std::endl;
	std::cout << "-------------------------------------------------------------------------------------------------" << std::endl;

	if (argc >= 4) {
		deviceNumber = atoi(argv[1]);
		if (deviceNumber < 0 || deviceNumber > 127) {
			std::cerr << "Invalid SwiftRNG device number" << std::endl;
			return -1;
		}

		minNumber = (int32_t)atol(argv[2]);
		if (minNumber < -100000000) {
			std::cerr << "The smallest number in the range cannot be smaller than -100000000" << std::endl;
			return -1;
		}

		maxNumber = (int32_t)atol(argv[3]);
		if (maxNumber > 100000000) {
			std::cerr << "The largest number in the range cannot be bigger than 100000000" << std::endl;
			return -1;
		}

		if (minNumber > maxNumber) {
			std::cerr << "The largest number in the range cannot be smaller than the smallest number" << std::endl;
			return -1;
		}

		range = abs(maxNumber - minNumber) + 1;
		if (range > maxSequenceRange) {
			std::cerr << "The range provided exceeds the " << maxSequenceRange << " numbers in a sequence" << std::endl;
			return -1;
		}
		numberCount = range;

		if (argc > 4) {
			numberCount = (int32_t)atol(argv[4]);
			if (numberCount > range || numberCount == 0) {
				std::cerr << "Invalid sequence limit value" << std::endl;
				return -1;
			}
		}
		if (argc > 5) {
			repeatCount = atoi(argv[5]);
			if (repeatCount < 1) {
				std::cerr << "Invalid repeat count value" << std::endl;
				return -1;
			}
		}


	} else {
		std::cout << "Usage: swrngseqgen <device number> <min number> <max number> [limit] [repeat count]" << std::endl;
		std::cout << "       <device number> - SwiftRNG device NUMBER, use 0 for the first device" << std::endl;
		std::cout << "       <min number> - the smallest number in the range" << std::endl;
		std::cout << "       <max number> - the largest number in the range" << std::endl;
		std::cout << "       [limit] - show this amount of numbers or skip it to include all random numbers in sequence" << std::endl;
		std::cout << "       [repeat count] - how many times to repeat, skip this option to generate just one sequence" << std::endl;
		std::cout << "       Please note that <min number> and <max number> range should not exceed " << maxSequenceRange << " numbers in total" << std::endl;
		std::cout << "Use the following examples to generate sample random sequences:" << std::endl;
		std::cout << "       To generate a sequence of unique random numbers between 1 and 50" << std::endl;
		std::cout << "          swrngseqgen 0 1 50" << std::endl;
		std::cout << "       To generate a sequence of unique random numbers between 1 and 50 and only show 6 numbers" << std::endl;
		std::cout << "          swrngseqgen 0 1 50 6" << std::endl;
		std::cout << "       To generate one number between 1 and 50 and repeat 5 times" << std::endl;
		std::cout << "          swrngseqgen 0 1 50 1 5" << std::endl;
		std::cout << std::endl;
		return 1;
	}

	return generateSequences();
}

/**
 * Generate sequences
 *
 * @return int 0 - successful or error code
 */
int generateSequences() {
	RandomSeqGenerator seqGen(deviceNumber, range);

	while (repeatCount-- > 0) {

		int status = seqGen.generateSequence(deviceBytesBuffer, numberCount);
		if(status) {
			std::cerr << "Failed to generate " << range << " random sequence numbers, error code " << status << std::endl;
			std::cerr << "Error message: " << seqGen.getLastErrorMessage() << std::endl;
			return status;
		}

		// Map sequence numbers to actual numbers
		for (uint32_t i = 0; i < numberCount; i++) {
			sequenceBuffer[i] = deviceBytesBuffer[i] - 1 + minNumber;
		}

		printRandomSequence(sequenceBuffer);
	}
	return 0;
}

/**
 * Print random numbers of the sequence to standard output
 *
 * @param buffer - array of numbers to print
 */
void printRandomSequence(const int64_t *buffer) {
	std::cout << std::endl << "-- Beginning of random sequence --" << std::endl;
	for (uint32_t i = 0; i < numberCount; i++) {
		std::cout << (int)buffer[i] << std::endl;
	}
	std::cout << "-- Ending of random sequence --" << std::endl;
}
