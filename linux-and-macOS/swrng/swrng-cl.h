/*
 * swrng-cl.h
 * Ver. 2.9
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for interacting with a cluster of SwiftRNG devices for the purpose of
 downloading true random bytes from such devices.

 This program requires the libusb-1.0 library when communicating with any SwiftRNG device. Please read the provided
 documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with TectroLabs devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef SWRNG_H_
#define SWRNG_H_

#include "swrng-cl-api.h"
#ifndef _WIN32
#include <unistd.h>
#else
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#endif

#ifdef __linux__
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/random.h>

#define KERNEL_ENTROPY_POOL_SIZE_BYTES 512
#define KERNEL_ENTROPY_POOL_NAME "/dev/random"
#endif

/*
 * Structures
 */
#ifdef __linux__
typedef struct {
	int entropy_count;
	int buf_size;
	unsigned char data[KERNEL_ENTROPY_POOL_SIZE_BYTES + 1];
}Entropy;
#endif

/**
 * Variables
 */

int64_t numGenBytes = -1; // Total number of random bytes needed (a command line argument) max 100000000000 bytes
char *filePathName = NULL; // File name for recording the random bytes (a command line argument)
char *postProcessingMethod = NULL; // Post processing method or NULL if not specified
int postProcessingMethodId = 0; // Post processing method id, 0 - SHA256, 1 - xorshift64, 2 - SHA512
int clusterSize = 2; // cluster size
int ppNum = 9; // Power profile number, between 0 and 9
FILE *pOutputFile = NULL; // Output file handle
swrngBool isOutputToStandardOutput = SWRNG_FALSE;
SwrngContext hcxt;
SwrngCLContext cxt;
long failOverCount = 0;
long resizeAttemptCount = 0;
int actClusterSize = 0;
swrngBool postProcessingEnabled = SWRNG_TRUE;
swrngBool statisticalTestsEnabled = SWRNG_TRUE;


#ifdef __linux__
int entropyAvailable; // A variable for checking the amount of the entropy available in the kernel pool
Entropy entropy;
#endif

/**
 * Function Declarations
 */

int displayDevices();
void displayUsage();
int process(int argc, char **argv);
int processArguments(int argc, char **argv);
int validateArgumentCount(int curIdx, int actualArgumentCount);
int parseDeviceNum(int idx, int argc, char **argv);
int processDownloadRequest();
int handleDownloadRequest();
void writeBytes(uint8_t *bytes, uint32_t numBytes);

#ifdef __linux__
int feedKernelEntropyPool();
#endif

#endif /* SWRNG_H_ */
