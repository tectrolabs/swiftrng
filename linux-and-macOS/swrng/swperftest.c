/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
 * swperftest.c
 * Ver. 2.5
 *
 * @brief This program is used for testing the performance of the SwiftRNG devices.
 */
#include <stdio.h>
#include <swrngapi.h>

/* Number of random bytes per block to retrieve */
#define SAMPLES (100000)

/* Total blocks to read */
#define NUM_BLOCKS (5000)

static unsigned char rnd_buffer[SAMPLES];
static SwrngContext ctxt;
static DeviceInfoList dil;

static int status;
static time_t start;
static time_t end;
static long l;
static int emb_corr_method_id;


/**
 * Run performance test using current post processing method
 * @return 0 - if successful, error otherwise
 */
int runPerfTest() {
	/* Wake up the device for best performance */
	status = swrngGetEntropy(&ctxt, rnd_buffer, SAMPLES);
	if (status != SWRNG_SUCCESS) {
		printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
		swrngDestroyContext(&ctxt);
		return status;
	}

	printf("Performance ------- in progress ------------------ ");
	start = time(NULL);
	for (l = 0; l < NUM_BLOCKS; l++) {
		/* Retrieve random bytes from the device */
		status = swrngGetEntropy(&ctxt, rnd_buffer, SAMPLES);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
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
int main() {

	printf("------------------------------------------------------------\n");
	printf("-- swperftest - SwiftRNG device performance test utility  --\n");
	printf("------------------------------------------------------------\n");
	printf("Searching for devices ------------------ ");


	setbuf(stdout, NULL);

	/* Initialize the context */
	status = swrngInitializeContext(&ctxt);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not initialize context\n");
		return status;
	}

	status = swrngGetDeviceList(&ctxt, &dil);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not generate device info list, status: %d\n", status);
		swrngDestroyContext(&ctxt);
		return status;
	}

	if (dil.numDevs > 0) {
		printf("found %d SwiftRNG device(s)\n", dil.numDevs);
	} else {
		printf("  no SwiftRNG device found\n");
		swrngDestroyContext(&ctxt);
		return -1;
	}

	for (int i = 0; i < dil.numDevs; i++) {
		printf("\n\n");
		printf("Testing ");
		printf("%s", dil.devInfoList[i].dm.value);
		printf(" with S/N: %s", dil.devInfoList[i].sn.value);
		printf(" version: %s\n", dil.devInfoList[i].dv.value);

		printf("Opening device -------------------------------------------- ");

		status = swrngOpen(&ctxt, i);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, error: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return status;
		}
		printf("Success\n");

		printf("Setting power profiles to %1d ------------------------------- ", 9);
		status = swrngSetPowerProfile(&ctxt, 9);
		if (status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return status;
		}
		printf("Success\n");

		if (swrngGetEmbeddedCorrectionMethod(&ctxt, &emb_corr_method_id) != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return status;
		}

		switch (emb_corr_method_id) {
		case 0:
			printf("Embedded correction algorithm -------------------------------- none");
			break;
		case 1:
			printf("Embedded correction algorithm ------------------------------ Linear");
			break;
		default:
			printf("Embedded correction algorithm ----------------------------- unknown");
		}

		printf("\n");

		status = swrngDisablePostProcessing(&ctxt);
		if (status == SWRNG_SUCCESS) {
			printf("\n");
			printf("Post processing  ----------------------------------------- disabled \n");
			if (runPerfTest()) {
				swrngDestroyContext(&ctxt);
				return status;
			}

		}

		status = swrngEnablePostProcessing(&ctxt, 0);
		if (status == SWRNG_SUCCESS) {
			printf("\n");
			printf("Post processing  ------------------------------------------- SHA256 \n");
			if (runPerfTest()) {
				swrngDestroyContext(&ctxt);
				return status;
			}

		}

		status = swrngEnablePostProcessing(&ctxt, 2);
		if (status == SWRNG_SUCCESS) {
			printf("\n");
			printf("Post processing  ------------------------------------------- SHA512 \n");
			if (runPerfTest()) {
				swrngDestroyContext(&ctxt);
				return status;
			}

		}

		status = swrngEnablePostProcessing(&ctxt, 1);
		if (status == SWRNG_SUCCESS) {
			printf("\n");
			printf("Post processing  --------------------------------------- xorshift64 \n");
			if (runPerfTest()) {
				swrngDestroyContext(&ctxt);
				return status;
			}
		}

		printf("Closing device -------------------------------------------- ");
		swrngClose(&ctxt);
		printf("Success\n");

	}

	swrngDestroyContext(&ctxt);
	printf("\n");
	printf("-------------------------------------------------------------------\n");
	return 0;

}
