#include "stdafx.h"

/*
 * swrng.cpp
 * Ver. 3.0
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for interacting with the hardware random data generator device SwiftRNG for the purpose of
 downloading true random bytes.

 This program requires the libusb-1.0 library when communicating with any SwiftRNG device. Please read the provided
 documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General 
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with TectroLabs devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "swrng.h"

#ifdef _WIN32
#define atoll(S) _atoi64(S)
#endif

/**
 * Display information about SwifrRNG devices
 *
 * @return int - 0 when run successfully
 */
int displayDevices() {
	DeviceInfoList dil;
	int i;

	int status = swrngGetDeviceList(&ctxt, &dil);

	if (status != 0) {
		fprintf(stderr, "Could not generate device info list, status: %d\n",
				status);
		return status;
	}

	if (dil.numDevs > 0) {
		printf("\n");
		for (i = 0; i < dil.numDevs; i++) {
			printf("{");
			printf("DevNum=%d", i);
			printf(" DevModel=%s", dil.devInfoList[i].dm.value);
			printf(" DevVer=%s", dil.devInfoList[i].dv.value);
			printf(" DevS/N=%s", dil.devInfoList[i].sn.value);
			printf("}\n");
		}
	} else {
		fprintf(stderr, "There are currently no SwiftRNG devices available\n");
	}
	return 0;
}

/**
 * Display usage message
 *
 */
void displayUsage() {
	printf("*********************************************************************************\n");
	printf("                   TectroLabs - swrng - download utility Ver 3.0  \n");
	printf("*********************************************************************************\n");
	printf("NAME\n");
	printf("     swrng  - True Random Number Generator SwiftRNG download \n");
	printf("              utility \n");
	printf("SYNOPSIS\n");
	printf("     swrng -ld --list-devices | -dd --download-data [options] \n");
#ifdef __linux__
	printf("           -fep --feed-entropy-pool\n");
#endif
	printf("\n");
	printf("DESCRIPTION\n");
	printf("     Swrng downloads random bytes from Hardware (True) \n");
	printf(
			"     Random Number Generator SwiftRNG device and writes them to a \n");
	printf("     binary file.\n");
	printf("\n");
	printf("FUNCTION LETTERS\n");
	printf("     Main operation mode:\n");
	printf("\n");
	printf("     -ld, --list-devices\n");
	printf(
			"           list all available (not currently in use) SwiftRNG devices\n");
	printf("\n");
	printf("     -dd, --download-data\n");
	printf(
			"           download random bytes from the SwiftRNG device and store \n");
	printf("           them into a file\n");
#ifdef __linux__
	printf("     -fep, --feed-entropy-pool\n");
	printf("           Feed /dev/random Kernel entropy pool with true random numbers.\n");
	printf("           It will continuously maintain /dev/random pool filled up with\n");
	printf("           true random bytes downloaded from the first device\n");
	printf("\n");
#endif
	printf("\n");
	printf("OPTIONS\n");
	printf("     Operation modifiers:\n");
	printf("\n");
	printf("     -fn FILE, --file-name FILE\n");
	printf(
			"           a FILE name for storing random data. Use STDOUT to send bytes\n");
	printf("           to standard output\n");
	printf("\n");
	printf("     -nb NUMBER, --number-bytes NUMBER\n");
	printf(
			"           NUMBER of random bytes to download into a file, max value\n");
	printf(
			"           200000000000, skip this option for unlimited amount of random\n");
	printf("           bytes (continuous download)\n");
	printf("\n");
	printf("     -dn NUMBER, --device-number NUMBER\n");
	printf(
			"           USB device NUMBER, if more than one. Skip this option if only\n");
	printf(
			"           one SwiftRNG device is connected, use '-ld' to list all available\n");
	printf("           devices\n");
	printf("\n");
	printf("     -ppn NUMBER, --power-profile-number NUMBER\n");
	printf(
			"           Device power profile NUMBER, 0 (lowest) to 9 (highest - default)\n");
	printf("\n");
	printf("     -ppm METHOD, --post-processing-method METHOD\n");
	printf("           SwiftRNG post processing method: SHA256, SHA512 or xorshift64\n");

	printf("\n");
	printf("     -dpp, --disable-post-processing\n");
	printf("           Disable post processing of random data for devices with version 1.2+\n");
	printf("\n");

	printf("\n");
	printf("     -dst, --disable-statistical-tests\n");
	printf("           Disable 'Repetition Count' and 'Adaptive Proportion' tests.\n");
	printf("\n");

	printf("EXAMPLES:\n");
	printf(
			"     It may require system admin permissions to run this utility on Linux or OSX.\n");
	printf(
			"     To list all available SwiftRNG (not currently in use) devices.\n");
	printf("           swrng -ld\n");
	printf("     To download 12 MB of true random bytes to 'rnd.bin' file\n");
	printf("           swrng  -dd -fn rnd.bin -nb 12000000\n");
	printf(
			"     To download 12 MB of true random bytes to 'rnd.bin' file using\n");
	printf("           lowest power consumption and slowest download speed\n");
	printf("           swrng  -dd -fn rnd.bin -nb 12000000 -ppn 0\n");
#ifdef __linux__
	printf("     To feed Kernel /dev/random entropy pool using the first device.\n");
	printf("           swrng -fep\n");
#endif
	printf("\n");
}

/**
 * Validate command line argument count
 *
 * @param int curIdx
 * @param int actualArgumentCount
 * @return swrngBool - true if run successfully
 */
swrngBool validateArgumentCount(int curIdx, int actualArgumentCount) {
	if (curIdx >= actualArgumentCount) {
		fprintf(stderr, "\nMissing command line arguments\n\n");
		displayUsage();
		return SWRNG_FALSE;
	}
	return SWRNG_TRUE;
}

/**
 * Parse device number if specified
 *
 * @param int idx - current parameter number
 * @param int argc - number of parameters
 * @param char ** argv - parameters
 * @return int - 0 when successfully parsed
 */
int parseDeviceNum(int idx, int argc, char **argv) {
	if (idx < argc) {
		if (strcmp("-dn", argv[idx]) == 0 || strcmp("--device-number",
				argv[idx]) == 0) {
			if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
				return -1;
			}
			deviceNum = atoi(argv[idx++]);
			if (deviceNum < 0) {
				fprintf(stderr, "Device number cannot be a negative value\n");
				return -1;
			}
		}
	}
	return 0;
}

/**
 * Parse power profile number if specified
 *
 * @param int idx - current parameter number
 * @param int argc - number of parameters
 * @param char ** argv - parameters
 * @return int - 0 when successfully parsed
 */
int parsePowerProfileNum(int idx, int argc, char **argv) {
	if (idx < argc) {
		if (strcmp("-ppn", argv[idx]) == 0 || strcmp("--power-profile-number",
				argv[idx]) == 0) {
			if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
				return -1;
			}
			ppNum = atoi(argv[idx++]);
			if (ppNum < 0 || ppNum > 9) {
				fprintf(stderr,
						"Power profile number invalid, must be between 0 and 9\n");
				return -1;
			}
		}
	}
	return 0;
}

/**
 * Parse arguments for extracting command line parameters
 *
 * @param int argc
 * @param char** argv
 * @return int - 0 when run successfully
 */
int processArguments(int argc, char **argv) {
	int idx = 1;
	if (strcmp("-ld", argv[idx]) == 0 || strcmp("--list-devices", argv[idx])
			== 0) {
		return displayDevices();
#ifdef __linux__
	} else if (strcmp("-fep", argv[idx]) == 0 || strcmp("--feed-entropy-pool", argv[idx]) == 0 ) {
		return feedKernelEntropyPool();
#endif
	} else if (strcmp("-dd", argv[idx]) == 0 || strcmp("--download-data",
			argv[idx]) == 0) {
		if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
			return -1;
		}
		while (idx < argc) {
			if (strcmp("-dpp", argv[idx]) == 0 || strcmp("--disable-post-processing",
					argv[idx]) == 0) {
				idx++;
				postProcessingEnabled = SWRNG_FALSE;
			} else if (strcmp("-dst", argv[idx]) == 0 || strcmp("--disable-statistical-tests",
					argv[idx]) == 0) {
				idx++;
				statisticalTestsEnabled = SWRNG_FALSE;
			} else if (strcmp("-nb", argv[idx]) == 0 || strcmp("--number-bytes",
					argv[idx]) == 0) {
				if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
					return -1;
				}
				numGenBytes = atoll(argv[idx++]);
				if (numGenBytes > 200000000000) {
					fprintf(stderr,
							"Number of bytes cannot exceed 200000000000\n");
					return -1;
				}
			} else if (strcmp("-fn", argv[idx]) == 0 || strcmp("--file-name",
					argv[idx]) == 0) {
				if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
					return -1;
				}
				filePathName = argv[idx++];
			} else if (strcmp("-ppm", argv[idx]) == 0 || strcmp("--post-processing-method",
					argv[idx]) == 0) {
				if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
					return -1;
				}
				postProcessingMethod = argv[idx++];
				if (strcmp("SHA256", postProcessingMethod) == 0) {
					postProcessingMethodId = 0;
				} else if (strcmp("SHA512", postProcessingMethod) == 0) {
					postProcessingMethodId = 2;
				} else if (strcmp("xorshift64", postProcessingMethod) == 0) {
					postProcessingMethodId = 1;
				} else {
					fprintf(stderr, "Invalid post processing method: %s \n", postProcessingMethod);
					return -1;
				}
			} else if (parseDeviceNum(idx, argc, argv) == -1) {
				return -1;
			} else if (parsePowerProfileNum(idx, argc, argv) == -1) {
				return -1;
			} else {
				// Could not handle the argument, skip to the next one
				++idx;
			}
		}
	}
	return processDownloadRequest();
}

/**
 * Close file handle
 *
 */
void closeHandle() {
	if (pOutputFile != NULL) {
		fclose(pOutputFile);
		pOutputFile = NULL;
	}
}

/**
 * Write bytes out to the file
 *
 * @param uint8_t* bytes - pointer to the byte array
 * @param uint32_t numBytes - number of bytes to write
 */
void writeBytes(uint8_t *bytes, uint32_t numBytes) {
	FILE *handle = pOutputFile;
	fwrite(bytes, 1, numBytes, handle);
}

/**
 * Handle download request
 *
 * @return int - 0 when run successfully
 */
int handleDownloadRequest() {

	uint8_t receiveByteBuffer[SWRNG_BUFF_FILE_SIZE_BYTES + 1]; // Add one extra byte of storage for the status byte
	swrngResetStatistics(&ctxt);

	int status = swrngOpen(&ctxt, deviceNum);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, " Cannot open device, error code %d ... ", status);
		swrngClose(&ctxt);
		return status;
	}

	if (statisticalTestsEnabled == SWRNG_FALSE) {
		status = swrngDisableStatisticalTests(&ctxt);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot disable statistical tests, error code %d ... ",status);
			swrngClose(&ctxt);
			return status;
		}
	}

	if (postProcessingEnabled == SWRNG_FALSE) {
		status = swrngDisablePostProcessing(&ctxt);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot disable post processing, error code %d ... ",
					status);
			swrngClose(&ctxt);
			return status;
		}
	} else if (postProcessingMethod != NULL) {
		status = swrngEnablePostProcessing(&ctxt, postProcessingMethodId);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot enable processing method, error code %d ... ",
					status);
			swrngClose(&ctxt);
			return status;
		}
	}

	if (swrngGetPostProcessingStatus(&ctxt, &postProcessingStatus) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngClose(&ctxt);
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
			strcpy(postProcessingMethodStr, "SHA256");
			break;
		case 1:
			strcpy(postProcessingMethodStr, "xorshift64");
			break;
		case 2:
			strcpy(postProcessingMethodStr, "SHA512");
			break;
		default:
			strcpy(postProcessingMethodStr, "*unknown*");
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


	status = swrngSetPowerProfile(&ctxt, ppNum);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, " Cannot set device power profile, error code %d ... ",
				status);
		swrngClose(&ctxt);
		return status;
	}

	if (filePathName == NULL) {
		fprintf(stderr, "No file name defined. ");
		swrngClose(&ctxt);
		return -1;
	}

	if (isOutputToStandardOutput == SWRNG_TRUE) {
#ifdef _WIN32
		_setmode(_fileno(stdout), _O_BINARY);
		pOutputFile = fdopen(_dup(fileno(stdout)), "wb");
#else
		pOutputFile = fdopen(dup(fileno(stdout)), "wb");
#endif

	} else {
		pOutputFile = fopen(filePathName, "wb");
	}
	if (pOutputFile == NULL) {
		fprintf(stderr, "Cannot open file: %s in write mode\n", filePathName);
		swrngClose(&ctxt);
		return -1;
	}


	swrngResetStatistics(&ctxt);

	while (numGenBytes == -1) {
		// Infinite loop for downloading unlimited random bytes
		status = swrngGetEntropy(&ctxt, receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
		if (status != SWRNG_SUCCESS) {
			fprintf(
					stderr,
					"Failed to receive %d bytes for unlimited download, error code %d. ",
					SWRNG_BUFF_FILE_SIZE_BYTES, status);
			closeHandle();
			swrngClose(&ctxt);
			return status;
		}
		writeBytes(receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
	}

	// Calculate number of complete random byte chunks to download
	int64_t numCompleteChunks = numGenBytes / SWRNG_BUFF_FILE_SIZE_BYTES;

	// Calculate number of bytes in the last incomplete chunk
	uint32_t chunkRemaindBytes = (uint32_t)(numGenBytes % SWRNG_BUFF_FILE_SIZE_BYTES);

	// Process each chunk
	int64_t chunkNum;
	for (chunkNum = 0; chunkNum < numCompleteChunks; chunkNum++) {
		status = swrngGetEntropy(&ctxt, receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, "Failed to receive %d bytes, error code %d. ",
					SWRNG_BUFF_FILE_SIZE_BYTES, status);
			closeHandle();
			swrngClose(&ctxt);
			return status;
		}
		writeBytes(receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
	}

	if (chunkRemaindBytes > 0) {
		//Process incomplete chunk
		status = swrngGetEntropy(&ctxt, receiveByteBuffer, chunkRemaindBytes);
		if (status != SWRNG_SUCCESS) {
			fprintf(
					stderr,
					"Failed to receive %d bytes for last chunk, error code %d. ",
					chunkRemaindBytes, status);
			closeHandle();
			swrngClose(&ctxt);
			return status;
		}
		writeBytes(receiveByteBuffer, chunkRemaindBytes);
	}

	closeHandle();
	swrngClose(&ctxt);
	return SWRNG_SUCCESS;
}

/**
 * Process Request
 *
 * @return int - 0 when run successfully
 */
int processDownloadRequest() {
	if (filePathName != NULL && (!strcmp(filePathName, "STDOUT") || !strcmp(filePathName, "/dev/stdout"))) {
		isOutputToStandardOutput = SWRNG_TRUE;
	} else {
		isOutputToStandardOutput = SWRNG_FALSE;
	}
	int status = handleDownloadRequest();
	DeviceStatistics *ds = swrngGenerateDeviceStatistics(&ctxt);
	if (!isOutputToStandardOutput) {
		printf("Completed in %d seconds, post-processing method used: %s, device built-in correction method used: %s, ",
				(int) ds->totalTime, postProcessingMethodStr, embeddedCorrectionMethodStr);
		if (statisticalTestsEnabled) {
			printf("statistical tests enabled, max RCT/APT failures per block: %d/%d, ", ctxt.maxRctFailuresPerBlock, ctxt.maxAptFailuresPerBlock);
		} else {
			printf("statistical tests disabled, ");
		}
		printf("speed: %d KBytes/sec, blocks re-sent: %d\n", (int) ds->downloadSpeedKBsec, (int) ds->totalRetries);
	}
	return status;
}

/**
 * Process command line arguments
 *
 * @param int argc - number of parameters
 * @param char ** argv - parameters
 * @return int - 0 when run successfully
 */
int process(int argc, char **argv) {
	// Initialize the context
#ifdef _WIN32
	strcpy_s(postProcessingMethodStr, "*unknown*");
#else
	strcpy(postProcessingMethodStr, "*unknown*");
#endif
	int status = swrngInitializeContext(&ctxt);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not initialize context\n");
		return(status);
	}

	swrngEnablePrintingErrorMessages(&ctxt);
	if (argc == 1) {
		displayUsage();
		return -1;
	}
	status = processArguments(argc, argv);
	swrngClose(&ctxt);
	return status;
}

//.............
#ifdef __linux__
//.............
/**
 * Feed Kernel entropy pool with true random bytes
 *
 * @return int - 0 when run successfully
 */
int feedKernelEntropyPool() {

	int status = swrngOpen(&ctxt, deviceNum);
	if(status != SWRNG_SUCCESS) {
		fprintf(stderr, "Cannot open device, error code %d ... ", status);
		swrngClose(&ctxt);
		return status;
	}

	int rndout = open(KERNEL_ENTROPY_POOL_NAME, O_WRONLY);
	if (rndout < 0) {
		printf("Cannot open %s : %d\n", KERNEL_ENTROPY_POOL_NAME, rndout);
		swrngClose(&ctxt);
		return rndout;
	}

	// Check to see if we have privileges for feeding the entropy pool
	int result = ioctl(rndout, RNDGETENTCNT, &entropyAvailable);
	if (result < 0) {
		printf("Cannot verify available entropy in the pool, make sure you run this utility with CAP_SYS_ADMIN capability\n");
		swrngClose(&ctxt);
		close(rndout);
		return result;
	}

	printf("Feeding the kernel %s entropy pool. Initial amount of entropy bits in the pool: %d ...\n", KERNEL_ENTROPY_POOL_NAME, entropyAvailable);

	// Infinite loop for feeding kernel entropy pool
	while(SWRNG_TRUE) {
		// Check to see if we need more entropy, feed the pool if the current level is below 2048 bits
		ioctl(rndout, RNDGETENTCNT, &entropyAvailable);
		if (entropyAvailable < (KERNEL_ENTROPY_POOL_SIZE_BYTES * 8) / 2) {
			int addMoreBytes = KERNEL_ENTROPY_POOL_SIZE_BYTES - (entropyAvailable >> 3);
			status = swrngGetEntropy(&ctxt, entropy.data, addMoreBytes);
			if(status != SWRNG_SUCCESS) {
				fprintf(stderr, "Failed to receive %d bytes for feeding entropy pool, error code %d. ", addMoreBytes, status);
				swrngClose(&ctxt);
				return status;
			}
			entropy.buf_size = addMoreBytes;
			// Estimate the amount of entropy
			entropy.entropy_count = entropyAvailable + (addMoreBytes << 3);
			// Push the entropy out to the pool
			result = ioctl(rndout, RNDADDENTROPY, &entropy);
			if (result < 0) {
				printf("Cannot add more entropy to the pool, error: %d\n", result);
				swrngClose(&ctxt);
				close(rndout);
				return result;
			}
			usleep(10);
		} else {
			usleep(30);
		}
	}

}
//.............
#endif
//.............


/**
 * Main entry
 *
 * @param int argc - number of parameters
 * @param char ** argv - parameters
 *
 */
int main(int argc, char **argv) {
	return process(argc, argv);
}

