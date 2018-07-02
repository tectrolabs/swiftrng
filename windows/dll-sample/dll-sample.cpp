#include "stdafx.h"
/*
 * dll-sample.cpp
 *
 * This is a sample C/C++ program that demonstrates how to download random bytes
 * from Hardware Random Number Generator SwiftRNG using 'SwingRNG.dll'.
 *
 */

#include "SwiftRNG.h"

//
// Main entry
//
int main() {
	int i;
	printf("---------------------------------------------------------------------------------------------\n"); 
	printf("--- Sample C program for downloading random bytes from SwiftRNG device using SwiftRNG.dll ---\n"); 
	printf("---------------------------------------------------------------------------------------------\n"); 

	// Open the first (device number 0 is the first device) SwiftRNG device if available 
	if (swftOpen(0) != 0) {
		printf("%s\n", swftGetLastErrorMessage());
		return(1);
	}
	printf("\nSwiftRNG device open successfully\n\n");
	printf("\nGenerating 10 random numbers between values 1 and 5\n\n");
	
	// Print random bytes
	for (i = 1; i <= 10; i++) {
		int rnd = swftGetEntropyByte();
		if (rnd > 255) {
			printf("%s\n", swftGetLastErrorMessage());
			return(1);
		}
		printf("random number %d: %d\n", i, (int)((rnd + 1)) % 6);
	}


	printf("\n");
	swftClose();
	return (0);

}

