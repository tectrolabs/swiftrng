/*
 * bitcount.c
 * Ver. 3.4
 *
 * A C program for counting '1' and '0' bits retrieved from SwiftRNG device or from a file
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <swrngapi.h>

#define BLOCK_SIZE (16000)

/**
 * Local variables
 */
static uint8_t buffer[BLOCK_SIZE];
static uint8_t byte_value_to_one_bit_count[256];
static long long total_ones;
static long long total_zeros;
static long long total_bits;
static long total_blocks;
static double arithmetic_zero_mean;
static int device_num = 0;
static int pp_method_id = -1;
static char emb_corr_method_char[32];
static char pp_method_char[256];

/**
 * Local functions
 */
static void count_one_bits(uint8_t byte, uint8_t *ones);
static void init_stats_data(void);
static void display_usage(void);
static int count_bits_from_device(void);
static int count_bits_from_file(char *fileName);
static void print_final_stats(void);

/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {
	strcpy(pp_method_char, "SHA256");
	strcpy(emb_corr_method_char, "none");

	printf("----------------------------------------------------------------------------\n");
	printf("---  A program for counting bits retrieved from SwiftRNG or from a file  ---\n");
	printf("----------------------------------------------------------------------------\n");

	setbuf(stdout, NULL);

	if (argc < 2) {
		display_usage();
		return 1;
	}

	if (strcmp("-fn", argv[1]) == 0) {
		if (argc < 3) {
			printf("File name not provided\n");
			display_usage();
			return 1;
		}
		return count_bits_from_file(argv[2]);
	}

	total_blocks = atol(argv[1]);
	if (total_blocks <= 0) {
		printf("Total blocks parameter invalid\n");
		display_usage();
		return 1;
	}

	if (argc > 2) {
		device_num = atoi(argv[2]);
		if (device_num < 0) {
			printf("Device number parameter invalid\n");
			display_usage();
			return 1;
		}
		if (argc > 3) {
			strcpy(pp_method_char, argv[3]);
			if (!strcmp("SHA256", pp_method_char)) {
				pp_method_id = 0;
			} else if (!strcmp("SHA512", pp_method_char)) {
				pp_method_id = 2;
			} else if (!strcmp("xorshift64", pp_method_char)) {
				pp_method_id = 1;
			} else {
				printf("Post processing %s not supported\n", pp_method_char);
				return 1;
			}
		}
	}

	return count_bits_from_device();
}

/**
 * Count number of 'ones' for provided byte value
 *
 * @param uint8_t byte - byte value to process
 * @param uint8_t *ones - pointer to the 'ones' counter
 */

static void count_one_bits(uint8_t byte, uint8_t *ones) {
	uint8_t val = byte;
	for (int i = 0; i < 8; i++) {
		if (val & 0x1) {
			(*ones)++;
		}
		val >>= 1;
	}
}

/**
 * Initialize statistics
 */
static void init_stats_data(void) {
	uint8_t oneCounter;
	for (int i = 0; i < 256; i++) {
		oneCounter = 0;
		count_one_bits((uint8_t)i, &oneCounter);
		byte_value_to_one_bit_count[i] = oneCounter;
	}
}

/**
 * Print usage message
 */
static void display_usage(void) {
	printf("---------------------------------------------------------------------------------\n");
	printf("--- A program for counting 1's and 0's bits retrieved from a SwiftRNG device  ---\n");
	printf("---------------------------------------------------------------------------------\n");
	printf("Usage: bitcount <total blocks> [device number] [SHA256, SHA512 or xorshift64]\n");
	printf("Usage: bitcount -fn <file name>\n");
	printf("Note: One block equals to 16000 bytes\n");
	printf("Example 1: using 160000 bytes from the first SwiftRNG device: bitcount 10 0\n");
	printf("Example 2: reading bytes from a data file: bitcount -fn binary-data-file.bin\n");
}

/**
 * Count all the bits retrieved from a SwiftRNG device
 *
 * @return int 0 - successful or error code
 */
static int count_bits_from_device(void) {

	SwrngContext ctxt;
	int act_pp_method_id;
	int pp_status;
	int emb_corr_method_id;

	/* Initialize the context */
	if (swrngInitializeContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		swrngDestroyContext(&ctxt);
		return 1;
	}

	init_stats_data();

	/* Open SwiftRNG device if available */
	if (swrngOpen(&ctxt, device_num) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngDestroyContext(&ctxt);
		return 1;
	}

	/* Set post processing method if provided */
	if (pp_method_id != -1 && swrngEnablePostProcessing(&ctxt, pp_method_id) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngDestroyContext(&ctxt);
		return 1;
	}
	printf("\nSwiftRNG device number %d open successfully\n\n", device_num);

	if (swrngGetPostProcessingStatus(&ctxt, &pp_status) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngDestroyContext(&ctxt);
		return 1;
	}

	if (pp_status == 0) {
		strcpy(pp_method_char, "none");
	} else {
		if (swrngGetPostProcessingMethod(&ctxt, &act_pp_method_id) != SWRNG_SUCCESS) {
			printf("%s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return 1;
		}
		switch (act_pp_method_id) {
		case 0:
			strcpy(pp_method_char, "SHA256");
			break;
		case 1:
			strcpy(pp_method_char, "xorshift64");
			break;
		case 2:
			strcpy(pp_method_char, "SHA512");
			break;
		default:
			strcpy(pp_method_char, "*unknown*");
			break;
		}

	}

	if (swrngGetEmbeddedCorrectionMethod(&ctxt, &emb_corr_method_id) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngDestroyContext(&ctxt);
		return 1;
	}

	switch(emb_corr_method_id) {
	case 0:
		strcpy(emb_corr_method_char, "none");
		break;
	case 1:
		strcpy(emb_corr_method_char, "Linear");
		break;
	default:
		strcpy(emb_corr_method_char, "*unknown*");
	}

	printf("*** retrieving random bytes and counting bits, post-processing: %s, embedded correction: %s ***\n", pp_method_char, emb_corr_method_char);
	total_ones = 0;
	total_zeros = 0;
	for (long l = 0; l < total_blocks; l++) {
		if (swrngGetEntropy(&ctxt, buffer, BLOCK_SIZE) != SWRNG_SUCCESS) {
			printf("Could not retrieve entropy from device. %s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return 1;
		}
		for(int i = 0; i < BLOCK_SIZE; i++) {
			int valPos = buffer[i];
			int oneCounter = byte_value_to_one_bit_count[valPos];
			total_ones += oneCounter;
			total_zeros += (8 - oneCounter);
		}
	}
	print_final_stats();
	swrngDestroyContext(&ctxt);
	return 0;
}

/**
 * Count all the bits retrieved from a file
 *
 * @param char *fileName - pointer to file name
 * @return int 0 - successful or error code
 */
static int count_bits_from_file(char *fileName) {
	FILE *file;
	size_t bytesRead;

	init_stats_data();
	file = NULL;

	file = fopen(fileName, "rb");
	if (file == NULL) {
		printf("File %s not found\n", fileName);
		display_usage();
		return 1;
	}

	printf("*** reading bytes and counting bits from file: %s ***\n", fileName);

	total_ones = 0;
	total_zeros = 0;


	while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
		for (size_t i = 0; i < bytesRead; i++) {
			int valPos = buffer[i];
			int oneCounter = byte_value_to_one_bit_count[valPos];
			total_ones += oneCounter;
			total_zeros += (8 - oneCounter);
		}
	}
	print_final_stats();
	fclose(file);
	return 0;
}

/**
 *  Print the test results
 *
 */
static void print_final_stats(void) {
	total_bits = total_zeros + total_ones;
	arithmetic_zero_mean = (double)total_zeros / (double)total_bits;
	printf("retrieved %lld total bits, 0's bit count: %lld, 1's bit count: %lld, 0's arithmetic mean: %.10g\n",
			total_bits, total_zeros, total_ones, arithmetic_zero_mean);
	printf("\n");
}
