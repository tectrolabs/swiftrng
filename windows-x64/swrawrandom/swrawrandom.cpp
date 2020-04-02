#include "stdafx.h"
/*
 * swrawrandom.cpp
 * Ver. 2.0
 *
 * A C program for retrieving raw (unprocessed) random bytes from SwiftRNG noise sources.
 *
 */

#include "swrngapi.h"

#define BLOCK_SIZE (16000)

static NoiseSourceRawData noiseSourceOneRawData;
static char *filePathName = NULL; // File name for recording the random bytes (a command line argument)
static FILE *pOutputFile = NULL; // Output file handle


/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {
	int deviceNum, noiseSourceNum;
	long totalBlocks, l;
	SwrngContext ctxt;

	printf("------------------------------------------------------------------------------\n");
	printf("--- A program for retrieving raw random bytes from SwiftRNG noise sources. ---\n");
	printf("---- No data alteration, verification or quality tests will be performed. ----\n");
	printf("------------------------------------------------------------------------------\n");

	setbuf(stdout, NULL);

	if (argc == 5) {
		totalBlocks = atol(argv[1]);
		deviceNum = atol(argv[2]);
		noiseSourceNum = atol(argv[3]);
		filePathName = argv[4];
	} else {
		printf("Usage: swrawrandom <number of blocks> <device> <noise source> <file>\n");
		printf("Note: One block equals to 16000 bytes\n");
		printf("      <device> - SwiftRNG device number, 0 - for first device \n");
		printf("      <noise source> - valid values: 0 (first) or 1 (second)\n");
		printf("      <file> - file for storing retrieved random bytes\n");
		printf("Example: swrawrandom 10 0 0 ns0.bin\n");
		printf("Example: swrawrandom 10 0 1 ns1.bin\n");
		return(1);
	}

	if (noiseSourceNum != 0 && noiseSourceNum != 1) {
		printf("Invalid noise source number specified\n");
		return(1);
	}

	// Initialize the context
	if (swrngInitializeContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		return(1);
	}

	// Open SwiftRNG device if available
	if (swrngOpen(&ctxt, deviceNum) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		return(1);
	}

	printf("\nSwiftRNG device number %d open successfully\n\n", deviceNum);

	pOutputFile = fopen(filePathName, "wb");

	if (pOutputFile == NULL) {
		fprintf(stderr, "Cannot open file: %s in write mode\n", filePathName);
		swrngClose(&ctxt);
		return -1;
	}

	printf("*** retrieving raw random bytes from noise source %d ***\n", noiseSourceNum);
	for (l = 0; l < totalBlocks; l++) {
		if (swrngGetRawDataBlock(&ctxt, &noiseSourceOneRawData, noiseSourceNum) != SWRNG_SUCCESS) {
			printf("%s\n", swrngGetLastErrorMessage(&ctxt));
			swrngClose(&ctxt);
			return (1);
		}
		fwrite(noiseSourceOneRawData.value, 1, BLOCK_SIZE, pOutputFile);
	}

	swrngClose(&ctxt);

	if (pOutputFile != NULL) {
		fclose(pOutputFile);
		pOutputFile = NULL;
	}
	printf("Completed\n");

	return (0);
}
