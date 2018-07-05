/*
 * swperf-cl-test.c
 * Ver. 1.4
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2018 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for testing the performance of a cluster of one or more SwiftRNG devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include "swrng-cl-api.h"

#define SAMPLES (100000)		// Number of random bytes per block to download
#define NUM_BLOCKS (5000)		// Total blocks to read

static unsigned char randonbuffer[SAMPLES];
static SwrngCLContext ctxt;

static int status;
static time_t start, end;
static long l;


/**
 * Run performance test using current post processing method
 * @return 0 - if successful, error otherwise
 */
int runPerfTest() {
	// Wake up the device for best performance
	status = swrngGetCLEntropy(&ctxt, randonbuffer, SAMPLES);
	if (status != SWRNG_SUCCESS) {
		printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return status;
	}

	printf("Performance ------- in progress ------------------ ");
	start = time(NULL);
	for (l = 0; l < NUM_BLOCKS; l++) {
		// Download random bytes from the device
		status = swrngGetCLEntropy(&ctxt, randonbuffer, SAMPLES);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return status;
		}
	}
	end = time(NULL);

	int64_t totalTime = end - start;
	if (totalTime == 0) {
		totalTime = 1;
	}
	double downloadSpeed = (SAMPLES * NUM_BLOCKS) / totalTime / 1000.0 / 1000.0 * 8.0;

	printf("%3.2f Mbits/sec\n", downloadSpeed);
	return SWRNG_SUCCESS;
}

/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {
	int clusterSize = 2;

	printf("------------------------------------------------------------------------\n");
	printf("-- swperf-cl-test - SwiftRNG device cluster performance test utility  --\n");
	printf("------------------------------------------------------------------------\n");

	if (argc > 1) {
		clusterSize = atoi(argv[1]);
	} else {
		printf("Usage: swperf-cl-test <cluster size>\n");
	}

	setbuf(stdout, NULL);

	// Open SwiftRNG device cluster if available
	if (swrngCLOpen(&ctxt, clusterSize) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetCLLastErrorMessage(&ctxt));
		return(1);
	}

	printf("\nCluster preferred size: %d, actual cluster size: %d (successfully open)\n\n", clusterSize, swrngGetCLSize(&ctxt));


	status = swrngGetCLEntropy(&ctxt, randonbuffer, SAMPLES);
	if (status != SWRNG_SUCCESS) {
		printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return status;
	}

	printf("Setting power profiles to %1d ------------------------------- ", 9);
	status = swrngSetCLPowerProfile(&ctxt, 9);
	if (status != SWRNG_SUCCESS) {
		printf("*FAILED*, err: %s\n", swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return status;
	}
	printf("Success\n");

	status = swrngDisableCLPostProcessing(&ctxt);
	if (status == SWRNG_SUCCESS) {
		printf("\n");
		printf("Post processing  ---( for devices with version 1.2+ )----- disabled \n");
		if (runPerfTest()) {
			swrngCLClose(&ctxt);
			return status;
		}

	}

	status = swrngEnableCLPostProcessing(&ctxt, 0);
	if (status == SWRNG_SUCCESS) {
		printf("\n");
		printf("Post processing  ------------------------------------------- SHA256 \n");
		if (runPerfTest()) {
			swrngCLClose(&ctxt);
			return status;
		}

	}

	status = swrngEnableCLPostProcessing(&ctxt, 2);
	if (status == SWRNG_SUCCESS) {
		printf("\n");
		printf("Post processing  ------------------------------------------- SHA512 \n");
		if (runPerfTest()) {
			swrngCLClose(&ctxt);
			return status;
		}

	}

	status = swrngEnableCLPostProcessing(&ctxt, 1);
	if (status == SWRNG_SUCCESS) {
		printf("\n");
		printf("Post processing  ---( for devices with version 1.2+ )--- xorshift64 \n");
		if (runPerfTest()) {
			swrngCLClose(&ctxt);
			return status;
		}
	}

	printf("Closing device -------------------------------------------- ");
	swrngCLClose(&ctxt);
	printf("Success\n");

	printf("\n");
	printf("-------------------------------------------------------------------\n");
	return 0;

}
