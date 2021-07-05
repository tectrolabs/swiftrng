#include "stdafx.h"
/*
 * swdiag-cl.cpp
 * Ver. 2.0
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2019 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for running diagnostics for a cluster of one or more SwiftRNG devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "swrng-cl-api.h"
#include <math.h>

#define SAMPLES (10000)		// Number of random bytes per block to retrieve
#define NUM_BLOCKS (1000)		// Total blocks to read
#define EXTLOOPS (5)
#define ENTROPY_SCORE_BYTES (24000000)
#define MAX_CHUNK_SIZE_BYTES (100000)

static void chi_sqrd_count_bits(uint8_t byte, double *ones, double *zeros);
static double chi_sqrd_calculate(void);
static int run_chi_squire_test(long idx);
static int calculate_entropy_score(void);


static double actOnes;
static double actZeros;
static double expctOnes = 4 * SAMPLES;
static double expctZeros = 4 * SAMPLES;
static double chiSquare;
static double chiSquareSum;

static unsigned char randonbuffer[SAMPLES];
static unsigned char entropybuffer[ENTROPY_SCORE_BYTES];
static unsigned long entropyfreqbuff[256];

SwrngCLContext ctxt;

/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {
	long l, k;
	long p;
	int status;
	int clusterSize = 2;

	printf("------------------------------------------------------------------------------\n");
	printf("--- TectroLabs - swdiag-cl - SwiftRNG cluster diagnostics utility Ver 2.0  ---\n");
	printf("------------------------------------------------------------------------------\n");

	setbuf(stdout, NULL);

	if (argc > 1) {
		clusterSize = atoi(argv[1]);
	} else {
		printf("Usage: swdiag-cl <cluster size>\n");
	}


	// Initialize the context
	if (swrngInitializeCLContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		return (1);
	}

	printf("Opening cluster--------------- ");

	// Open the cluster if any SwiftRNG device if available
	if (swrngCLOpen(&ctxt, clusterSize) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetCLLastErrorMessage(&ctxt));
		return (1);
	}

	printf("SwiftRNG cluster of %d devices open successfully\n\n", swrngGetCLSize(&ctxt));

	status = swrngSetCLPowerProfile(&ctxt, 9);
	if (status != SWRNG_SUCCESS) {
		printf("*** Could not set power profile, err: %s\n",
				swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return status;
	}

	printf(
			"-------- Running APT, RCT and device built-in tests ---------------\n");
	printf("Retrieving %d blocks of %6d random bytes each -------- ",
			NUM_BLOCKS, SAMPLES);
	for (l = 0; l < NUM_BLOCKS; l++) {
		// Retrieve random bytes from device
		status = swrngGetCLEntropy(&ctxt, randonbuffer, SAMPLES);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return status;
		}
	}
	printf("Success\n");
	for (p = 0; p < 10; p++) {
		printf("\nSetting power profiles to %1ld ------------------------------- ",
				p);
		status = swrngSetCLPowerProfile(&ctxt, p);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return status;
		}
		status = swrngGetCLEntropy(&ctxt, randonbuffer, SAMPLES);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return status;
		}
		printf("Success\n");

		// Retrieve random data for entropy score
		printf("Entropy score for %8d bytes -------------------------- ", ENTROPY_SCORE_BYTES);
		for (k = 0; k < ENTROPY_SCORE_BYTES; k+=MAX_CHUNK_SIZE_BYTES)
		status = swrngGetCLEntropy(&ctxt, entropybuffer + k, MAX_CHUNK_SIZE_BYTES);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return status;
		}
		status = calculate_entropy_score();
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*");
			swrngCLClose(&ctxt);
			return status;
		}
		printf("\n");

		printf("---------------- Running Chi-Square test %3d times  --------------- \n",
				EXTLOOPS);
		for (l = 0; l < EXTLOOPS; l++) {
			status = run_chi_squire_test(l);
			if (status != SWRNG_SUCCESS) {
				swrngCLClose(&ctxt);
				return status;
			}
		}
	}
	printf("-------------------------------------------------------------------\n");
	printf("Number of cluster fail-over events ------------------------ %ld\n", swrngGetCLFailoverEventCount(&ctxt));
	printf("Closing cluster -------------------------------------------- ");
	swrngCLClose(&ctxt);
	printf("Success\n");

	printf("-------------------------------------------------------------------\n");
	printf("----------------- All tests passed successfully -------------------\n");
	return 0;
}

/**
 * Count number of 'ones' and 'zeros' in provided byte value
 *
 * @param uint8_t byte - byte value to process
 * @param double *ones - pointer to the 'ones' counter
 * @param double *zeros - pointer to the 'zeros' counter
 */

void chi_sqrd_count_bits(uint8_t byte, double *ones, double *zeros) {
	int i;
	uint8_t val = byte;
	for (i = 0; i < 8; i++) {
		if (val & 0x1) {
			(*ones)++;
		} else {
			(*zeros)++;
		}
		val >>= 1;
	}
}

/**
 * Calculate chi-squire value
 *
 * @return double - calculated chi-squire value
 */
double chi_sqrd_calculate() {
	double val1 = (actOnes - expctOnes);
	double val2 = (actZeros - expctZeros);
	return val1 * val1 / expctOnes + val2 * val2 / expctZeros;
}

/**
 * Generate and print entropy score
 * @return int 0 - successful or error code
 */
static int calculate_entropy_score() {
	int i;
	double p, score;

	for(i = 0; i < 256; i++) {
		entropyfreqbuff[i] = 0;
	}

	for(i = 0; i < ENTROPY_SCORE_BYTES; i++) {
		entropyfreqbuff[entropybuffer[i]] ++;
	}

	score = 0;
	for(i = 0; i < 256; i++) {
		if (entropyfreqbuff[i] > 0) {
			p = (double)entropyfreqbuff[i] / (double)ENTROPY_SCORE_BYTES;
			score -= p * log2(p);
		}
	}

	printf("%6.4lf ", score);

	if (score > 7.9) {
		if (score >=7.99) {
			printf("(Full Entropy)");
		} else {
			printf("(Warning)");
		}
		return SWRNG_SUCCESS;
	}
	return -1;
}

/**
 * Run chi-squire test
 *
 * @param long idx - test number
 * @return int 0 - successful or error code
 */
int run_chi_squire_test(long idx) {
	long l;
	int status;

	printf("Average chi-square for test %3ld --------------------------- ",
			idx + 1);
	chiSquareSum = 0;
	for (l = 0; l < NUM_BLOCKS; l++) {
		// Retrieve random bytes from device
		status = swrngGetCLEntropy(&ctxt, randonbuffer, SAMPLES);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			return status;
		}
		actOnes = 0;
		actZeros = 0;
		int i = 0;
		for (i = 0; i < SAMPLES; i++) {
			chi_sqrd_count_bits(randonbuffer[i], &actOnes, &actZeros);
		}
		chiSquare = chi_sqrd_calculate();
		chiSquareSum += chiSquare;
	}
	double avgSquare = chiSquareSum / NUM_BLOCKS;
	printf("%f ", avgSquare);
	if (avgSquare > 3.84) {
		printf("(Not acceptable)\n");
		return -1;
	} else if (avgSquare < 0.004) {
		printf("(Weak)\n");
	} else {
		printf("(Acceptable)\n");
	}

	return SWRNG_SUCCESS;
}
