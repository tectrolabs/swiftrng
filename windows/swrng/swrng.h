/*
 * swrng.h
 * ver. 2.1
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2017 TectroLabs, http://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for interacting with the hardware random data generator device SwiftRNG for the purpose of
 downloading true random bytes.

 This program requires the libusb-1.0 library when communicating with the SwiftRNG device. Please read the provided
 documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General 
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with the SwiftRNG device.

 This program may require 'sudo' permissions when running on Linux or OSX operating systems.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef SWRNG_H_
#define SWRNG_H_

#include "swrngapi.h"
#ifndef _WIN32
#include <unistd.h>
#else
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
int deviceNum; // USB device number starting with 0 (a command line argument)
int ppNum = 9; // Power profile number, between 0 and 9
FILE *pOutputFile = NULL; // Output file handle
swrngBool isOutputToStandardOutput = SWRNG_FALSE;

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
