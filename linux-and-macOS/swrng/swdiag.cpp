#include "stdafx.h"

/*
 * swdiag.cpp
 * Ver. 2.3
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for running diagnostics for one or more SwiftRNG devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include "swrngapi.h"
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
static int printFrequencyTableSummary(uint16_t frequencyTable[]);
static int inspectRawData(NoiseSourceRawData *rawData1, NoiseSourceRawData *rawData2);

static double 	actOnes;
static double  	actZeros;
static double  	expctOnes = 4 * SAMPLES;
static double  	expctZeros = 4 * SAMPLES;
static double 	chiSquare;
static double 	chiSquareSum;

static unsigned char randonbuffer[SAMPLES];
static unsigned char entropybuffer[ENTROPY_SCORE_BYTES];
static unsigned long entropyfreqbuff[256];
static FrequencyTables freqTables;
static NoiseSourceRawData noiseSourceOneRawData;
static NoiseSourceRawData noiseSourceTwoRawData;
static int embeddedCorrectionMethodId;
SwrngContext ctxt;

/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main() {
	DeviceInfoList dil;
	long i, k;
	long l;
	int p;
	int status;
	double actualDeviceVersion;
	int postProcessingEnabled;

	printf("-------------------------------------------------------------------\n");
	printf("--- TectroLabs - swdiag - SwiftRNG diagnostics utility Ver 2.3  ---\n");
	printf("-------------------------------------------------------------------\n");
	printf("Searching for devices ------------------ ");

	setbuf(stdout, NULL);

	// Initialize the context
	status = swrngInitializeContext(&ctxt);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not initialize context\n");
		return(status);
	}

	status = swrngGetDeviceList(&ctxt, &dil);
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

		status = swrngOpen(&ctxt, i);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, error: %s\n", swrngGetLastErrorMessage(&ctxt));
			return status;
		}
		printf("Success\n");

		status = swrngSetPowerProfile(&ctxt, 9);
		if (status != SWRNG_SUCCESS) {
			printf("*** Could not set power profile, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngClose(&ctxt);
			return status;
		}

		status = swrngGetVersionNumber(&ctxt, &actualDeviceVersion);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngClose(&ctxt);
			return status;
		}

		if(actualDeviceVersion >= 1.2) {
			printf("\n------------- Disabling post processing for this device -----------");
			status = swrngDisablePostProcessing(&ctxt);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngClose(&ctxt);
				return status;
			}
		}

		status = swrngGetPostProcessingStatus(&ctxt, &postProcessingEnabled);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngClose(&ctxt);
			return status;
		}
		if (postProcessingEnabled) {
			printf("\n------------- Post processing enabled for this device -------------");
		} else {
			printf("\n------------- Post processing disabled for this device ------------");
			printf("\n------------- Tests will be performed on RAW byte stream ----------");
		}

		if (swrngGetEmbeddedCorrectionMethod(&ctxt, &embeddedCorrectionMethodId) != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngClose(&ctxt);
			return status;
		}

		switch (embeddedCorrectionMethodId) {
		case 0:
			printf("\n------------- Using no embedded correction algorithm --------------");
			break;
		case 1:
			printf("\n------------- Using embedded correction algorithm: Linear ---------");
			break;
		default:
			printf("\n------------- Unknown built-in correction algorithm ---------------");
		}

		if (actualDeviceVersion >= 2.0) {
			// This feature was only implemented in devices with version 2.0 and up
			printf("\n\n---------- Running device internal diagnostics  ----------  ");
			status = swrngRunDeviceDiagnostics(&ctxt);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngClose(&ctxt);
				return status;
			}
			printf("Success\n");
		}

		if (actualDeviceVersion >= 1.2) {
			// These features were only implemented in devices with version 1.2 and up
			printf("\n-------------- Verifying noise sources of the device --------------\n");

			printf("\n-------- Retrieving RAW data from noise sources  ---------  ");
			status = swrngGetRawDataBlock(&ctxt, &noiseSourceOneRawData, 0);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngClose(&ctxt);
				return status;
			}
			status = swrngGetRawDataBlock(&ctxt, &noiseSourceTwoRawData, 1);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngClose(&ctxt);
				return status;
			}
			printf("Success\n");

			printf("----------  Inspecting RAW data of the noise sources  -------------\n");
			status = inspectRawData(&noiseSourceOneRawData, &noiseSourceTwoRawData);
			if ( status != SWRNG_SUCCESS) {
				swrngClose(&ctxt);
				return status;
			}

			status = swrngGetFrequencyTables(&ctxt, &freqTables);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngClose(&ctxt);
				return status;
			}
			printf("\n-------- Retrieving frequency table for noise source 1 ------------\n");
			status = printFrequencyTableSummary(freqTables.freqTable1);
			if ( status != SWRNG_SUCCESS) {
				swrngClose(&ctxt);
				return status;
			}
			printf("\n-------- Retrieving frequency table for noise source 2 ------------\n");
			status = printFrequencyTableSummary(freqTables.freqTable2);
			if ( status != SWRNG_SUCCESS) {
				swrngClose(&ctxt);
				return status;
			}
		}
		printf("\n-------- Running APT, RCT and device built-in tests ---------------\n");
		printf("Retrieving %d blocks of %6d random bytes each -------- ", NUM_BLOCKS, SAMPLES);
		// APT and RCT tests are implemented in SwiftRNG Software API.
		// Those tests are also embedded in SwiftRNG devices
		for (l = 0; l < NUM_BLOCKS; l++) {
			// Retrieve random bytes from device
			status = swrngGetEntropy(&ctxt, randonbuffer, SAMPLES);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngClose(&ctxt);
				return status;
			}
		}
		printf("Success\n");

		for (p = 0; p < 10; p++) {
			// Power profiles are only implemented in 'SwiftRNG' models
			if (!strcmp("SwiftRNG", dil.devInfoList[i].dm.value)) {
				printf("\nSetting power profiles to %1d ------------------------------- ", p);
				status = swrngSetPowerProfile(&ctxt, p);
				if (status != SWRNG_SUCCESS) {
					printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
					swrngClose(&ctxt);
					return status;
				}
				status = swrngGetEntropy(&ctxt, randonbuffer, SAMPLES);
				if (status != SWRNG_SUCCESS) {
					printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
					swrngClose(&ctxt);
					return status;
				}
				printf("Success\n");
			}

			// Retrieve random data for entropy score
			printf("Entropy score for %8d bytes -------------------------- ", ENTROPY_SCORE_BYTES);
			for (k = 0; k < ENTROPY_SCORE_BYTES; k+=MAX_CHUNK_SIZE_BYTES)
			status = swrngGetEntropy(&ctxt, entropybuffer + k, MAX_CHUNK_SIZE_BYTES);
			if (status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngClose(&ctxt);
				return status;
			}
			status = calculate_entropy_score();
			if (status != SWRNG_SUCCESS) {
				printf("*FAILED*");
				swrngClose(&ctxt);
				return status;
			}
			printf("\n");

			printf("-------- Running Chi-Square test (%1d times), step %1d of 10 ---------- \n", EXTLOOPS, p+1);
			for (l = 0; l < EXTLOOPS; l++) {
				status = run_chi_squire_test(l);
				if (status != SWRNG_SUCCESS) {
					swrngClose(&ctxt);
					return status;
				}
			}
		}
		printf("Maximum RCT/APT failures per block: ----------------------- %d/%d      ", ctxt.maxRctFailuresPerBlock, ctxt.maxAptFailuresPerBlock);
		if (ctxt.maxRctFailuresPerBlock >= 3 || ctxt.maxAptFailuresPerBlock >= 3) {
			printf("   (Warning)");
		} else {
			printf("(Acceptable)\n");
		}
		printf("Closing device -------------------------------------------- ");
		swrngClose(&ctxt);
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
 * Inspect raw random data of the noise sources
 * @param NoiseSourceRawData *rawData1 - raw random data from noise source 1
 * @param NoiseSourceRawData *rawData2 - raw random data from noise source 2
 * @return int - status
 */
static int inspectRawData(NoiseSourceRawData *rawData1, NoiseSourceRawData *rawData2) {
	int i;
	int count1, count2;
	int minFreq1, minFreq2, maxFreq1, maxFreq2;
	uint16_t frequencyTableSource1[256];
	uint16_t frequencyTableSource2[256];
	int freqRange1;
	int freqRange2;

	// Clear the frequency tables
	for (i = 0; i < 256; i++) {
		frequencyTableSource1[i] = 0;
		frequencyTableSource2[i] = 0;
	}


	// Fill the frequency tables
	for (i = 0; i < 16000; i++) {
		frequencyTableSource1[(uint8_t)rawData1->value[i]]++;
		frequencyTableSource2[(uint8_t)rawData2->value[i]]++;
	}

	// Calculate min and max values for each table
	minFreq1 = frequencyTableSource1[0];
	maxFreq1 = frequencyTableSource1[0];
	minFreq2 = frequencyTableSource2[0];
	maxFreq2 = frequencyTableSource2[0];
	count1 = 0;
	count2 = 0;
	for (i = 0; i < 256; i++) {
		count1 += frequencyTableSource1[i];
		count2 += frequencyTableSource2[i];
		if (frequencyTableSource1[i] > maxFreq1) {
			maxFreq1 = frequencyTableSource1[i];
		}
		if (frequencyTableSource1[i] < minFreq1) {
			minFreq1 = frequencyTableSource1[i];
		}
		if (frequencyTableSource2[i] > maxFreq2) {
			maxFreq2 = frequencyTableSource2[i];
		}
		if (frequencyTableSource2[i] < minFreq2) {
			minFreq2 = frequencyTableSource2[i];
		}
	}

	printf("Frequency table source 1: min %d, max %d, samples %d", minFreq1, maxFreq1, count1);
	freqRange1 = maxFreq1 - minFreq1;
	if (freqRange1 > 200.0 || count1 != 16000) {
		printf(" *FAILED*\n");
		return -1;
	} else if (freqRange1 > 100.0) {
		printf(" *WARNING*\n");
	} else {
		printf(" (healthy)\n");
	}

	printf("Frequency table source 2: min %d, max %d, samples %d", minFreq2, maxFreq2, count2);
	freqRange2 = maxFreq2 - minFreq2;
	if (freqRange2 > 200.0 || count2 != 16000) {
		printf(" *FAILED*\n");
		return -1;
	} else if (freqRange2 > 100.0) {
		printf(" *WARNING*\n");
	} else {
		printf(" (healthy)\n");
	}

	return SWRNG_SUCCESS;
}

/**
 * Inspect and print retrieved frequency tables
 * @param uint16_t table[] - frequency table pointer
 * @return int - status
 */
int printFrequencyTableSummary(uint16_t frequencyTable[]) {
	int k;
	int minFreq;
	int maxFreq;
	int totalSamples;
	int freqRange;

	minFreq = frequencyTable[0];
	maxFreq = frequencyTable[0];
	totalSamples = 0;
	for (k = 0; k < 256; k += 8) {
		printf("(%3d : %3d)  %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d \n", k, k+7,
				frequencyTable[k+0],
				frequencyTable[k+1],
				frequencyTable[k+2],
				frequencyTable[k+3],
				frequencyTable[k+4],
				frequencyTable[k+5],
				frequencyTable[k+6],
				frequencyTable[k+7]);
	}

	for (k = 0; k < 256; k++) {
		totalSamples += frequencyTable[k];
		if (frequencyTable[k] > maxFreq) {
			maxFreq = frequencyTable[k];
		}
		if (frequencyTable[k] < minFreq) {
			minFreq = frequencyTable[k];
		}
	}
	printf("-------------------------------------------------------------------\n");
	printf("Table summary: min %d, max %d, total samples %d", minFreq, maxFreq, totalSamples);
	freqRange = maxFreq - minFreq;
	if (freqRange > 200.0 || totalSamples != 16000) {
		printf(" *FAILED*\n");
		return -1;
	} else if (freqRange > 100.0) {
		printf(" *WARNING*\n");
	} else {
		printf(" (healthy)\n");
	}
	return SWRNG_SUCCESS;
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
		// Retrieve random bytes from device
		status = swrngGetEntropy(&ctxt, randonbuffer, SAMPLES);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
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
