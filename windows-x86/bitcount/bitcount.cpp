#include "stdafx.h"
/*
 * bitcount.c
 * Ver. 1.9
 *
 * A C program for counting '1' and '0' bits downloaded from SwiftRNG device
 *
 */

#include "swrngapi.h"

#define BLOCK_SIZE (16000)

static uint8_t buffer[BLOCK_SIZE];
static uint8_t byteValueToOneBitCount[256];

static void count_one_bits(uint8_t byte, uint8_t *ones);
static void initializeStatData(void);


/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {
	int i, deviceNum;
	long totalBlocks, l;
	SwrngContext ctxt;
	long long totalOnes;
	long long totalZeros;
	long long totalBits;
	double arithmeticZeroMean;
	int postProcessingMethod = -1;
	char postProcessingMethodStr[256];
	char embeddedCorrectionMethodStr[32];
	int actualPostProcessingMethodId;
	int postProcessingStatus;
	int embeddedCorrectionMethodId;
#ifdef _WIN32
	strcpy_s(postProcessingMethodStr, "SHA256");
	strcpy_s(embeddedCorrectionMethodStr, "none");
#else
	strcpy(postProcessingMethodStr, "SHA256");
	strcpy(embeddedCorrectionMethodStr, "none");
#endif

	printf("------------------------------------------------------------------------------\n");
	printf("--- A program for counting 1's and 0's bits retrieved from SwiftRNG device ---\n");
	printf("------------------------------------------------------------------------------\n");

	setbuf(stdout, NULL);

	if (argc > 2) {
		totalBlocks = atol(argv[1]);
		deviceNum = atol(argv[2]);
		if (argc > 3) {
#ifdef _WIN32
			strcpy_s(postProcessingMethodStr, argv[3]);
#else
			strcpy(postProcessingMethodStr, argv[3]);
#endif
			if (!strcmp("SHA256", postProcessingMethodStr)) {
				postProcessingMethod = 0;
			} else if (!strcmp("SHA512", postProcessingMethodStr)) {
				postProcessingMethod = 2;
			} else if (!strcmp("xorshift64", postProcessingMethodStr)) {
				postProcessingMethod = 1;
			} else {
				printf("Post processing %s not supported\n", postProcessingMethodStr);
				return(1);
			}
		}
	} else if (argc > 1) {
		totalBlocks = atol(argv[1]);
		deviceNum = 0;
	} else {
		printf("Usage: bitcount <number of blocks> <device number> [SHA256, SHA512 or xorshift64]\n");
		printf("Note: One block equals to 16000 bytes\n");
		return(1);
	}


	// Initialize the context
	if (swrngInitializeContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		return(1);
	}

	initializeStatData();

	// Open SwiftRNG device if available
	if (swrngOpen(&ctxt, deviceNum) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		return(1);
	}

	// Set post processing method if provided
	if (postProcessingMethod != -1) {
		if (swrngEnablePostProcessing(&ctxt, postProcessingMethod) != SWRNG_SUCCESS) {
			printf("%s\n", swrngGetLastErrorMessage(&ctxt));
			return(1);
		}
	}
	printf("\nSwiftRNG device number %d open successfully\n\n", deviceNum);

	if (swrngGetPostProcessingStatus(&ctxt, &postProcessingStatus) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		return(1);
	}

	if (postProcessingStatus == 0) {
#ifdef _WIN32
		strcpy_s(postProcessingMethodStr, "none");
#else
		strcpy(postProcessingMethodStr, "none");
#endif
	} else {
		if (swrngGetPostProcessingMethod(&ctxt, &actualPostProcessingMethodId) != SWRNG_SUCCESS) {
			printf("%s\n", swrngGetLastErrorMessage(&ctxt));
			return(1);
		}
		switch (actualPostProcessingMethodId) {
		case 0:
#ifdef _WIN32
			strcpy_s(postProcessingMethodStr, "SHA256");
#else
			strcpy(postProcessingMethodStr, "SHA256");
#endif
			break;
		case 1:
#ifdef _WIN32
			strcpy_s(postProcessingMethodStr, "xorshift64");
#else
			strcpy(postProcessingMethodStr, "xorshift64");
#endif
			break;
		case 2:
#ifdef _WIN32
			strcpy_s(postProcessingMethodStr, "SHA512");
#else
			strcpy(postProcessingMethodStr, "SHA512");
#endif
			break;
		default:
#ifdef _WIN32
			strcpy_s(postProcessingMethodStr, "*unknown*");
#else
			strcpy(postProcessingMethodStr, "*unknown*");
#endif
			break;
		}

	}

	if (swrngGetEmbeddedCorrectionMethod(&ctxt, &embeddedCorrectionMethodId) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		return(1);
	}

	switch(embeddedCorrectionMethodId) {
	case 0:
		#ifdef _WIN32
			strcpy_s(embeddedCorrectionMethodStr, "none");
		#else
			strcpy(embeddedCorrectionMethodStr, "none");
		#endif
			break;
	case 1:
		#ifdef _WIN32
				strcpy_s(embeddedCorrectionMethodStr, "Linear");
		#else
				strcpy(embeddedCorrectionMethodStr, "Linear");
		#endif
			break;
	default:
		#ifdef _WIN32
			strcpy_s(embeddedCorrectionMethodStr, "*unknown*");
		#else
			strcpy(embeddedCorrectionMethodStr, "*unknown*");
		#endif
	}

	printf("*** downloading random bytes and counting bits, post processing: %s, embedded correction: %s ***\n", postProcessingMethodStr, embeddedCorrectionMethodStr);
	totalOnes = 0;
	totalZeros = 0;
	for (l = 0; l < totalBlocks; l++) {
		if (swrngGetEntropy(&ctxt, buffer, BLOCK_SIZE) != SWRNG_SUCCESS) {
			printf("%s\n", swrngGetLastErrorMessage(&ctxt));
			swrngClose(&ctxt);
			return (1);
		}
		for(i = 0; i < BLOCK_SIZE; i++) {
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
	swrngClose(&ctxt);
	return (0);

}

/**
 * Count number of 'ones' for provided byte value
 *
 * @param uint8_t byte - byte value to process
 * @param uint8_t *ones - pointer to the 'ones' counter
 */

void count_one_bits(uint8_t byte, uint8_t *ones) {
	int i;
	uint8_t val = byte;
	for (i = 0; i < 8; i++) {
		if (val & 0x1) {
			(*ones)++;
		}
		val >>= 1;
	}
}

/**
 * Initialize statistical data
 */
static void initializeStatData(void) {
	uint8_t oneCounter;
	int i;
	for (i = 0; i < 256; i++) {
		oneCounter = 0;
		count_one_bits(i, &oneCounter);
		byteValueToOneBitCount[i] = oneCounter;
	}
}
