/*
 * dll-sample.c
 *
 * This is a sample C/C++ program that demonstrates how to download random bytes
 * from Hardware Random Number Generator SwiftRNG using 'SwingRNG.dll' API.
 *
 */

#include <SwiftRNG.h>

char errBuff[256];

//
// Main entry
//
int main() {
	int i;
	printf("---------------------------------------------------------------------------------------------\n"); 
	printf("- Sample C program for downloading random bytes from SwiftRNG device using SwiftRNG.dll API -\n"); 
	printf("---------------------------------------------------------------------------------------------\n"); 

	printf("\nGenerating 10 random numbers between values 0 and 255\n\n");

	for (i = 1; i <= 10; i++) {
		int rnd = swftGetEntropyByteSynchronized();
		if (rnd > 255) {
			printf("getEntropyByteSynchronized() failed\n");
			return(1);
		}
		printf("random number using thread-safe method %d: %d\n", i, (int)(rnd));
	}
	printf("\n");

	return (0);

}

