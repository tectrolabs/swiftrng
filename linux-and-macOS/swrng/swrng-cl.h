/*
 * swrng-cl.h
 * Ver. 3.3
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

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

#include <swrng-cl-api.h>
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


#define SWRNG_BUFF_FILE_SIZE_BYTES (10000 * 10)


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

static const int val_true = 1;
static const int val_false = 0;

/**
 * Variables
 */

/* Total number of random bytes needed (a command line argument) max 100000000000 bytes */
static int64_t num_gen_bytes = -1;

/* File name for recording the random bytes (a command line argument) */
static char *file_path_name = NULL;

/* Post processing method or NULL if not specified */
static char *pp_method = NULL;

/* Post processing method id, 0 - SHA256, 1 - xorshift64, 2 - SHA512 */
static int pp_method_id = 0;

/* Cluster size */
static int cl_size = 2;

/* Power profile number, between 0 and 9 */
static int pp_num = 9;

/* Output file handle */
static FILE *p_output_file = NULL;

static int is_output_to_standard_output;
static SwrngContext hcxt;
static SwrngCLContext cxt;
static long failOverCount = 0;
static long resizeAttemptCount = 0;
static int actClusterSize = 0;
static int postProcessingEnabled;
static int statisticalTestsEnabled;


#ifdef __linux__
/* A variable for checking the amount of the entropy available in the kernel pool */
static int entropyAvailable;
Entropy entropy;
#endif

/**
 * Function Declarations
 */

static int display_devices(void);
static void display_usage(void);
static int process(int argc, char **argv);
static int process_arguments(int argc, char **argv);
static int validate_argument_count(int curIdx, int act_argument_count);
static int process_download_request(void);
static int handle_download_request(void);
static void write_bytes(const uint8_t *bytes, uint32_t num_bytes);


#ifdef __linux__
static int feedKernelEntropyPool();
#endif

#endif /* SWRNG_H_ */
