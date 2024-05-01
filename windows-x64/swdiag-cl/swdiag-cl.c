/*
 * swdiag-cl.c
 * Ver. 2.6
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2024 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for running diagnostics for a cluster of one or more SwiftRNG devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <swrng-cl-api.h>
#include <math.h>

/* Number of random bytes per block to retrieve */
#define SAMPLES (10000)

/* Total blocks to read */
#define NUM_BLOCKS (1000)

#define EXTLOOPS (5)
#define ENTROPY_SCORE_BYTES (24000000)
#define MAX_CHUNK_SIZE_BYTES (100000)

static void chi_sqrd_count_bits(uint8_t byte, double *ones, double *zeros);
static double chi_sqrd_calculate(void);
static int run_chi_squire_test(long idx);
static int calculate_entropy_score(void);


static double act_ones;
static double act_zeros;
static double expct_ones = 4 * SAMPLES;
static double expct_zeros = 4 * SAMPLES;
static double chi_square;
static double chi_square_sum;

static unsigned char rnd_buffer[SAMPLES];
static unsigned char entropy_buffer[ENTROPY_SCORE_BYTES];
static unsigned long entropy_freq_buff[256];

SwrngCLContext ctxt;

/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {
	int status;
	int cluster_size = 2;

	printf("------------------------------------------------------------------------------\n");
	printf("--- TectroLabs - swdiag-cl - SwiftRNG cluster diagnostics utility Ver 2.6  ---\n");
	printf("------------------------------------------------------------------------------\n");

	setbuf(stdout, NULL);

	if (argc > 1) {
		cluster_size = atoi(argv[1]);
	} else {
		printf("Usage: swdiag-cl <cluster size>\n");
	}


	/* Initialize the context */
	if (swrngInitializeCLContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		return 1;
	}

	printf("Opening cluster--------------- ");

	/* Open the cluster if any SwiftRNG device if available */
	if (swrngCLOpen(&ctxt, cluster_size) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetCLLastErrorMessage(&ctxt));
		return 1;
	}

	printf("SwiftRNG cluster of %d devices open successfully\n\n", swrngGetCLSize(&ctxt));

	status = swrngSetCLPowerProfile(&ctxt, 9);
	if (status != SWRNG_SUCCESS) {
		printf("*** Could not set power profile, err: %s\n",
				swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return status;
	}

	printf("-------- Running APT, RCT and device built-in tests ---------------\n");
	printf("Retrieving %d blocks of %6d random bytes each -------- ",
			NUM_BLOCKS, SAMPLES);
	for (long l = 0; l < NUM_BLOCKS; l++) {
		/* Retrieve random bytes from device */
		status = swrngGetCLEntropy(&ctxt, rnd_buffer, SAMPLES);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return status;
		}
	}
	printf("Success\n");
	for (int p = 0; p < 10; p++) {
		printf("\nSetting power profiles to %1d ------------------------------- ", p);
		status = swrngSetCLPowerProfile(&ctxt, p);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return status;
		}
		status = swrngGetCLEntropy(&ctxt, rnd_buffer, SAMPLES);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return status;
		}
		printf("Success\n");

		/* Retrieve random data for entropy score */
		printf("Entropy score for %8d bytes -------------------------- ", ENTROPY_SCORE_BYTES);
		for (long k = 0; k < ENTROPY_SCORE_BYTES; k += MAX_CHUNK_SIZE_BYTES) {
			status = swrngGetCLEntropy(&ctxt, entropy_buffer + k, MAX_CHUNK_SIZE_BYTES);
			if (status != SWRNG_SUCCESS) {
				break;
			}
		}
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
		for (long l = 0; l < EXTLOOPS; l++) {
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

static void chi_sqrd_count_bits(uint8_t byte, double *ones, double *zeros) {
	uint8_t val = byte;
	for (int i = 0; i < 8; i++) {
		if (val & 0x1) {
			(*ones)++;
		} else {
			(*zeros)++;
		}
		val >>= 1;
	}
}

/**
 * Calculate chi-squire valu
 *
 * @return double - calculated chi-squire value
 */
static double chi_sqrd_calculate() {
	double val1 = (act_ones - expct_ones);
	double val2 = (act_zeros - expct_zeros);
	return val1 * val1 / expct_ones + val2 * val2 / expct_zeros;
}

/**
 * Generate and print entropy score
 * @return int 0 - successful or error code
 */
static int calculate_entropy_score() {
	double p;
	double score;

	for(int i = 0; i < 256; i++) {
		entropy_freq_buff[i] = 0;
	}

	for(int i = 0; i < ENTROPY_SCORE_BYTES; i++) {
		entropy_freq_buff[entropy_buffer[i]] ++;
	}

	score = 0;
	for(int i = 0; i < 256; i++) {
		if (entropy_freq_buff[i] > 0) {
			p = (double)entropy_freq_buff[i] / (double)ENTROPY_SCORE_BYTES;
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
 * Run cgi-squire test
 *
 * @param long idx - test number
 * @return int 0 - successful or error code
 */
static int run_chi_squire_test(long idx) {
	int status;

	printf("Average chi-square for test %3ld --------------------------- ",
			idx + 1);
	chi_square_sum = 0;
	for (long l = 0; l < NUM_BLOCKS; l++) {
		/* Retrieve random bytes from device */
		status = swrngGetCLEntropy(&ctxt, rnd_buffer, SAMPLES);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			return status;
		}
		act_ones = 0;
		act_zeros = 0;
		for (int i = 0; i < SAMPLES; i++) {
			chi_sqrd_count_bits(rnd_buffer[i], &act_ones, &act_zeros);
		}
		chi_square = chi_sqrd_calculate();
		chi_square_sum += chi_square;
	}
	double avgSquare = chi_square_sum / NUM_BLOCKS;
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
