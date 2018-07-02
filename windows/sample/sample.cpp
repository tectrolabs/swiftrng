#include "stdafx.h"
/*
 * sample.c
 *
 * This is a sample C program that demonstrates how to download random bytes
 * from Hardware Random Number Generator SwiftRNG using 'swrngapi' API for C language.
 *
 */

#include "swrngapi.h"

#define BYTE_BUFF_SIZE (10)
#define DEC_BUFF_SIZE (10)

unsigned char randombyte[BYTE_BUFF_SIZE + 1]; // Allocate memory for random bytes, include an additional element for SwiftRNG status byte
unsigned int randomint[DEC_BUFF_SIZE + 1]; // Allocate memory for random integers, include an additional element for SwiftRNG status byte

//
// Main entry
//
int main() {
	int i;
	double d;
	unsigned int ui;
	printf("--------------------------------------------------------------------------\n"); 
	printf("--- Sample C program for downloading random bytes from SwiftRNG device ---\n"); 
	printf("--------------------------------------------------------------------------\n"); 

	// Open the first (device number 0 is the first device) SwiftRNG device if available 
	if (swrngOpen(0) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage());
		return(1);
	}
	printf("\nSwiftRNG device open successfully\n\n");
	
	// Download random bytes from device
	if (swrngGetEntropy(randombyte, BYTE_BUFF_SIZE) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage());
		swrngClose();
		return (1);
	}

	printf("*** Generating %d random bytes ***\n", BYTE_BUFF_SIZE);
	// Print random bytes
	for (i = 0; i < BYTE_BUFF_SIZE; i++) {
		printf("random byte %d -> %d\n", i, (int)randombyte[i]);
	}

	// Download random integers from device
	if (swrngGetEntropy((unsigned char*)randomint, DEC_BUFF_SIZE * sizeof(unsigned int)) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage());
		swrngClose();
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
	swrngClose();
	return (0);

}
