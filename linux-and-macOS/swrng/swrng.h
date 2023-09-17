 /*
 * swrng.h
 * ver. 3.3
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2023 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for interacting with the hardware random data generator device SwiftRNG for the purpose of
 downloading true random bytes.

 This program requires the libusb-1.0 library when communicating with the SwiftRNG device. Please read the provided
 documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General 
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with TectroLabs devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef SWRNG_H_
#define SWRNG_H_

#include <swrngapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#ifndef _WIN32
#include <unistd.h>
#else
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#endif

#define SWRNG_BUFF_FILE_SIZE_BYTES (10000 * 10)


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

/* USB device number starting with 0 (a command line argument) */
static int device_num;

/* Power profile number, between 0 and 9 */
static int pp_num = 9;

/* Output file handle */
static FILE *p_output_file = NULL;

static int is_output_to_standard_output;
static SwrngContext ctxt;
static int pp_enabled;
static int stats_tests_enabled;
static int act_pp_method_id;
static int pp_status;
static char pp_method_char[32];
static char emb_corr_method_char[32];
static int emb_corr_method_id;


#ifdef __linux__
/* A variable for checking the amount of the entropy available in the kernel pool */
int entropyAvailable;
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
static int parse_device_num(int idx, int argc, char **argv);
static int process_download_request(void);
static int handle_download_request(void);
static void write_bytes(const uint8_t *bytes, uint32_t num_bytes);
static int parse_pp_num(int idx, int argc, char **argv);
static void close_handle(void);
static void initialize(void);

#ifdef __linux__
static int feed_kernel_entropy_pool();
#endif

#endif /* SWRNG_H_ */
