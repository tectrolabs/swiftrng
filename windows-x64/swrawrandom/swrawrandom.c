/*
 * swrawrandom.c
 * Ver. 2.3
 *
 * A C program for retrieving raw (unprocessed) random bytes from SwiftRNG noise sources.
 *
 */

#include <swrngapi.h>
#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE (16000)

static NoiseSourceRawData noise_source_one_raw_data;

/* File name for recording the random bytes (a command line argument) */
static char *file_path_name = NULL;

/* Output file handle */
static FILE *p_output_file = NULL;


/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main(int argc, char **argv) {
	int device_num, noise_source_num;
	long total_blocks, l;
	SwrngContext ctxt;

	printf("------------------------------------------------------------------------------\n");
	printf("--- A program for retrieving raw random bytes from SwiftRNG noise sources. ---\n");
	printf("---- No data alteration, verification or quality tests will be performed. ----\n");
	printf("------------------------------------------------------------------------------\n");

	setbuf(stdout, NULL);

	if (argc == 5) {
		total_blocks = atol(argv[1]);
		device_num = atol(argv[2]);
		noise_source_num = atol(argv[3]);
		file_path_name = argv[4];
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

	if (noise_source_num != 0 && noise_source_num != 1) {
		printf("Invalid noise source number specified\n");
		return(1);
	}

	/* Initialize the context */
	if (swrngInitializeContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		return(1);
	}

	/* Open SwiftRNG device if available */
	if (swrngOpen(&ctxt, device_num) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngDestroyContext(&ctxt);
		return(1);
	}

	printf("\nSwiftRNG device number %d open successfully\n\n", device_num);

	p_output_file = fopen(file_path_name, "wb");

	if (p_output_file == NULL) {
		fprintf(stderr, "Cannot open file: %s in write mode\n", file_path_name);
		swrngDestroyContext(&ctxt);
		return -1;
	}

	printf("*** retrieving raw random bytes from noise source %d ***\n", noise_source_num);
	for (l = 0; l < total_blocks; l++) {
		if (swrngGetRawDataBlock(&ctxt, &noise_source_one_raw_data, noise_source_num) != SWRNG_SUCCESS) {
			printf("%s\n", swrngGetLastErrorMessage(&ctxt));
			swrngDestroyContext(&ctxt);
			return (1);
		}
		fwrite(noise_source_one_raw_data.value, 1, BLOCK_SIZE, p_output_file);
	}

	swrngDestroyContext(&ctxt);

	if (p_output_file != NULL) {
		fclose(p_output_file);
		p_output_file = NULL;
	}
	printf("Completed\n");

	return (0);
}
