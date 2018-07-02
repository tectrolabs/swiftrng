#include "stdafx.h"
/*
 * swrngseqgen.cpp
 * Ver. 1.2
 *
 * A program for generating random sequences of unique integer numbers based
 * on true random bytes produced by a SwiftRNG device.
 *
 */

#include <iostream>
#include <string>
#include "SWRNGRandomSeqGenerator.h"

int deviceNumber;			// SwiftRNG device number, 0 - first device
int32_t minNumber;			// When generating random sequence, this is the smallest value
int32_t maxNumber;			// When generating random sequence, this is the maximum value
uint32_t numberCount;		// Only show specified amount of numbers in sequence
uint32_t range;				// Random Sequence range
int repeatCount = 1;		// How many times to repeat the sequence generation
static const uint32_t maxSequenceRange = 10000000;
uint32_t deviceBytesBuffer[maxSequenceRange];
int64_t  sequenceBuffer[maxSequenceRange];

int generateSequences();
void printRandomSequence(int64_t *buffer);

/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {

	std::cerr << "-------------------------------------------------------------------------------------------------" << std::endl;
	std::cout << "--- A program for generating random sequences of unique integer numbers using SwiftRNG device ---" << std::endl;
	std::cout << "-------------------------------------------------------------------------------------------------" << std::endl;

	if (argc >= 4) {
		deviceNumber = atol(argv[1]);
		if (deviceNumber < 0 || deviceNumber > 127) {
			std::cerr << "Invalid SwiftRNG device number" << std::endl;
			return -1;
		}

		minNumber = atol(argv[2]);
		if (minNumber < -100000000) {
			std::cerr << "The smallest number in the range cannot be smaller then -100000000" << std::endl;
			return -1;
		}

		maxNumber = atol(argv[3]);
		if (maxNumber > 100000000) {
			std::cerr << "The largest number in the range cannot be bigger then 100000000" << std::endl;
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
			numberCount = atol(argv[4]);
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
		return(1);
	}

	return generateSequences();
}

/**
 * Generate sequences
 * @return int 0 - successful or error code
 */
int generateSequences() {
	SWRNGRandomSeqGenerator seqGen(deviceNumber, range);

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
void printRandomSequence(int64_t *buffer) {
	// Print sequence numbers
	std::cout << std::endl << "-- Beginning of random sequence --" << std::endl;
	for (uint32_t i = 0; i < numberCount; i++) {
		std::cout << (int)buffer[i] << std::endl;
	}
	std::cout << "-- Ending of random sequence --" << std::endl;
}

