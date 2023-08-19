/*
 * sample++.cpp
 * Ver. 1.0
 *
 * A sample C++ program that demonstrates how to retrieve random bytes
 * from a SwiftRNG device using SwiftRNG C++ API.
 *
 *
 */

#include <SwiftRngApi.h>
#include <string>
#include <iostream>


using namespace swiftrng;


/**
 * Main entry
 * @return int 0 - successful or error code
 */
int main() {

	SwiftRngApi api;
	unsigned char entropyBytes [10];

	std::cout << "---------------------------------------------------------------------------" << std::endl;
	std::cout << "--- Sample C++ program for retrieving random bytes from SwiftRNG device ---" << std::endl;
	std::cout << "---------------------------------------------------------------------------" << std::endl;


	if (api.open(0)) {
		std::cerr << api.get_last_error_log() << std::endl;
		return -1;
	}

	if (api.get_entropy(entropyBytes, sizeof(entropyBytes))) {
		std::cerr << "Could not retrieve entropy from SwiftRNG device. " << api.get_last_error_log() << std::endl;
		api.close();
		return -1;
	}

	for (int i = 0; i < (int)sizeof(entropyBytes); ++i) {
		std::cout << "entropy byte " << i+1 << ": " << (int)entropyBytes[i] << std::endl;
	}

	api.close();
	return 0;
}
