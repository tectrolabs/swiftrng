/*
 * swdiag.c
 * Ver. 2.6
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2023 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for running diagnostics for one or more SwiftRNG devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <swrngapi.h>

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
static int print_frequency_table_summary(uint16_t *frequency_table);
static int inspectRawData(NoiseSourceRawData *raw_data_1, NoiseSourceRawData *raw_data_2);

static double 	act_ones;
static double  	act_zeros;
static double  	expct_ones = 4 * SAMPLES;
static double  	expct_zeros = 4 * SAMPLES;
static double 	chi_square;
static double 	chi_square_sum;

static unsigned char rnd_buffer[SAMPLES];
static unsigned char entropy_buffer[ENTROPY_SCORE_BYTES];
static unsigned long entropy_freq_buff[256];
static FrequencyTables freq_tables;
static NoiseSourceRawData noise_source_one_raw_data;
static NoiseSourceRawData noise_source_two_raw_data;
static int emb_corr_method_id;
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
	double act_device_version;
	int pp_enabled;
	uint16_t max_apt_failures_per_block;
	uint16_t max_rct_failures_per_block;

	printf("-------------------------------------------------------------------\n");
	printf("--- TectroLabs - swdiag - SwiftRNG diagnostics utility Ver 2.6  ---\n");
	printf("-------------------------------------------------------------------\n");
	printf("Searching for devices ------------------ ");

	setbuf(stdout, NULL);

	/* Initialize the context */
	status = swrngInitializeContext(&ctxt);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not initialize context\n");
		return(status);
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
			swrngDestroyContext(&ctxt);
			return status;
		}
		printf("Success\n");

		status = swrngSetPowerProfile(&ctxt, 9);
		if (status != SWRNG_SUCCESS) {
			printf("*** Could not set power profile, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return status;
		}

		status = swrngGetVersionNumber(&ctxt, &act_device_version);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return status;
		}

		if(act_device_version >= 1.2) {
			printf("\n------------- Disabling post processing for this device -----------");
			status = swrngDisablePostProcessing(&ctxt);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngDestroyContext(&ctxt);
				return status;
			}
		}

		status = swrngGetPostProcessingStatus(&ctxt, &pp_enabled);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return status;
		}
		if (pp_enabled) {
			printf("\n------------- Post processing enabled for this device -------------");
		} else {
			printf("\n------------- Post processing disabled for this device ------------");
			printf("\n------------- Tests will be performed on RAW byte stream ----------");
		}

		if (swrngGetEmbeddedCorrectionMethod(&ctxt, &emb_corr_method_id) != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return status;
		}

		switch (emb_corr_method_id) {
		case 0:
			printf("\n------------- Using no embedded correction algorithm --------------");
			break;
		case 1:
			printf("\n------------- Using embedded correction algorithm: Linear ---------");
			break;
		default:
			printf("\n------------- Unknown built-in correction algorithm ---------------");
		}

		if (act_device_version >= 2.0) {
			/* This feature was only implemented in devices with version 2.0 and up */
			printf("\n\n---------- Running device internal diagnostics  ----------  ");
			status = swrngRunDeviceDiagnostics(&ctxt);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngDestroyContext(&ctxt);
				return status;
			}
			printf("Success\n");
		}

		if (act_device_version >= 1.2) {
			/* These features were only implemented in devices with version 1.2 and up */
			printf("\n-------------- Verifying noise sources of the device --------------\n");

			printf("\n-------- Retrieving RAW data from noise sources  ---------  ");
			status = swrngGetRawDataBlock(&ctxt, &noise_source_one_raw_data, 0);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngDestroyContext(&ctxt);
				return status;
			}
			status = swrngGetRawDataBlock(&ctxt, &noise_source_two_raw_data, 1);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngDestroyContext(&ctxt);
				return status;
			}
			printf("Success\n");

			printf("----------  Inspecting RAW data of the noise sources  -------------\n");
			status = inspectRawData(&noise_source_one_raw_data, &noise_source_two_raw_data);
			if ( status != SWRNG_SUCCESS) {
				swrngDestroyContext(&ctxt);
				return status;
			}

			status = swrngGetFrequencyTables(&ctxt, &freq_tables);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngDestroyContext(&ctxt);
				return status;
			}
			printf("\n-------- Retrieving frequency table for noise source 1 ------------\n");
			status = print_frequency_table_summary(freq_tables.freqTable1);
			if ( status != SWRNG_SUCCESS) {
				swrngDestroyContext(&ctxt);
				return status;
			}
			printf("\n-------- Retrieving frequency table for noise source 2 ------------\n");
			status = print_frequency_table_summary(freq_tables.freqTable2);
			if ( status != SWRNG_SUCCESS) {
				swrngDestroyContext(&ctxt);
				return status;
			}
		}
		printf("\n-------- Running APT, RCT and device built-in tests ---------------\n");
		printf("Retrieving %d blocks of %6d random bytes each -------- ", NUM_BLOCKS, SAMPLES);
		/* APT and RCT tests are implemented in SwiftRNG Software API.
		Those tests are also embedded in SwiftRNG devices */
		for (l = 0; l < NUM_BLOCKS; l++) {
			/* Retrieve random bytes from device */
			status = swrngGetEntropy(&ctxt, rnd_buffer, SAMPLES);
			if ( status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngDestroyContext(&ctxt);
				return status;
			}
		}
		printf("Success\n");

		for (p = 0; p < 10; p++) {
			/* Power profiles are only implemented in 'SwiftRNG' models */
			if (!strcmp("SwiftRNG", dil.devInfoList[i].dm.value)) {
				printf("\nSetting power profiles to %1d ------------------------------- ", p);
				status = swrngSetPowerProfile(&ctxt, p);
				if (status != SWRNG_SUCCESS) {
					printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
					swrngDestroyContext(&ctxt);
					return status;
				}
				status = swrngGetEntropy(&ctxt, rnd_buffer, SAMPLES);
				if (status != SWRNG_SUCCESS) {
					printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
					swrngDestroyContext(&ctxt);
					return status;
				}
				printf("Success\n");
			}

			/* Retrieve random data for entropy score */
			printf("Entropy score for %8d bytes -------------------------- ", ENTROPY_SCORE_BYTES);
			for (k = 0; k < ENTROPY_SCORE_BYTES; k+=MAX_CHUNK_SIZE_BYTES)
			status = swrngGetEntropy(&ctxt, entropy_buffer + k, MAX_CHUNK_SIZE_BYTES);
			if (status != SWRNG_SUCCESS) {
				printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
				swrngDestroyContext(&ctxt);
				return status;
			}
			status = calculate_entropy_score();
			if (status != SWRNG_SUCCESS) {
				printf("*FAILED*");
				swrngDestroyContext(&ctxt);
				return status;
			}
			printf("\n");

			printf("-------- Running Chi-Square test (%1d times), step %1d of 10 ---------- \n", EXTLOOPS, p+1);
			for (l = 0; l < EXTLOOPS; l++) {
				status = run_chi_squire_test(l);
				if (status != SWRNG_SUCCESS) {
					swrngDestroyContext(&ctxt);
					return status;
				}
			}
		}

		swrngGetMaxAptFailuresPerBlock(&ctxt, &max_apt_failures_per_block);
		swrngGetMaxRctFailuresPerBlock(&ctxt, &max_rct_failures_per_block);
		printf("Maximum RCT/APT failures per block: ----------------------- %d/%d      ", (int)max_rct_failures_per_block, (int)max_apt_failures_per_block);
		if (max_rct_failures_per_block >= 3 || max_apt_failures_per_block >= 3) {
			printf("   (Warning)\n");
		} else {
			printf("(Acceptable)\n");
		}
		printf("Closing device -------------------------------------------- ");
		swrngDestroyContext(&ctxt);
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
	double val1 = (act_ones - expct_ones);
	double val2 = (act_zeros - expct_zeros);
	return val1 * val1 / expct_ones + val2 * val2 / expct_zeros;
}

/**
 * Inspect raw random data of the noise sources
 * @param NoiseSourceRawData *raw_data_1 - raw random data from noise source 1
 * @param NoiseSourceRawData *raw_data_2 - raw random data from noise source 2
 * @return int - status
 */
static int inspectRawData(NoiseSourceRawData *raw_data_1, NoiseSourceRawData *raw_data_2) {
	int i;
	int count1, count2;
	int min_freq1, min_freq2, max_freq1, max_freq2;
	uint16_t frequency_table_source1[256];
	uint16_t frequency_table_source2[256];
	int freq_range1;
	int freq_range2;

	/* Clear the frequency tables */
	for (i = 0; i < 256; i++) {
		frequency_table_source1[i] = 0;
		frequency_table_source2[i] = 0;
	}


	/* Fill the frequency tables */
	for (i = 0; i < 16000; i++) {
		frequency_table_source1[(uint8_t)raw_data_1->value[i]]++;
		frequency_table_source2[(uint8_t)raw_data_2->value[i]]++;
	}

	/* Calculate min and max values for each table */
	min_freq1 = frequency_table_source1[0];
	max_freq1 = frequency_table_source1[0];
	min_freq2 = frequency_table_source2[0];
	max_freq2 = frequency_table_source2[0];
	count1 = 0;
	count2 = 0;
	for (i = 0; i < 256; i++) {
		count1 += frequency_table_source1[i];
		count2 += frequency_table_source2[i];
		if (frequency_table_source1[i] > max_freq1) {
			max_freq1 = frequency_table_source1[i];
		}
		if (frequency_table_source1[i] < min_freq1) {
			min_freq1 = frequency_table_source1[i];
		}
		if (frequency_table_source2[i] > max_freq2) {
			max_freq2 = frequency_table_source2[i];
		}
		if (frequency_table_source2[i] < min_freq2) {
			min_freq2 = frequency_table_source2[i];
		}
	}

	printf("Frequency table source 1: min %d, max %d, samples %d", min_freq1, max_freq1, count1);
	freq_range1 = max_freq1 - min_freq1;
	if (freq_range1 > 200.0 || count1 != 16000) {
		printf(" *FAILED*\n");
		return -1;
	} else if (freq_range1 > 100.0) {
		printf(" *WARNING*\n");
	} else {
		printf(" (healthy)\n");
	}

	printf("Frequency table source 2: min %d, max %d, samples %d", min_freq2, max_freq2, count2);
	freq_range2 = max_freq2 - min_freq2;
	if (freq_range2 > 200.0 || count2 != 16000) {
		printf(" *FAILED*\n");
		return -1;
	} else if (freq_range2 > 100.0) {
		printf(" *WARNING*\n");
	} else {
		printf(" (healthy)\n");
	}

	return SWRNG_SUCCESS;
}

/**
 * Inspect and print retrieved frequency tables
 * @param uint16_t frequency_table - pointer to the frequency table
 * @return int - status
 */
int print_frequency_table_summary(uint16_t *frequency_table) {
	int k;
	int min_freq;
	int max_freq;
	int total_samples;
	int freq_range;

	min_freq = frequency_table[0];
	max_freq = frequency_table[0];
	total_samples = 0;
	for (k = 0; k < 256; k += 8) {
		printf("(%3d : %3d)  %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d \n", k, k+7,
				frequency_table[k+0],
				frequency_table[k+1],
				frequency_table[k+2],
				frequency_table[k+3],
				frequency_table[k+4],
				frequency_table[k+5],
				frequency_table[k+6],
				frequency_table[k+7]);
	}

	for (k = 0; k < 256; k++) {
		total_samples += frequency_table[k];
		if (frequency_table[k] > max_freq) {
			max_freq = frequency_table[k];
		}
		if (frequency_table[k] < min_freq) {
			min_freq = frequency_table[k];
		}
	}
	printf("-------------------------------------------------------------------\n");
	printf("Table summary: min %d, max %d, total samples %d", min_freq, max_freq, total_samples);
	freq_range = max_freq - min_freq;
	if (freq_range > 200.0 || total_samples != 16000) {
		printf(" *FAILED*\n");
		return -1;
	} else if (freq_range > 100.0) {
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
		entropy_freq_buff[i] = 0;
	}

	for(i = 0; i < ENTROPY_SCORE_BYTES; i++) {
		entropy_freq_buff[entropy_buffer[i]] ++;
	}

	score = 0;
	for(i = 0; i < 256; i++) {
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
int run_chi_squire_test(long idx) {
	long l;
	int status;

	printf("Average chi-square for test %3ld --------------------------- ", idx + 1);
	chi_square_sum = 0;
	for (l = 0; l < NUM_BLOCKS; l++) {
		/* Retrieve random bytes from device */
		status = swrngGetEntropy(&ctxt, rnd_buffer, SAMPLES);
		if ( status != SWRNG_SUCCESS) {
			printf("*FAILED*, err: %s\n", swrngGetLastErrorMessage(&ctxt));
			return status;
		}
		act_ones = 0;
		act_zeros = 0;
		int i = 0;
		for (i = 0; i < SAMPLES; i++) {
			chi_sqrd_count_bits(rnd_buffer[i], &act_ones, &act_zeros);
		}
		chi_square = chi_sqrd_calculate();
		chi_square_sum += chi_square;
	}
	double avgSquare = chi_square_sum/NUM_BLOCKS;
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
