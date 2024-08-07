/*
 * swrng-cl.c
 * ver. 3.6
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2024 TectroLabs, https://tectrolabs.com

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

#include "swrng-cl.h"

/**
 * Display information about SwifrRNG devices
 *
 * @return int - 0 when run successfully
 */
static int display_devices(void) {
	DeviceInfoList dil;

	int status = swrngGetDeviceList(&hcxt, &dil);

	if (status != 0) {
		fprintf(stderr, "Could not generate device info list, status: %d\n",
				status);
		return status;
	}

	if (dil.numDevs > 0) {
		printf("\n");
		for (int i = 0; i < dil.numDevs; i++) {
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
	printf("             TectroLabs - swrng-cl - cluster download utility Ver 3.6        \n");
	printf("*********************************************************************************\n");
	printf("NAME\n");
	printf("     swrng-cl - Download true random bytes from a cluster of SwiftRNG devices\n");
	printf("SYNOPSIS\n");
	printf("     swrng-cl -ld --list-devices | -dd --download-data [options] \n");
#ifdef __linux__
	printf("           -fep --feed-entropy-pool\n");
#endif
	printf("\n");
	printf("DESCRIPTION\n");
	printf("     swrng-cl downloads random bytes from a cluster of SwiftRNG devices into \n");
	printf("     a binary file.\n");
	printf("\n");
	printf("FUNCTION LETTERS\n");
	printf("     Main operation mode:\n");
	printf("\n");
	printf("     -ld, --list-devices\n");
	printf("           list all available (not currently in use) SwiftRNG devices\n");
	printf("\n");
	printf("     -dd, --download-data\n");
	printf("           download random bytes from the a cluster of SwiftRNG devices and store \n");
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
	printf("           a FILE name for storing random data. Use /dev/stdout to send bytes\n");
	printf("           to standard output\n");
	printf("\n");
	printf("     -nb NUMBER, --number-bytes NUMBER\n");
	printf("           NUMBER of random bytes to download into a file, max value\n");
	printf("           200000000000, skip this option for unlimited amount of random\n");
	printf("           bytes (continuous download)\n");
	printf("\n");
	printf("     -cs NUMBER, --cluster-size NUMBER\n");
	printf("           Preferred number (between 1 and 10) of devices in a cluster.\n");
	printf("           Default value is 2\n");

	printf("\n");
	printf("     -ppn NUMBER, --power-profile-number NUMBER\n");
	printf("           Device power profile NUMBER, 0 (lowest) to 9 (highest - default)\n");
	printf("\n");
	printf("     -ppm METHOD, --post-processing-method METHOD\n");
	printf("           SwiftRNG post processing method: SHA256, SHA512 or xorshift64\n");

	printf("\n");
	printf("     -dpp, --disable-post-processing\n");
	printf("           Disable post processing of random data for devices with version 1.2+\n");
	printf("\n");
	printf("     -dst, --disable-statistical-tests\n");
	printf("           Disable 'Repetition Count' and 'Adaptive Proportion' tests.\n");
	printf("\n");
	printf("EXAMPLES:\n");
	printf("     It may require system admin permissions to run this utility on Linux or OSX.\n");
	printf("     To list all available SwiftRNG (not currently in use) devices.\n");
	printf("           swrng-cl -ld\n");
	printf("     To download 12 MB of true random bytes to 'rnd.bin' file using a cluster \n");
	printf("     of 2 devices \n");
	printf("           swrng-cl  -dd -fn rnd.bin -nb 12000000\n");
	printf("     To download 12 MB of true random bytes to 'rnd.bin' file using a cluster \n");
	printf("     of 3 devices \n");
	printf("           swrng-cl  -dd -fn rnd.bin -nb 12000000 -cs 3\n");
	printf("     To download 12 MB of true random bytes to 'rnd.bin' file using \n");
	printf("           lowest power consumption and slowest download speed\n");
	printf("           swrng-cl  -dd -fn rnd.bin -nb 12000000 -ppn 0\n");
#ifdef __linux__
	printf("     To feed Kernel /dev/random entropy pool using a cluster of 2 devices.\n");
	printf("           ./swrng -fep\n");
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
 * Parse cluster size if specified
 *
 * @param int idx - current parameter number
 * @param int argc - number of parameters
 * @param char ** argv - parameters
 * @return int - 0 when successfully parsed
 */
static int parseClusterSize(int idx, int argc, char **argv) {
	if (idx < argc) {
		if (strcmp("-cs", argv[idx]) == 0 || strcmp("--cluster-size",
				argv[idx]) == 0) {
			if (validate_argument_count(++idx, argc) == val_false) {
				return -1;
			}
			cl_size = atoi(argv[idx++]);
			if (cl_size < 0 || cl_size > 10) {
				fprintf(stderr, "Cluster size must be between 1 and 10\n");
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
static int parsePowerProfileNum(int idx, int argc, char **argv) {
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
		return feedKernelEntropyPool();
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
				postProcessingEnabled = val_false;
			} else if (strcmp("-dst", argv[idx]) == 0 || strcmp("--disable-statistical-tests", argv[idx]) == 0) {
				idx++;
				statisticalTestsEnabled = val_false;
			} else if (strcmp("-nb", argv[idx]) == 0 || strcmp("--number-bytes",
					argv[idx]) == 0) {
				if (validate_argument_count(++idx, argc) == val_false) {
					return -1;
				}
				num_gen_bytes = atoll(argv[idx++]);
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
			} else if (parseClusterSize(idx, argc, argv) == -1) {
				return -1;
			} else if (parsePowerProfileNum(idx, argc, argv) == -1) {
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
static void closeHandle() {
	if (p_output_file != NULL) {
		fclose(p_output_file);
		p_output_file = NULL;
	}
}

/**
 * Write bytes out to the file
 *
 * @param const uint8_t* bytes - pointer to the byte array
 * @param uint32_t num_bytes - number of bytes to write
 */
static void write_bytes(const uint8_t *bytes, uint32_t num_bytes) {
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

	int status = swrngCLOpen(&cxt, cl_size);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, " Cannot open cluster, error code %d ... ", status);
		swrngCLClose(&cxt);
		return status;
	}

	if (statisticalTestsEnabled == val_false) {
		status = swrngDisableCLStatisticalTests(&cxt);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot disable statistical tests, error code %d ... ", status);
			swrngCLClose(&cxt);
			return status;
		}
	}

	if (postProcessingEnabled == val_false) {
		status = swrngDisableCLPostProcessing(&cxt);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot disable post processing, error code %d ... ",
					status);
			swrngCLClose(&cxt);
			return status;
		}
	} else if (pp_method != NULL) {
		status = swrngEnableCLPostProcessing(&cxt, pp_method_id);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, " Cannot enable processing method, error code %d ... ",
					status);
			swrngCLClose(&cxt);
			return status;
		}
	}

	status = swrngSetCLPowerProfile(&cxt, pp_num);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, " Cannot set device power profile, error code %d ... ",
				status);
		swrngCLClose(&cxt);
		return status;
	}

	if (file_path_name == NULL) {
		fprintf(stderr, "No file name defined. ");
		swrngCLClose(&cxt);
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
		swrngCLClose(&cxt);
		return -1;
	}

	while (num_gen_bytes == -1) {
		/* Infinite loop for downloading unlimited random bytes */
		status = swrngGetCLEntropy(&cxt, receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
		if (status != SWRNG_SUCCESS) {
			fprintf(
					stderr,
					"Failed to receive %d bytes for unlimited download, error code %d. ",
					SWRNG_BUFF_FILE_SIZE_BYTES, status);
			closeHandle();
			swrngCLClose(&cxt);
			return status;
		}
		write_bytes(receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
	}

	/* Calculate number of complete random byte chunks to download */
	int64_t numCompleteChunks = num_gen_bytes / SWRNG_BUFF_FILE_SIZE_BYTES;

	/* Calculate number of bytes in the last incomplete chunk */
	uint32_t chunkRemaindBytes = (uint32_t)(num_gen_bytes % SWRNG_BUFF_FILE_SIZE_BYTES);

	/* Process each chunk */
	for (int64_t chunkNum = 0; chunkNum < numCompleteChunks; chunkNum++) {
		status = swrngGetCLEntropy(&cxt, receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, "Failed to receive %d bytes, error code %d. ",
					SWRNG_BUFF_FILE_SIZE_BYTES, status);
			closeHandle();
			swrngCLClose(&cxt);
			return status;
		}
		write_bytes(receiveByteBuffer, SWRNG_BUFF_FILE_SIZE_BYTES);
	}

	if (chunkRemaindBytes > 0) {
		/* Process incomplete chunk */
		status = swrngGetCLEntropy(&cxt, receiveByteBuffer, chunkRemaindBytes);
		if (status != SWRNG_SUCCESS) {
			fprintf(
					stderr,
					"Failed to receive %d bytes for last chunk, error code %d. ",
					chunkRemaindBytes, status);
			closeHandle();
			swrngCLClose(&cxt);
			return status;
		}
		write_bytes(receiveByteBuffer, chunkRemaindBytes);
	}

	closeHandle();
	failOverCount = swrngGetCLFailoverEventCount(&cxt);
	resizeAttemptCount = swrngGetCLResizeAttemptCount(&cxt);
	actClusterSize = swrngGetCLSize(&cxt);
	swrngCLClose(&cxt);
	return SWRNG_SUCCESS;
}

/**
 * Process Request
 *
 * @return int - 0 when run successfully
 */
static int process_download_request() {
	if (file_path_name != NULL && (!strcmp(file_path_name, "STDOUT") || !strcmp(file_path_name, "/dev/stdout"))) {
		is_output_to_standard_output = val_true;
	} else {
		is_output_to_standard_output = val_false;
	}
	int status = handle_download_request();
	if (!is_output_to_standard_output) {
		printf("Completed, cluster size: %d", actClusterSize);
		printf(", fail-over events: %ld", failOverCount);
		printf(", cluster resize attempts: %ld", resizeAttemptCount);
		printf("\n");
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
	/* Initialize the context */
	int status = swrngInitializeContext(&hcxt);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not initialize context\n");
		return status;
	}

	status = swrngInitializeCLContext(&cxt);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not initialize cluster context\n");
		return status;
	}

	swrngEnableCLPrintingErrorMessages(&cxt);
	if (argc == 1) {
		display_usage();
		swrngDestroyContext(&hcxt);
		return -1;
	}
	status = process_arguments(argc, argv);
	swrngDestroyContext(&hcxt);
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
static int feedKernelEntropyPool() {

	int status = swrngCLOpen(&cxt, cl_size);
	if(status != SWRNG_SUCCESS) {
		fprintf(stderr, "Cannot open device, error code %d ... ", status);
		swrngCLClose(&cxt);
		return status;
	}

	int rndout = open(KERNEL_ENTROPY_POOL_NAME, O_WRONLY);
	if (rndout < 0) {
		printf("Cannot open %s : %d\n", KERNEL_ENTROPY_POOL_NAME, rndout);
		swrngCLClose(&cxt);
		return rndout;
	}

	/* Check to see if we have privileges for feeding the entropy pool */
	int result = ioctl(rndout, RNDGETENTCNT, &entropyAvailable);
	if (result < 0) {
		printf("Cannot verify available entropy in the pool, make sure you run this utility with CAP_SYS_ADMIN capability\n");
		swrngCLClose(&cxt);
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
			status = swrngGetCLEntropy(&cxt, entropy.data, addMoreBytes);
			if(status != SWRNG_SUCCESS) {
				fprintf(stderr, "Failed to receive %d bytes for feeding entropy pool, error code %d. ", addMoreBytes, status);
				swrngCLClose(&cxt);
				return status;
			}
			entropy.buf_size = addMoreBytes;
			/* Estimate the amount of entropy */
			entropy.entropy_count = entropyAvailable + (addMoreBytes << 3);
			/* Push the entropy out to the pool */
			result = ioctl(rndout, RNDADDENTROPY, &entropy);
			if (result < 0) {
				printf("Cannot add more entropy to the pool, error: %d\n", result);
				swrngCLClose(&cxt);
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
 * Main entry
 *
 * @param int argc - number of parameters
 * @param char ** argv - parameters
 */
int main(int argc, char **argv) {
	/* Initialize some values here to work around compiling errors on some platforms*/
	is_output_to_standard_output = val_false;
	postProcessingEnabled = val_true;
	statisticalTestsEnabled = val_true;

	return process(argc, argv);
}

