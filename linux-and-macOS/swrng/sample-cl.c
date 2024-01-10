/*
 * sample-cl.c
 * Ver. 2.8
 *
 * @brief This is a sample C program that demonstrates how to retrieve random bytes from
 * a cluster of two SwiftRNG devices using 'swrng-cl-api' API for C language.
 *
 */
#include <swrng-cl-api.h>

#define BYTE_BUFF_SIZE (10)
#define DEC_BUFF_SIZE (10)

/* Allocate memory for random bytes */
unsigned char random_byte[BYTE_BUFF_SIZE];

/* Allocate memory for random integers */
unsigned int random_int[DEC_BUFF_SIZE];

/****************
 * Main entry
 ****************/
int main() {
	int i;
	double d;
	unsigned int ui;
	SwrngCLContext ctxt;

	printf("-------------------------------------------------------------------------------------------\n");
	printf("--- Sample C program for retrieving random bytes from a cluster of two SwiftRNG device ----\n");
	printf("-------------------------------------------------------------------------------------------\n");

	/* Initialize the context */
	if (swrngInitializeCLContext(&ctxt) != SWRNG_SUCCESS) {
		printf("Could not initialize context\n");
		return 1;
	}

	/* Open the cluster if any SwiftRNG device if available */
	if (swrngCLOpen(&ctxt, 2) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetCLLastErrorMessage(&ctxt));
		return 1;
	}

	printf("\nSwiftRNG cluster of %d devices open successfully\n\n", swrngGetCLSize(&ctxt));


	/* Retrieve random bytes from cluster */
	if (swrngGetCLEntropy(&ctxt, random_byte, BYTE_BUFF_SIZE) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return 1;
	}

	printf("*** Generating %d random bytes ***\n", BYTE_BUFF_SIZE);
	/* Print random bytes */
	for (i = 0; i < BYTE_BUFF_SIZE; i++) {
		printf("random byte %d -> %d\n", i, (int)random_byte[i]);
	}

	/* Retrieve random integers from the cluster */
	if (swrngGetCLEntropy(&ctxt, (unsigned char*)random_int, DEC_BUFF_SIZE * sizeof(unsigned int)) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetCLLastErrorMessage(&ctxt));
		swrngCLClose(&ctxt);
		return 1;
	}

	printf("\n*** Generating %d random numbers between 0 and 1 with 5 decimals  ***\n", DEC_BUFF_SIZE);
	/* Print random bytes */
	for (i = 0; i < DEC_BUFF_SIZE; i++) {
		ui = random_int[i] % 99999;
		d = (double)ui / 100000.0;
		printf("random number -> %lf\n", d);
	}


	printf("\n");
	swrngCLClose(&ctxt);
	return 0;

}
