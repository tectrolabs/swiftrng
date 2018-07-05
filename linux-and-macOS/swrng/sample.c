#include "stdafx.h"
/*
 * sample.c
 * Ver. 2.1
 *
 * This is a sample C program that demonstrates how to download random bytes
 * from a SwiftRNG using 'swrngapi' API for C language.
 *
 */

#include "swrngapi.h"

#define BYTE_BUFF_SIZE (10)
#define DEC_BUFF_SIZE (10)

unsigned char randombyte[BYTE_BUFF_SIZE]; // Allocate memory for random bytes
unsigned int randomint[DEC_BUFF_SIZE]; // Allocate memory for random integers

//
// Main entry
//
int main() {
	int i;
	double d;
	unsigned int ui;
	SwrngContext ctxt;
	printf("--------------------------------------------------------------------------\n");
	printf("--- Sample C program for downloading random bytes from SwiftRNG device ---\n");
	printf("--------------------------------------------------------------------------\n");

	// Initialize the context
	if (swrngInitializeContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		return(1);
	}

	// Open the first (device number 0 is the first device) SwiftRNG device if available
	if (swrngOpen(&ctxt, 0) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		return(1);
	}
	printf("\nSwiftRNG device open successfully\n\n");

	// Download random bytes from device
	if (swrngGetEntropy(&ctxt, randombyte, BYTE_BUFF_SIZE) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngClose(&ctxt);
		return (1);
	}

	printf("*** Generating %d random bytes ***\n", BYTE_BUFF_SIZE);
	// Print random bytes
	for (i = 0; i < BYTE_BUFF_SIZE; i++) {
		printf("random byte %d -> %d\n", i, (int)randombyte[i]);
	}

	// Download random integers from device
	if (swrngGetEntropy(&ctxt, (unsigned char*)randomint, DEC_BUFF_SIZE * sizeof(unsigned int)) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngClose(&ctxt);
		return (1);
	}

	printf("\n*** Generating %d random numbers between 0 and 1 with 5 decimals  ***\n", DEC_BUFF_SIZE);
	// Print random bytes
	for (i = 0; i < DEC_BUFF_SIZE; i++) {
		ui = randomint[i] % 99999;
		d = (double)ui / 100000.0;
		printf("random number -> %lf\n", d);
	}

	printf("\n");
	swrngClose(&ctxt);
	return (0);

}
