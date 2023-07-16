/*
 * swrng.c
 * Ver. 3.4
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2023 TectroLabs, https://tectrolabs.com

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
static int display_devices(void) {
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
static void display_usage(void) {
	printf("*********************************************************************************\n");
	printf("                   TectroLabs - swrng - download utility Ver 3.4  \n");
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
 * @param int act_argument_count
 * @return int - 1 if run successfully
 */
static int validate_argument_count(int curIdx, int act_argument_count) {
	if (curIdx >= act_argument_count) {
		fprintf(stderr, "\nMissing command line arguments\n\n");
		display_usage();
		return val_false;
	}
	return val_true;
}

/**
 * Parse device number if specified
 *
 * @param int idx - current parameter number
 * @param int argc - number of parameters
 * @param char ** argv - parameters
 * @return int - 0 when successfully parsed
 */
static int parse_device_num(int idx, int argc, char **argv) {
	if (idx < argc) {
		if (strcmp("-dn", argv[idx]) == 0 || strcmp("--device-number",
				argv[idx]) == 0) {
			if (validate_argument_count(++idx, argc) == val_false) {
				return -1;
			}
			device_num = atoi(argv[idx++]);
			if (device_num < 0) {
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
static int parse_pp_num(int idx, int argc, char **argv) {
	if (idx < argc) {
		if (strcmp("-ppn", argv[idx]) == 0 || strcmp("--power-profile-number",
				argv[idx]) == 0) {
			if (validate_argument_count(++idx, argc) == val_false) {
				return -1;
			}
			pp_num = atoi(argv[idx++]);
			if (pp_num < 0 || pp_num > 9) {
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
static int process_arguments(int argc, char **argv) {
	int idx = 1;
	if (strcmp("-ld", argv[idx]) == 0 || strcmp("--list-devices", argv[idx])
			== 0) {
		return display_devices();
#ifdef __linux__
	} else if (strcmp("-fep", argv[idx]) == 0 || strcmp("--feed-entropy-pool", argv[idx]) == 0 ) {
		return feed_kernel_entropy_pool();
#endif
	} else if (strcmp("-dd", argv[idx]) == 0 || strcmp("--download-data",
			argv[idx]) == 0) {
		if (validate_argument_count(++idx, argc) == val_false) {
			return -1;
		}
		while (idx < argc) {
			if (strcmp("-dpp", argv[idx]) == 0 || strcmp("--disable-post-processing",
					argv[idx]) == 0) {
				idx++;
				pp_enabled = val_false;
			} else if (strcmp("-dst", argv[idx]) == 0 || strcmp("--disable-statistical-tests",
					argv[idx]) == 0) {
				idx++;
				stats_tests_enabled = val_false;
			} else if (strcmp("-nb", argv[idx]) == 0 || strcmp("--number-bytes",
					argv[idx]) == 0) {
				if (validate_argument_count(++idx, argc) == val_false) {
					return -1;
				}
				num_gen_bytes = (int64_t)atoll(argv[idx++]);
				if (num_gen_bytes > 200000000000) {
					fprintf(stderr,
							"Number of bytes cannot exceed 200000000000\n");
					return -1;
				}
			} else if (strcmp("-fn", argv[idx]) == 0 || strcmp("--file-name",
					argv[idx]) == 0) {
				if (validate_argument_count(++idx, argc) == val_false) {
					return -1;
				}
				file_path_name = argv[idx++];
			} else if (strcmp("-ppm", argv[idx]) == 0 || strcmp("--post-processing-method",
					argv[idx]) == 0) {
				if (validate_argument_count(++idx, argc) == val_false) {
					return -1;
				}
				pp_method = argv[idx++];
				if (strcmp("SHA256", pp_method) == 0) {
					pp_method_id = 0;
				} else if (strcmp("SHA512", pp_method) == 0) {
					pp_method_id = 2;
				} else if (strcmp("xorshift64", pp_method) == 0) {
					pp_method_id = 1;
				} else {
					fprintf(stderr, "Invalid post processing method: %s \n", pp_method);
					return -1;
				}
			} else if (parse_device_num(idx, argc, argv) == -1) {
				return -1;
			} else if (parse_pp_num(idx, argc, argv) == -1) {
				return -1;
			} else {
				/* Could not handle the argument, skip to the next one */
				++idx;
			}
		}
	}
	return process_download_request();
}

/**
 * Close file handle
 *
 */
static void close_handle(void) {
	if (p_output_file != NULL) {
		fclose(p_output_file);
		p_output_file = NULL;
	}
}

/**
 * Write bytes out to the file
 *
 * @param uint8_t* bytes - pointer to the byte array
 * @param uint32_t num_bytes - number of bytes to write
 */
static void write_bytes(uint8_t *bytes, uint32_t num_bytes) {
	FILE *handle = p_output_file;
	fwrite(bytes, 1, num_bytes, handle);
}

/**
 * Handle download request
 *
 * @return int - 0 when run successfully
 */
static int handle_download_request(void) {

	/* Add one extra byte of storage for the status byte */
	uint8_t receiveByteBuffer[SWRNG_BUFF_FILE_SIZE_BYTES + 1];
	swrngResetStatistics(&ctxt);

	int status = swrngOpen(&ctxt, device_num);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, " Cannot open device, error code %d ... ", status);
		swrngClose(&ctxt);
		return status;
	}

	if (stats_tests_enabled == val_false) {
		status = swrngDisableStatisticalTests(&ctxt);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot disable statistical tests, error code %d ... ",status);
			swrngClose(&ctxt);
			return status;
		}
	}

	if (pp_enabled == val_false) {
		status = swrngDisablePostProcessing(&ctxt);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot disable post processing, error code %d ... ",
					status);
			swrngClose(&ctxt);
			return status;
		}
	} else if (pp_method != NULL) {
		status = swrngEnablePostProcessing(&ctxt, pp_method_id);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot enable processing method, error code %d ... ",
					status);
			swrngClose(&ctxt);
			return status;
		}
	}

	if (swrngGetPostProcessingStatus(&ctxt, &pp_status) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		swrngClose(&ctxt);
		return(1);
	}

	if (pp_status == 0) {
		strcpy(pp_method_char, "none");
	} else {
		if (swrngGetPostProcessingMethod(&ctxt, &act_pp_method_id) != SWRNG_SUCCESS) {
			printf("%s\n", swrngGetLastErrorMessage(&ctxt));
			return(1);
		}
		switch (act_pp_method_id) {
		case 0:
			strcpy(pp_method_char, "SHA256");
			break;
		case 1:
			strcpy(pp_method_char, "xorshift64");
			break;
		case 2:
			strcpy(pp_method_char, "SHA512");
			break;
		default:
			strcpy(pp_method_char, "*unknown*");
			break;
		}
	}

	if (swrngGetEmbeddedCorrectionMethod(&ctxt, &emb_corr_method_id) != SWRNG_SUCCESS) {
		printf("%s\n", swrngGetLastErrorMessage(&ctxt));
		return(1);
	}

	switch(emb_corr_method_id) {
	case 0:
		strcpy(emb_corr_method_char, "none");
		break;
	case 1:
		strcpy(emb_corr_method_char, "Linear");
		break;
	default:
		strcpy(emb_corr_method_char, "*unknown*");
	}


	status = swrngSetPowerProfile(&ctxt, pp_num);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, " Cannot set device power profile, error code %d ... ",
				status);
		swrngClose(&ctxt);
		return status;
	}

	if (file_path_name == NULL) {
		fprintf(stderr, "No file name defined. ");
		swrngClose(&ctxt);
		return -1;
	}

	if (is_output_to_standard_output == val_true) {
#ifdef _WIN32
		_setmode(_fileno(stdout), _O_BINARY);
		p_output_file = fdopen(_dup(fileno(stdout)), "wb");
#else
		p_output_file = fdopen(dup(fileno(stdout)), "wb");
#endif

	} else {
		p_output_file = fopen(file_path_name, "wb");
	}
	if (p_output_file == NULL) {
		fprintf(stderr, "Cannot open file: %s in write mode\n", file_path_name);
		swrngClose(&ctxt);
		return -1;
	}


	swrngResetStatistics(&ctxt);

	while (num_gen_bytes == -1) {
		/* Infinite loop for downloading unlimited random bytes */
		status = swrngGetEntropy(&ctxt, receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
		if (status != SWRNG_SUCCESS) {
			fprintf(
					stderr,
					"Failed to receive %d bytes for unlimited download, error code %d. ",
					SWRNG_BUFF_FILE_SIZE_BYTES, status);
			close_handle();
			swrngClose(&ctxt);
			return status;
		}
		write_bytes(receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
	}

	/* Calculate number of complete random byte chunks to download */
	int64_t numCompleteChunks = num_gen_bytes / SWRNG_BUFF_FILE_SIZE_BYTES;

	/* Calculate number of bytes in the last incomplete chunk */
	uint32_t chunkRemaindBytes = (uint32_t)(num_gen_bytes % SWRNG_BUFF_FILE_SIZE_BYTES);

	/* Process each chunk */
	int64_t chunkNum;
	for (chunkNum = 0; chunkNum < numCompleteChunks; chunkNum++) {
		status = swrngGetEntropy(&ctxt, receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, "Failed to receive %d bytes, error code %d. ",
					SWRNG_BUFF_FILE_SIZE_BYTES, status);
			close_handle();
			swrngClose(&ctxt);
			return status;
		}
		write_bytes(receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
	}

	if (chunkRemaindBytes > 0) {
		/* Process incomplete chunk */
		status = swrngGetEntropy(&ctxt, receiveByteBuffer, chunkRemaindBytes);
		if (status != SWRNG_SUCCESS) {
			fprintf(
					stderr,
					"Failed to receive %d bytes for last chunk, error code %d. ",
					chunkRemaindBytes, status);
			close_handle();
			swrngClose(&ctxt);
			return status;
		}
		write_bytes(receiveByteBuffer, chunkRemaindBytes);
	}

	close_handle();
	swrngClose(&ctxt);
	return SWRNG_SUCCESS;
}

/**
 * Process Request
 *
 * @return int - 0 when run successfully
 */
static int process_download_request(void) {

	uint16_t maxAptFailuresPerBlock;
	uint16_t maxRctFailuresPerBlock;

	if (file_path_name != NULL && (!strcmp(file_path_name, "STDOUT") || !strcmp(file_path_name, "/dev/stdout"))) {
		is_output_to_standard_output = val_true;
	} else {
		is_output_to_standard_output = val_false;
	}
	int status = handle_download_request();
	DeviceStatistics *ds = swrngGenerateDeviceStatistics(&ctxt);
	if (!is_output_to_standard_output) {
		printf("Completed in %d seconds, post-processing method used: %s, device built-in correction method used: %s, ",
				(int) ds->totalTime, pp_method_char, emb_corr_method_char);
		if (stats_tests_enabled) {
			swrngGetMaxAptFailuresPerBlock(&ctxt, &maxAptFailuresPerBlock);
			swrngGetMaxRctFailuresPerBlock(&ctxt, &maxRctFailuresPerBlock);
			printf("statistical tests enabled, max RCT/APT failures per block: %d/%d, ", maxRctFailuresPerBlock, maxAptFailuresPerBlock);
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
static int process(int argc, char **argv) {
	strcpy(pp_method_char, "*unknown*");
	int status = swrngInitializeContext(&ctxt);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not initialize context\n");
		return(status);
	}

	swrngEnablePrintingErrorMessages(&ctxt);
	if (argc == 1) {
		display_usage();
		swrngDestroyContext(&ctxt);
		return -1;
	}
	status = process_arguments(argc, argv);
	swrngDestroyContext(&ctxt);
	return status;
}

/* ............. */
#ifdef __linux__
/* ............. */
/**
 * Feed Kernel entropy pool with true random bytes
 *
 * @return int - 0 when run successfully
 */
static int feed_kernel_entropy_pool(void) {

	int status = swrngOpen(&ctxt, device_num);
	if(status != SWRNG_SUCCESS) {
		fprintf(stderr, "Cannot open device, error code %d ... ", status);
		swrngDestroyContext(&ctxt);
		return status;
	}

	int rndout = open(KERNEL_ENTROPY_POOL_NAME, O_WRONLY);
	if (rndout < 0) {
		printf("Cannot open %s : %d\n", KERNEL_ENTROPY_POOL_NAME, rndout);
		swrngDestroyContext(&ctxt);
		return rndout;
	}

	/* Check to see if we have privileges for feeding the entropy pool */
	int result = ioctl(rndout, RNDGETENTCNT, &entropyAvailable);
	if (result < 0) {
		printf("Cannot verify available entropy in the pool, make sure you run this utility with CAP_SYS_ADMIN capability\n");
		swrngDestroyContext(&ctxt);
		close(rndout);
		return result;
	}

	printf("Feeding the kernel %s entropy pool. Initial amount of entropy bits in the pool: %d ...\n", KERNEL_ENTROPY_POOL_NAME, entropyAvailable);

	/* Infinite loop for feeding kernel entropy pool */
	while(val_true) {
		/* Check to see if we need more entropy, feed the pool if the current level is below 2048 bits */
		ioctl(rndout, RNDGETENTCNT, &entropyAvailable);
		if (entropyAvailable < (KERNEL_ENTROPY_POOL_SIZE_BYTES * 8) / 2) {
			int addMoreBytes = KERNEL_ENTROPY_POOL_SIZE_BYTES - (entropyAvailable >> 3);
			status = swrngGetEntropy(&ctxt, entropy.data, addMoreBytes);
			if(status != SWRNG_SUCCESS) {
				fprintf(stderr, "Failed to receive %d bytes for feeding entropy pool, error code %d. ", addMoreBytes, status);
				swrngDestroyContext(&ctxt);
				return status;
			}
			entropy.buf_size = addMoreBytes;
			/* Estimate the amount of entropy */
			entropy.entropy_count = entropyAvailable + (addMoreBytes << 3);
			/* Push the entropy out to the pool */
			result = ioctl(rndout, RNDADDENTROPY, &entropy);
			if (result < 0) {
				printf("Cannot add more entropy to the pool, error: %d\n", result);
				swrngDestroyContext(&ctxt);
				close(rndout);
				return result;
			}
			usleep(10);
		} else {
			usleep(30);
		}
	}

}
/* ............. */
#endif
/* ............. */

/**
* Initialize some values here to work around compiling errors on some platforms
*/
static void initialize(void) {
	is_output_to_standard_output = val_false;
	pp_enabled = val_true;
	stats_tests_enabled = val_true;
}

/**
 * Main entry
 *
 * @param int argc - number of parameters
 * @param char ** argv - parameters
 *
 */
int main(int argc, char **argv) {
	initialize();
	return process(argc, argv);
}

