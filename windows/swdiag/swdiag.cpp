#include "stdafx.h"

/*
 * swdiag.c
 * ver. 1.0
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2016 TectroLabs, http://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for testing one or more plugged in SwiftRNG devices.

 This program may require 'sudo' permissions when running on Linux or OSX operating systems.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include "swrngapi.h"
#include <math.h>

#define SAMPLES (10000)		// Number of random bytes per block to download
#define NUM_BLOCKS (1000)		// Total blocks to read
#define EXTLOOPS (5)

static void chi_sqrd_count_bits(uint8_t byte, double *ones, double *zeros);
static double chi_sqrd_calculate(void);
static int run_chi_squire_test(long idx);

static double 	actOnes;
static double  	actZeros;
static double  	expctOnes = 4 * SAMPLES;
static double  	expctZeros = 4 * SAMPLES;
static double 	chiSquare;
static double 	chiSquareSum;

static unsigned char randonbuffer[SAMPLES + 1];

/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main() {
	DeviceInfoList dil;
	long i;
	long l;
	long p;
	int status;

	printf("-------------------------------------------------------------------\n");
	printf("--- TectroLabs - swdiag - SwiftRNG diagnostics utility Ver 1.0  ---\n");
	printf("-------------------------------------------------------------------\n");
	printf("Searching for devices ------------------ ");

	setbuf(stdout, NULL);

	status = swrngGetDeviceList(&dil);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not generate device info list, status: %d\n", status);
		return status;
	}

	if (dil.numDevs > 0) {
		printf("found %d SwiftRNG device(s)\n", dil.numDevs);
	} else {
		printf("  no SwiftRNG device found\n");
		return -1;
	}

	for (i = 0; i < dil.numDevs; i++) {
		printf("\n\n");
		printf("Testing ");
		printf("%s", dil.devInfoList[i].dm.value);
		printf(" with S/N: %s", dil.devInfoList[i].sn.value);
		printf(" version: %s\n", dil.devInfoList[i].dv.value);

		printf("Opening device -------------------------------------------- ");

		status = swrngOpen(i);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, error: %s\n", swrngGetLastErrorMessage());
			return status;
		}
		printf("Success\n");

		status = swrngSetPowerProfile(9);
		if (status != SWRNG_SUCCESS) {
			printf("*** Could not set power profile, err: %s\n", swrngGetLastErrorMessage());
			swrngClose();
			return status;
		}


		printf("-------- Running APT, RCT and device built-in tests ---------------\n");
		printf("Retrieving %d blocks of %6d random bytes each -------- ", NUM_BLOCKS, SAMPLES);
		for (l = 0; l < NUM_BLOCKS; l++) {
			// Download random bytes from device
			status = swrngGetEntropy(randonbuffer, SAMPLES);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage());
				swrngClose();
				return status;
			}
		}
		printf("Success\n");

		for (p=0; p< 10; p++) {
			printf("Setting power profiles to %1ld ------------------------------- ", p);
			status = swrngSetPowerProfile(p);
			if (status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage());
				swrngClose();
				return status;
			}
			status = swrngGetEntropy(randonbuffer, SAMPLES);
			if (status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage());
				swrngClose();
				return status;
			}
			printf("Success\n");

			printf("---------------- Running Chi-Square test %3d times  --------------- \n", EXTLOOPS);
			for (l = 0; l < EXTLOOPS; l++) {
				status = run_chi_squire_test(l);
				if (status != SWRNG_SUCCESS) {
					swrngClose();
					return status;
				}
			}
		}

		printf("Closing device -------------------------------------------- ");
		swrngClose();
		printf("Success\n");

	}

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
 * Calculate chi-squire valu
 *
 * @return double - calculated chi-squire value
 */
double chi_sqrd_calculate() {
	return pow((actOnes - expctOnes), 2) / expctOnes + pow((actZeros - expctZeros), 2) / expctZeros;
}

/**
 * Run cgi-squire test
 *
 * @param long idx - test number
 * @return int 0 - successful or error code
 */
int run_chi_squire_test(long idx) {
	long l;
	int status;

	printf("Average chi-square for test %3ld --------------------------- ", idx + 1);
	chiSquareSum = 0;
	for (l = 0; l < NUM_BLOCKS; l++) {
		// Download random bytes from device
		status = swrngGetEntropy(randonbuffer, SAMPLES);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage());
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
	double avgSquare = chiSquareSum/NUM_BLOCKS;
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

