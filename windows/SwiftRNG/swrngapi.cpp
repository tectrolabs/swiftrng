#include "stdafx.h"

/*
 * swrngapi.c
 * ver. 1.2
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2016 TectroLabs, http://tectrolabs.com

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

#include "swrngapi.h"


#define DEV_NOT_FOUND_MSG "Device not open"
#define TOO_MANY_DEVICES "Cannot have more than 127 USB devices"
#define CANNOT_READ_DEVICE_DESCRIPTOR "Failed to retrieve USB device descriptor"
#define LIBUSB_INIT_FAILURE "Failed to initialize libusb"
#define VENDOR_ID (0x1fc9)
#define PRODUCT_ID (0x8110)	// This product ID is only to be used with SwiftRNG device
#define BULK_EP_OUT     0x01
#define BULK_EP_IN      0x81

#define WORD_SIZE_BYTES (4)
#define MIN_INPUT_NUM_WORDS (8)
#define OUT_NUM_WORDS (8)
#define NUM_CHUNKS (500)
#define RND_IN_BUFFSIZE (NUM_CHUNKS * MIN_INPUT_NUM_WORDS * WORD_SIZE_BYTES)
#define TRND_OUT_BUFFSIZE (NUM_CHUNKS * OUT_NUM_WORDS * WORD_SIZE_BYTES)
#define USB_READ_MAX_RETRY_CNT	(15)
#define USB_READ_TIMEOUT_SECS	(2)
#define USB_BULK_READ_TIMEOUT_MLSECS	(100)
#define MAX_REQUEST_SIZE_BYTES (100000L)

//
// Structures section
//

static struct sha_data {
	uint32_t a, b, c, d, e, f, g, h;
	uint32_t h0, h1, h2, h3, h4, h5, h6, h7;
	uint32_t tmp1, tmp2;
	uint32_t w[64];
	uint32_t blockSerialNumber;
} sd;

// Repetition Count Test data
static struct rct_data {
	uint32_t maxRepetitions;
	uint32_t curRepetitions;
	uint8_t lastSample;
	uint8_t statusByte;
	uint8_t signature;
	swrngBool isInitialized;
	uint32_t failureWindow;
	uint16_t failureCount;
} rct;

// Adaptive Proportion Test data
static struct apt_data {
	uint16_t windoiwSize;
	uint16_t cutoffValue;
	uint16_t curRepetitions;
	uint16_t curSamples;
	uint8_t statusByte;
	uint8_t signature;
	swrngBool isInitialized;
	uint8_t firstSample;
	uint16_t cycleFailures;
} apt;


//
// SHA256 data variable section
//
#define ROTR(sb,w) (((w) >> (sb)) | ((w) << (32-(sb))))

static const uint32_t k[64] =
		{ 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
				0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
				0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
				0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
				0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
				0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
				0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
				0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
				0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
				0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
				0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
				0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
				0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

static const uint32_t testSeq1[11] = { 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0x428a2f98,
		0x71374491, 0xb5c0fbcf };

static const uint32_t exptHashSeq1[8] = { 0x114c3052, 0x76410592, 0xc024566b,
		0xa492b1a2, 0xb0559389, 0xb7c41156, 0x2ec8d6c3, 0x3dcb02dd };

static const uint8_t maxDataBlockSizeWords = 16;


static libusb_device **devs = NULL;
static libusb_device *dev = NULL;
static libusb_device_handle *devh = NULL;
static swrngBool libusbInitialized; // True if libusb has been initialized
static swrngBool deviceOpen; // True if device successfully open
static int curTrngOutIdx = TRND_OUT_BUFFSIZE; // Current index for the buffTRndOut buffer
static unsigned char bulk_in_buffer[RND_IN_BUFFSIZE + 1];
static unsigned char bulk_out_buffer[16];
static char buffRndIn[RND_IN_BUFFSIZE + 1]; // Random input buffer
static char buffTRndOut[TRND_OUT_BUFFSIZE]; // Random output buffer
static uint32_t srcToHash[MIN_INPUT_NUM_WORDS + 1]; // The source of one block of data to hash
static const uint8_t numConsecFailThreshold = 5;// For real-time statistical tests
static DeviceStatistics ds;
static char lastError[512];
static swrngBool enPrintErrorMessages = SWRNG_FALSE;

//
// Declarations for local functions
//

static void swrng_printErrorMessage(const char* errMsg);
static void swrng_clearReceiverBuffer();
static void sha256_initialize(void);
static void sha256_stampSerialNumber(void *inputBlock);
static void sha256_initializeSerialNumber(uint32_t initValue);
static int sha256_generateHash(uint32_t *src, int16_t len, uint32_t *dst);
static void sha256_hashCurrentBlock(void);
static uint32_t sha256_ch(uint32_t *x, uint32_t *y, uint32_t *z);
static uint32_t sha256_maj(uint32_t *x, uint32_t *y, uint32_t *z);
static uint32_t sha256_sum0(uint32_t *x);
static uint32_t sha256_sum1(uint32_t *x);
static uint32_t sha256_sigma0(uint32_t *x);
static uint32_t sha256_sigma1(uint32_t *x);
static int sha256_selfTest(void);

static void rct_initialize(void);
static void rct_restart(void);
static void rct_sample(uint8_t value);

static void apt_initialize(void);
static void apt_restart(void);
static void apt_restart_cycle(void);
static void apt_sample(uint8_t value);


/**
 * Close device if open
 *
 * @return int - 0 when processed successfully
 */
int swrngClose() {
	if (devh) {
		libusb_release_interface(devh, 0); //release the claimed interface
		libusb_close(devh);
		devh = NULL;
	}
	if (devs) {
		libusb_free_device_list(devs, 1);
		devs = NULL;
	}
	if (libusbInitialized == SWRNG_TRUE) {
		libusb_exit( NULL);
		libusbInitialized = SWRNG_FALSE;
	}
	deviceOpen = SWRNG_FALSE;
	return SWRNG_SUCCESS;
}

/**
 * A function to handle SW device receive command
 * @param char *buff - a pointer to the data receive buffer
 * @param int length - how many bytes expected to receive
 * @param int opTimeoutSecs - device read time out value in seconds

 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int swrng_chip_read_data(char *buff, int length, int opTimeoutSecs) {
	long secsWaited;
	int transferred;
	time_t start, end;
	int cnt;
	int i;
	int retval;

	start = time(NULL);

	cnt = 0;
	do {
		retval = libusb_bulk_transfer(devh, BULK_EP_IN, bulk_in_buffer, length,
				&transferred, USB_BULK_READ_TIMEOUT_MLSECS);

#ifdef inDebugMode
		fprintf(stderr, "chip_read_data retval %d transferred %d, length %d\n", retval, transferred, length);
#endif
		if (retval) {
			return retval;
		}

		if (transferred > RND_IN_BUFFSIZE + 1) {
			swrng_printErrorMessage("Received unexpected bytes when processing USB device request");
			return -EFAULT;
		}

		end = time(NULL);
		secsWaited = (long)(end - start);
		if (transferred > 0) {
			for (i = 0; i < transferred; i++) {
				buff[cnt++] = bulk_in_buffer[i];
			}
		}
	} while (cnt < length && secsWaited < opTimeoutSecs);

	if (cnt != length) {
#ifdef inDebugMode
		fprintf(stderr, "timeout received, cnt %d\n", cnt);
#endif
		return -ETIMEDOUT;
	}

	return SWRNG_SUCCESS;
}

/**
 * Send a SW device command and receive response
 *
 * @param char *snd -  a pointer to the command
 * @param int sizeSnd - how many bytes in command
 * @param char *rcv - a pointer to the data receive buffer
 * @param int sizeRcv - how many bytes expected to receive
 * @param int opTimeoutSecs - device read time out value in seconds
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int swrng_snd_rcv_usb_data(char *snd, int sizeSnd, char *rcv, int sizeRcv,
		int opTimeoutSecs) {
	int retry;
	int actualcCnt;
	int retval = SWRNG_SUCCESS;

	for (retry = 0; retry < USB_READ_MAX_RETRY_CNT; retry++) {
		retval = libusb_bulk_transfer(devh, BULK_EP_OUT, (unsigned char*) snd,
				sizeSnd, &actualcCnt, 100);
		if (retval == SWRNG_SUCCESS && actualcCnt == sizeSnd) {
			retval = swrng_chip_read_data(rcv, sizeRcv + 1, opTimeoutSecs);
			if (retval == SWRNG_SUCCESS) {
				if (rcv[sizeRcv] != 0) {
					retval = -EFAULT;
				} else {
					ds.numGenBytes += sizeRcv;
					break;
				}
			}
		}
		#ifdef inDebugMode
			fprintf(stderr, "It was an error during data communication. Cleaning up the receiving queue and continue.\n");
		#endif
		ds.totalRetries++;
		swrng_chip_read_data(buffRndIn, RND_IN_BUFFSIZE + 1, opTimeoutSecs);
	}
	if (retry >= USB_READ_MAX_RETRY_CNT) {
		if (retval == SWRNG_SUCCESS) {
			retval = -ETIMEDOUT;
		}
	}
	return retval;
}

/**
 * Open SwiftRNG USB specific device.
 *
 * @param int deviceNum - device number (0 - for the first device)
 * @return int 0 - when open successfully or error code
 */
int swrngOpen(int deviceNum) {
	strcpy(lastError, "");
	swrngClose();

	rct_initialize();
	apt_initialize();

	sha256_initializeSerialNumber((uint32_t) ds.beginTime);
	if (sha256_selfTest() != SWRNG_SUCCESS) {
		swrng_printErrorMessage("Post processing logic failed the self-test");
		return -EPERM;
	}

	int r = libusb_init(NULL);
	if (r < 0) {
		swrng_printErrorMessage(LIBUSB_INIT_FAILURE);
		return r;
	}
	libusbInitialized = SWRNG_TRUE;
	ssize_t cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0) {
		return cnt;
	}
	int i = 0;
	int curFoundDevNum = -1;
	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			swrng_printErrorMessage(CANNOT_READ_DEVICE_DESCRIPTOR);
			return r;
		}
		uint16_t idVendorCur = desc.idVendor;
		uint16_t idProductCur = desc.idProduct;
		if (idVendorCur == VENDOR_ID && idProductCur == PRODUCT_ID) {
			if (++curFoundDevNum == deviceNum) {
				r = libusb_open(dev, &devh);
				switch (r) {
				case LIBUSB_ERROR_NO_MEM:
					swrng_printErrorMessage("Memory allocation failure");
					return r;
				case LIBUSB_ERROR_ACCESS:
					swrng_printErrorMessage("User has insufficient permissions");
					return r;
				case LIBUSB_ERROR_NO_DEVICE:
					swrng_printErrorMessage("Device has been disconnected");
					return r;
				}
				if (r < 0) {
					swrng_printErrorMessage("Failed to open USB device");
					return r;
				}

				if (libusb_kernel_driver_active(devh, 0) == 1) { //find out if kernel driver is attached
					swrng_printErrorMessage("Device is already in use");
					return -1;
				}
				
				r = libusb_claim_interface(devh, 0);
				if (r < 0) {
					swrng_printErrorMessage("Cannot claim the USB interface");
					return r;
				}
				

				// Clear the receive buffer
				swrng_clearReceiverBuffer();
				deviceOpen = SWRNG_TRUE;
				return SWRNG_SUCCESS;
			}
		}
	}
	swrng_printErrorMessage("Could not find any SwiftRNG device");
	return -1;
}

/**
 * Clear potential random bytes from the receiveer buffer if any
 */
static void swrng_clearReceiverBuffer() {
	int transferred;
	int retval;
	int i;
	for(i = 0; i < 10; i++) {
		retval = libusb_bulk_transfer(devh, BULK_EP_IN, bulk_in_buffer, RND_IN_BUFFSIZE + 1,
			&transferred, USB_BULK_READ_TIMEOUT_MLSECS);
		if (retval) {
			break;
		} if (transferred == 0) {
			break;
		}
	}
}

/**
 * A function to fill the buffer with new entropy bytes
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int swrng_rcv_rnd_bytes(void) {
	int retval;
	int i, j;
	uint32_t *dst;

	if (!deviceOpen) {
		return -EPERM;
	}

	bulk_out_buffer[0] = 'x';

	retval = swrng_snd_rcv_usb_data((char *) bulk_out_buffer, 1, buffRndIn,
			RND_IN_BUFFSIZE, USB_READ_TIMEOUT_SECS);
	if (retval == SWRNG_SUCCESS) {
		dst = (uint32_t *) buffTRndOut;
		rct_restart();
		apt_restart();
		for (i = 0; i < RND_IN_BUFFSIZE / WORD_SIZE_BYTES; i
				+= MIN_INPUT_NUM_WORDS) {
			for (j = 0; j < MIN_INPUT_NUM_WORDS; j++) {
				srcToHash[j] = ((uint32_t *) buffRndIn)[i + j];
			}
			sha256_stampSerialNumber(srcToHash);
			sha256_generateHash(srcToHash, MIN_INPUT_NUM_WORDS + 1, dst);
			dst += OUT_NUM_WORDS;
		}
		curTrngOutIdx = 0;
		for (i = 0; i < TRND_OUT_BUFFSIZE; i++) {
			rct_sample(buffTRndOut[i]);
			apt_sample(buffTRndOut[i]);
		}

		if (rct.statusByte != SWRNG_SUCCESS) {
			swrng_printErrorMessage("Repetition Count Test failure");
			retval = -EPERM;
		} else if (apt.statusByte != SWRNG_SUCCESS) {
			swrng_printErrorMessage("Adaptive Proportion Test failure");
			retval = -EPERM;
		}
	}

	return retval;
}

/**
 * A function to request new entropy bytes when running out of entropy in the local buffer
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int swrng_getEntropyBytes(void) {
	int status;
	if (curTrngOutIdx >= TRND_OUT_BUFFSIZE) {
		status = swrng_rcv_rnd_bytes();
	} else {
		status = SWRNG_SUCCESS;
	}
	return status;
}

static void rct_initialize(void) {
	memset(&rct, 0x00, sizeof(rct));
	rct.statusByte = 0;
	rct.signature = 1;
	rct.maxRepetitions = 5;
	rct_restart();
}

static void rct_restart(void) {
	rct.isInitialized = SWRNG_FALSE;
	rct.curRepetitions = 1;
	rct.failureWindow = 0;
	rct.failureCount = 0;
}

static void rct_sample(uint8_t value) {
	if (!rct.isInitialized) {
		rct.isInitialized = SWRNG_TRUE;
		rct.lastSample = value;
	} else {
		if (rct.lastSample == value) {
			rct.curRepetitions++;
			if (rct.curRepetitions >= rct.maxRepetitions) {
				rct.curRepetitions = 1;
				if (++rct.failureCount >= numConsecFailThreshold) {
					if (rct.statusByte == 0) {
						rct.statusByte = rct.signature;
					}
				}
			}

		} else {
			rct.lastSample = value;
			rct.curRepetitions = 1;
		}
	}

}

static void apt_initialize(void) {
	memset(&apt, 0x00, sizeof(apt));
	apt.statusByte = 0;
	apt.signature = 2;
	apt.windoiwSize = 64;
	apt.cutoffValue = 5;
	apt_restart();
}

static void apt_restart(void) {
	apt.isInitialized = SWRNG_FALSE;
	apt_restart_cycle();
}

static void apt_restart_cycle(void) {
	apt.cycleFailures = 0;
}

static void apt_sample(uint8_t value) {
	if (!apt.isInitialized) {
		apt.isInitialized = SWRNG_TRUE;
		apt.firstSample = value;
		apt.curRepetitions = 0;
		apt.curSamples = 0;
	} else {
		if (++apt.curSamples >= apt.windoiwSize) {
			apt.isInitialized = SWRNG_FALSE;
		}
		if (apt.firstSample == value) {
			if (++apt.curRepetitions > apt.cutoffValue) {
				// Check to see if we have reached the failure threshold
				if (++apt.cycleFailures >= numConsecFailThreshold) {
					if (apt.statusByte == 0) {
						apt.statusByte = apt.signature;
					}
				}
			} else {
				apt_restart_cycle();
			}
		}
	}
}

/**
 * A function to retrieve random bytes
 * @param insigned char *buffer - a pointer to the data receive buffer
 * @param long length - how many bytes expected to receive
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */

int swrngGetEntropy(unsigned char *buffer, long length) {
	int retval = SWRNG_SUCCESS;
	long act;
	long total;

	if (length > MAX_REQUEST_SIZE_BYTES) {
		retval = -EPERM;
	} else if (!deviceOpen) {
		retval = -ENODATA;
	} else {
		total = 0;
		do {
			if (curTrngOutIdx >= TRND_OUT_BUFFSIZE) {
				retval = swrng_getEntropyBytes();
			}
			if (retval == SWRNG_SUCCESS) {
				act = TRND_OUT_BUFFSIZE - curTrngOutIdx;
				if (act > (length - total)) {
					act = (length - total);
				}
				memcpy(buffer + total, buffTRndOut + curTrngOutIdx, act);
				curTrngOutIdx += act;
				total += act;
			} else {
				break;
			}
		} while (total < length);
#ifdef inDebugMode
		if (total > length) {
			fprintf(stderr, "Expected %d bytes to read and actually got %d \n", (int)length, (int)total);
		}
#endif

	}
	return retval;
}

/**
 * Retrieve a complete list of SwiftRNG adevices currently plugged into USB ports
 *
 * @param DeviceInfoList* devInfoList - pointer to the structure for holding SwiftRNG
 * @return int value 0 if processed - successfully
 *
 */
int swrngGetDeviceList(DeviceInfoList *devInfoList) {

	libusb_device * *devs;
	libusb_device * dev;

	if (deviceOpen == SWRNG_TRUE) {
		swrng_printErrorMessage("Cannot invoke listDevices when there is a USB session in progress");
		return -1;
	}

	if (libusbInitialized == SWRNG_FALSE) {
		int r = libusb_init(NULL);
		if (r < 0) {
			swrng_printErrorMessage(LIBUSB_INIT_FAILURE);
			return r;
		}
		libusbInitialized = SWRNG_TRUE;
	}

	memset((void*) devInfoList, 0, sizeof(DeviceInfoList));
	ssize_t cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0) {
		return cnt;
	}
	int i = -1;
	int curFoundDevNum = 0;
	while ((dev = devs[++i]) != NULL) {
		if (i > 127) {
			libusb_free_device_list(devs, 1);
			// This should never happen, cannot have more than 127 USB devices
			swrng_printErrorMessage(TOO_MANY_DEVICES);
			return -1;
		}
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			libusb_free_device_list(devs, 1);
			swrng_printErrorMessage(CANNOT_READ_DEVICE_DESCRIPTOR);
			return r;
		}
		uint16_t idVendorCur = desc.idVendor;
		uint16_t idProductCur = desc.idProduct;
		if (idVendorCur == VENDOR_ID && idProductCur == PRODUCT_ID) {
			devInfoList->devInfoList[curFoundDevNum].devNum = curFoundDevNum;
			if (swrngOpen(curFoundDevNum) == 0) {
				swrngGetModel(&devInfoList->devInfoList[curFoundDevNum].dm);
				swrngGetVersion(&devInfoList->devInfoList[curFoundDevNum].dv);
				swrngGetSerialNumber(
						&devInfoList->devInfoList[curFoundDevNum].sn);
			}
			swrngClose();
			devInfoList->numDevs++;
			curFoundDevNum++;
		}
	}
	libusb_free_device_list(devs, 1);
	return 0;
}

/**
 * Retrieve SwiftRNG device serial number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param DeviceSerialNumber* serialNumber - pointer to a structure for holding SwiftRNG device S/N
 * @return int - 0 when serial number retrieved successfully
 *
 */
int swrngGetSerialNumber(DeviceSerialNumber *serialNumber) {
	if (deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(DEV_NOT_FOUND_MSG);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data((char*) "s", 1, serialNumber->value,
			sizeof(DeviceSerialNumber) - 1, USB_READ_TIMEOUT_SECS);
	// Replace status byte with \0 for ASCIIZ
	serialNumber->value[sizeof(DeviceSerialNumber) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage("Could not retrieve device serial number");
	}
	return resp;
}

/**
 * Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param DeviceVersion* version - pointer to a structure for holding SwiftRNG device version
 * @return int - 0 when version retrieved successfully
 *
 */
int swrngGetVersion(DeviceVersion *version) {
	if (deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(DEV_NOT_FOUND_MSG);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data((char*) "v", 1, version->value,
			sizeof(DeviceVersion) - 1, USB_READ_TIMEOUT_SECS);
	// Replace status byte with \0 for ASCIIZ
	version->value[sizeof(DeviceVersion) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage("Could not retrieve device version");
	}
	return resp;
}

int swrngSetPowerProfile(int ppNum) {

	char ppn[1];

	if (deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(DEV_NOT_FOUND_MSG);
		return -1;
	}

	ppn[0] = '0' + ppNum;

	int resp = swrng_snd_rcv_usb_data(ppn, 1, ppn, 0, USB_READ_TIMEOUT_SECS);
	if (resp != 0) {
		swrng_printErrorMessage("Could not set power profile");
	}
	return resp;
}

/**
 * Retrieve SwiftRNG device model number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param DeviceModel* model - pointer to a structure for holding SwiftRNG device model number
 * @return int 0 - when model retrieved successfully
 *
 */
int swrngGetModel(DeviceModel *model) {
	if (deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(DEV_NOT_FOUND_MSG);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data((char*) "m", 1, model->value,
			sizeof(DeviceModel) - 1, USB_READ_TIMEOUT_SECS);
	// Replace status byte with \0 for ASCIIZ
	model->value[sizeof(DeviceModel) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage("Could not retrieve device model");
	}
	return resp;
}

/**
 * Initialize the SHA256 data
 *
 */
static void sha256_initialize(void) {
	// Initialize H0, H1, H2, H3, H4, H5, H6 and H7
	sd.h0 = 0x6a09e667;
	sd.h1 = 0xbb67ae85;
	sd.h2 = 0x3c6ef372;
	sd.h3 = 0xa54ff53a;
	sd.h4 = 0x510e527f;
	sd.h5 = 0x9b05688c;
	sd.h6 = 0x1f83d9ab;
	sd.h7 = 0x5be0cd19;
}

/**
 * Stamp a new serial number for the input data block into the last word
 *
 * @param void* inputBlock pointer to the input hashing block
 *
 */
static void sha256_stampSerialNumber(void *inputBlock) {
	uint32_t *inw = (uint32_t*) inputBlock;
	inw[MIN_INPUT_NUM_WORDS] = sd.blockSerialNumber++;
}

/**
 * Initialize the serial number for hashing
 *
 * @param uint32_t initValue - a startup random number for generating serial number for hashing
 *
 */
static void sha256_initializeSerialNumber(uint32_t initValue) {
	sd.blockSerialNumber = initValue;
}

/**
 * Generate SHA256 value.
 *
 * @param uint32_t* src - pointer to an array of 32 bit words used as hash input
 * @param uint32_t dst - pointer to an array of 8 X 32 bit words used as hash output
 * @param int16_t len - number of 32 bit words available in array pointed by 'src'
 *
 * @return int 0 for successful operation, -1 for invalid parameters
 *
 */
static int sha256_generateHash(uint32_t *src, int16_t len, uint32_t *dst) {

	uint16_t blockNum;
	uint8_t ui8;
	int32_t initialMessageSize;
	uint16_t numCompleteDataBlocks;
	uint16_t reminder;
	uint16_t srcOffset;
	uint8_t needAdditionalBlock;
	uint8_t needToAddOneMarker;

	if (len <= 0) {
		return -1;
	}

	sha256_initialize();

	initialMessageSize = len * 8 * 4;
	numCompleteDataBlocks = len / maxDataBlockSizeWords;
	reminder = len % maxDataBlockSizeWords;

	// Process complete blocks
	for (blockNum = 0; blockNum < numCompleteDataBlocks; blockNum++) {
		srcOffset = blockNum * maxDataBlockSizeWords;
		for (ui8 = 0; ui8 < maxDataBlockSizeWords; ui8++) {
			sd.w[ui8] = src[ui8 + srcOffset];
		}
		// Hash the current block
		sha256_hashCurrentBlock();
	}

	srcOffset = numCompleteDataBlocks * maxDataBlockSizeWords;
	needAdditionalBlock = 1;
	needToAddOneMarker = 1;
	if (reminder > 0) {
		// Process the last data block if any
		ui8 = 0;
		for (; ui8 < reminder; ui8++) {
			sd.w[ui8] = src[ui8 + srcOffset];
		}
		// Append '1' to the message
		sd.w[ui8++] = 0x80000000;
		needToAddOneMarker = 0;
		if (ui8 < maxDataBlockSizeWords - 1) {
			for (; ui8 < maxDataBlockSizeWords - 2; ui8++) {
				// Fill with zeros
				sd.w[ui8] = 0x0;
			}
			// add the message size to the current block
			sd.w[ui8++] = 0x0;
			sd.w[ui8] = initialMessageSize;
			sha256_hashCurrentBlock();
			needAdditionalBlock = 0;
		} else {
			// Fill the rest with '0'
			// Will need to create another block
			sd.w[ui8] = 0x0;
			sha256_hashCurrentBlock();
		}
	}

	if (needAdditionalBlock) {
		ui8 = 0;
		if (needToAddOneMarker) {
			sd.w[ui8++] = 0x80000000;
		}
		for (; ui8 < maxDataBlockSizeWords - 2; ui8++) {
			sd.w[ui8] = 0x0;
		}
		sd.w[ui8++] = 0x0;
		sd.w[ui8] = initialMessageSize;
		sha256_hashCurrentBlock();
	}

	// Save the results
	dst[0] = sd.h0;
	dst[1] = sd.h1;
	dst[2] = sd.h2;
	dst[3] = sd.h3;
	dst[4] = sd.h4;
	dst[5] = sd.h5;
	dst[6] = sd.h6;
	dst[7] = sd.h7;

	return 0;
}

/**
 * Hash current block
 *
 */
static void sha256_hashCurrentBlock(void) {
	uint8_t t;

	// Process elements 16...63
	for (t = 16; t <= 63; t++) {
		sd.w[t] = sha256_sigma1(&sd.w[t - 2]) + sd.w[t - 7] + sha256_sigma0(
				&sd.w[t - 15]) + sd.w[t - 16];
	}

	// Initialize variables
	sd.a = sd.h0;
	sd.b = sd.h1;
	sd.c = sd.h2;
	sd.d = sd.h3;
	sd.e = sd.h4;
	sd.f = sd.h5;
	sd.g = sd.h6;
	sd.h = sd.h7;

	// Process elements 0...63
	for (t = 0; t <= 63; t++) {
		sd.tmp1 = sd.h + sha256_sum1(&sd.e) + sha256_ch(&sd.e, &sd.f, &sd.g)
				+ k[t] + sd.w[t];
		sd.tmp2 = sha256_sum0(&sd.a) + sha256_maj(&sd.a, &sd.b, &sd.c);
		sd.h = sd.g;
		sd.g = sd.f;
		sd.f = sd.e;
		sd.e = sd.d + sd.tmp1;
		sd.d = sd.c;
		sd.c = sd.b;
		sd.b = sd.a;
		sd.a = sd.tmp1 + sd.tmp2;
	}

	// Calculate the final hash for the block
	sd.h0 += sd.a;
	sd.h1 += sd.b;
	sd.h2 += sd.c;
	sd.h3 += sd.d;
	sd.h4 += sd.e;
	sd.h5 += sd.f;
	sd.h6 += sd.g;
	sd.h7 += sd.h;
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.2)
 *
 * @param uint32_t* x pointer to variable x
 * @param uint32_t* y pointer to variable y
 * @param uint32_t* z pointer to variable z
 * $return uint32_t Ch value
 *
 */
static uint32_t sha256_ch(uint32_t *x, uint32_t *y, uint32_t *z) {
	return ((*x) & (*y)) ^ (~(*x) & (*z));
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.3)
 *
 * @param uint32_t* x pointer to variable x
 * @param uint32_t* y pointer to variable y
 * @param uint32_t* z pointer to variable z
 * $return uint32_t Maj value
 *
 */
static uint32_t sha256_maj(uint32_t *x, uint32_t *y, uint32_t *z) {
	return ((*x) & (*y)) ^ ((*x) & (*z)) ^ ((*y) & (*z));
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.4)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t Sum0 value
 *
 */
static uint32_t sha256_sum0(uint32_t *x) {
	return ROTR(2, *x) ^ ROTR(13, *x) ^ ROTR(22, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.5)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t Sum1 value
 *
 */
static uint32_t sha256_sum1(uint32_t *x) {
	return ROTR(6, *x) ^ ROTR(11, *x) ^ ROTR(25, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.6)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t sigma0 value
 *
 */
static uint32_t sha256_sigma0(uint32_t *x) {
	return ROTR(7, *x) ^ ROTR(18, *x) ^ ((*x) >> 3);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.7)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t sigma1 value
 *
 */
static uint32_t sha256_sigma1(uint32_t *x) {
	return ROTR(17, *x) ^ ROTR(19, *x) ^ ((*x) >> 10);
}


/*
 * A function to retrieve random bytes
 * @param insigned char *buffer - a pointer to the data receive buffer
 * @param long length - how many bytes expected to receive
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int sha256_selfTest(void) {
       uint32_t results[8];
       int retVal;

       retVal = sha256_generateHash((uint32_t*) testSeq1, (uint16_t) 11,
                       (uint32_t*) results);
       if (retVal == 0) {
               // Compare the expected with actual results
              retVal = memcmp(results, exptHashSeq1, 8);
       }
       return retVal;
}

/**
 * Generate and retrieve device statistics
 *
 */
DeviceStatistics* swrngGenerateDeviceStatistics() {
	time(&ds.endTime); // Stop performance timer
	// Calculate performance
	ds.totalTime = ds.endTime - ds.beginTime;
	if (ds.totalTime == 0) {
		ds.totalTime = 1;
	}
	ds.downloadSpeedKBsec = (int) (ds.numGenBytes / (int64_t) 1024
			/ (int64_t) ds.totalTime);
	return &ds;
}

/**
 * Reset statistics for the SWRNG device
 *
 */
void swrngResetStatistics() {
	time(&ds.beginTime); // Start performance timer
	ds.downloadSpeedKBsec = 0;
	ds.numGenBytes = 0;
	ds.totalRetries = 0;
	ds.endTime = 0;
	ds.totalTime = 0;
}

/**
 * Retrieve the last error messages
 * @return - pointer to the error message
 */
const char* swrngGetLastErrorMessage() {

	if (lastError == (char*)NULL) {
		strcpy(lastError, "");
	}
	return lastError;
}

/**
 * Call this methods to enable printing error messages to standard error
 */
void swrngEnablePrintingErrorMessages() {
	enPrintErrorMessages = SWRNG_TRUE;
}

/**
 * Print and/or save error message
 * @param errMsg - pointer to error message
 */
static void swrng_printErrorMessage(const char* errMsg) {
	if (enPrintErrorMessages) {
		fprintf(stderr, "%s", errMsg);
		fprintf(stderr, "\n");
	}
	if (strlen(errMsg) >= sizeof(lastError)) {
		strcpy(lastError, "Error message too long");
	} else {
		strcpy(lastError, errMsg);
	}
}
