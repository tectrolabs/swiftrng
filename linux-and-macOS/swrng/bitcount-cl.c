/*
 * bitcount-cl.c
 * Ver. 2.3
 *
 * A C program for counting '1' and '0' bits retrieved from a SwiftRNG device cluster
 * using default configuration.
 *
 */

#include <swrng-cl-api.h>

#define BLOCK_SIZE (16000)

static uint8_t buffer[BLOCK_SIZE];
static uint8_t byteValueToOneBitCount[256];


/**
 * Local functions
 */
static void count_one_bits(uint8_t byte, uint8_t *ones);
static void init_stats_data(void);


/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {
	int clusterSize;
	long totalBlocks;
	SwrngCLContext ctxt;
	long long totalOnes;
	long long totalZeros;
	long long totalBits;
	double arithmeticZeroMean;
	int postProcessingMethod = -1;
	char postProcessingMethodStr[256];

	printf("---------------------------------------------------------------------------------\n");
	printf("--- A program for counting 1's and 0's bits retrieved from a SwiftRNG cluster ---\n");
	printf("---------------------------------------------------------------------------------\n");

	setbuf(stdout, NULL);
	strcpy(postProcessingMethodStr, "default");

	if (argc > 2) {
		totalBlocks = atol(argv[1]);
		if (totalBlocks <= 0) {
			printf("Total blocks parameter invalid\n");
			return 1;
		}

		clusterSize = atoi(argv[2]);
		if (argc > 3) {
			strcpy(postProcessingMethodStr, argv[3]);
			if (!strcmp("SHA256", postProcessingMethodStr)) {
				postProcessingMethod = 0;
			} else if (!strcmp("SHA512", postProcessingMethodStr)) {
				postProcessingMethod = 2;
			} else if (!strcmp("xorshift64", postProcessingMethodStr)) {
				postProcessingMethod = 1;
			} else {
				printf("Post processing %s not supported\n", postProcessingMethodStr);
				return 1;
			}
		}
	} else if (argc > 1) {
		totalBlocks = atol(argv[1]);
		clusterSize = 0;
	} else {
		printf("Usage: bitcount-cl <number of blocks> <cluster size> [SHA256, SHA512 or xorshift64]\n");
		printf("Note: One block equals to 16000 bytes\n");
		return 1;
	}

	/* Initialize the context */
	if (swrngInitializeCLContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		return 1;
	}

	init_stats_data();

	/* Open the cluster if any SwiftRNG device if available */
	if (swrngCLOpen(&ctxt, clusterSize) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return 1;
	}

	/* Set post processing method if provided */
	if (postProcessingMethod != -1 && swrngEnableCLPostProcessing(&ctxt, postProcessingMethod) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return 1;
	}

	printf("\nSwiftRNG cluster of %d devices open successfully\n\n", swrngGetCLSize(&ctxt));

	printf("*** retrieving random bytes and counting bits using post processing method: %s ***\n", postProcessingMethodStr);
	totalOnes = 0;
	totalZeros = 0;
	for (long l = 0; l < totalBlocks; l++) {
		if (swrngGetCLEntropy(&ctxt, buffer, BLOCK_SIZE) != SWRNG_SUCCESS) {
			printf("Could not retrieve entropy from device cluster. %s\n", swrngGetCLLastErrorMessage(&ctxt));
			swrngCLClose(&ctxt);
			return 1;
		}
		for(int i = 0; i < BLOCK_SIZE; i++) {
			int valPos = buffer[i];
			int oneCounter = byteValueToOneBitCount[valPos];
			totalOnes += oneCounter;
			totalZeros += (8 - oneCounter);
		}
	}
	totalBits = (long long)totalBlocks * BLOCK_SIZE * 8;
	arithmeticZeroMean = (double)totalZeros / (double)totalBits;
	printf("retrieved %lld total bits, 0's bit count: %lld, 1's bit count: %lld, 0's arithmetic mean: %.10g\n",
			totalBits, totalZeros, totalOnes, arithmeticZeroMean);

	printf("\n");
	swrngCLClose(&ctxt);
	return 0;

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
 * Initialize statistical data
 */
static void init_stats_data(void) {
	uint8_t oneCounter;
	for (int i = 0; i < 256; i++) {
		oneCounter = 0;
		count_one_bits((uint8_t)i, &oneCounter);
		byteValueToOneBitCount[i] = oneCounter;
	}
}
