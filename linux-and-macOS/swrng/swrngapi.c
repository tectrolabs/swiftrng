#include "stdafx.h"

/*
 * swrngapi.c
 * ver. 3.7
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

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

#include "swrngapi.h"

#define SWRNG_START_CONTEXT_SIGNATURE 18392
#define SWRNG_END_CONTEXT_SIGNATURE 30816
#define SWRNG_DEV_NOT_OPEN_MSG "Device not open"
#define SWRNG_FREQ_TABLE_INVALID_MSG "Invalid FrequencyTables pointer argument"
#define SWRNG_CANNOT_DISABLE_PP_MSG "Post processing cannot be disabled for device"
#define SWRNG_PP_OP_NOT_SUPPORTED_MSG "Post processing method not supported for device"
#define SWRNG_DIAG_OP_NOT_SUPPORTED_MSG "Diagnostics not supported for device"
#define SWRNG_PP_METHOD_NOT_SUPPORTED_MSG "Post processing method not supported"
#define SWRNG_CANNOT_GET_FREQ_TABLE_FOR_DEVICE_MSG "Frequency tables can only be retrieved for devices with version 1.2 and up"
#define SWRNG_TOO_MANY_DEVICES "Cannot have more than 127 USB devices"
#define SWRNG_CANNOT_READ_DEVICE_DESCRIPTOR "Failed to retrieve USB device descriptor"
#define SWRNG_LIBUSB_INIT_FAILURE "Failed to initialize libusb"
#define SWRNG_VENDOR_ID (0x1fc9)
#define SWRNG_PRODUCT_ID (0x8110)	// This product ID is only to be used with SwiftRNG device
#define SWRNG_PRODUCT_ID2 (0x8111)	// This product ID is only to be used with SwiftRNG device
#define SWRNG_BULK_EP_OUT     0x01
#define SWRNG_BULK_EP_IN      0x81

#define SWRNG_USB_READ_MAX_RETRY_CNT	(15)
#define SWRNG_USB_READ_TIMEOUT_SECS	(2)
#define SWRNG_USB_BULK_READ_TIMEOUT_MLSECS	(100)
#define SWRNG_MAX_REQUEST_SIZE_BYTES (100000L)

static const char ctxtNotInitializedErrMsg[] = "Context not initialized";

//
// SHA256 data variable section
//
#define ROTR32(sb,w) (((w) >> (sb)) | ((w) << (32-(sb))))

static const uint32_t sha256_k[64] =
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

static const uint32_t sha256_testSeq1[11] = { 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0x428a2f98,
		0x71374491, 0xb5c0fbcf };

static const uint32_t sha256_exptHashSeq1[8] = { 0x114c3052, 0x76410592, 0xc024566b,
		0xa492b1a2, 0xb0559389, 0xb7c41156, 0x2ec8d6c3, 0x3dcb02dd };


//
// SHA512 data variable section
//
#define ROTR64(sb,w) (((w) >> (sb)) | ((w) << (64-(sb))))

static const uint64_t sha512_k[80] = {
	0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
	0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
	0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
	0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
	0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
	0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
	0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
	0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
	0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
	0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
	0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
	0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
	0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
	0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
	0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
	0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
	0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
	0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
	0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
	0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
};

static const uint64_t sha512_exptHashSeq1[8] = { 0x6cbce8f347e8d1b3, 0xd3517b27fdc4ee1c, 0x71d8406ab54e2335,
		0xf3a39732fa0009d2, 0x2193c41677d18504, 0xe90b4c1138c32e7c, 0xc1aa7500597ba99c, 0xacd525ef2c44e9dc };




static const uint8_t maxDataBlockSizeWords = 16;

//
// Declarations for local functions
//

static void swrng_updateDevInfoList(DeviceInfoList* devInfoList, int* curFoundDevNum);
static int swrng_handleDeviceVersion(SwrngContext* ctxt);
static void swrng_contextReset(SwrngContext* ctxt);
static void closeUSBLib(SwrngContext *ctxt);
static swrngBool isContextInitialized(SwrngContext *ctxt);
static void swrng_printErrorMessage(SwrngContext *ctxt, const char* errMsg);
static void swrng_clearReceiverBuffer(SwrngContext *ctxt);

static uint64_t xorshift64_postProcessWord(uint64_t rawWord);
static void xorshift64_postProcessWords(uint64_t *buffer, int numElements);
static void xorshift64_postProcess(uint8_t *buffer, int numElements);
static int xorshift64_selfTest(SwrngContext *ctxt);

static void sha256_initialize(SwrngContext *ctxt);
static void sha256_stampSerialNumber(SwrngContext *ctxt, void *inputBlock);
static void sha256_initializeSerialNumber(SwrngContext *ctxt, uint32_t initValue);
static int sha256_generateHash(SwrngContext *ctxt, uint32_t *src, int16_t len, uint32_t *dst);
static void sha256_hashCurrentBlock(SwrngContext *ctxt);

static uint32_t sha256_ch(uint32_t *x, uint32_t *y, uint32_t *z);
static uint32_t sha256_maj(uint32_t *x, uint32_t *y, uint32_t *z);
static uint32_t sha256_sum0(uint32_t *x);
static uint32_t sha256_sum1(uint32_t *x);
static uint32_t sha256_sigma0(uint32_t *x);
static uint32_t sha256_sigma1(uint32_t *x);
static int sha256_selfTest(SwrngContext *ctxt);


static void sha512_initialize(SwrngContext *ctxt);
static int sha512_generateHash(SwrngContext *ctxt, uint64_t *src, int16_t len, uint64_t *dst);
static void sha512_hashCurrentBlock(SwrngContext *ctxt);

static uint64_t sha512_ch(uint64_t *x, uint64_t *y, uint64_t *z);
static uint64_t sha512_maj(uint64_t *x, uint64_t *y, uint64_t *z);
static uint64_t sha512_sum0(uint64_t *x);
static uint64_t sha512_sum1(uint64_t *x);
static uint64_t sha512_sigma0(uint64_t *x);
static uint64_t sha512_sigma1(uint64_t *x);
static int sha512_selfTest(SwrngContext *ctxt);

static void test_samples(SwrngContext *ctxt);
static void rct_initialize(SwrngContext *ctxt);
static void rct_restart(SwrngContext *ctxt);

static void apt_initialize(SwrngContext *ctxt);
static void apt_restart(SwrngContext *ctxt);


/**
 * Close device if open
 *
 * @param ctxt - pointer to SwrngContext structure
 * @return int - 0 when processed successfully
 */
int swrngClose(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

#ifdef _WIN32
	if (ctxt->usbComPort != NULL) {
		ctxt->usbComPort->disconnect();
		delete ctxt->usbComPort;
		ctxt->usbComPort = NULL;
	}
#endif
	closeUSBLib(ctxt);
	ctxt->deviceOpen = SWRNG_FALSE;
	return SWRNG_SUCCESS;
}

static void swrng_contextReset(SwrngContext* ctxt) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return;
	}
#ifdef _WIN32
	if (ctxt->usbComPort != NULL) {
		ctxt->usbComPort->disconnect();
	}
#endif
	closeUSBLib(ctxt);
	strcpy(ctxt->lastError, "");
	ctxt->deviceOpen = SWRNG_FALSE;
}

/**
 * @param ctxt - pointer to SwrngContext structure
 * Close libusb 
 *
 */
static void closeUSBLib(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return;
	}

	if (ctxt->devh) {
		libusb_release_interface(ctxt->devh, 0); //release the claimed interface
		libusb_close(ctxt->devh);
		ctxt->devh = NULL;
	}
	if (ctxt->devs) {
		libusb_free_device_list(ctxt->devs, 1);
		ctxt->devs = NULL;
	}
	if (ctxt->libusbInitialized == SWRNG_TRUE) {
		libusb_exit(ctxt->luctx);
		ctxt->luctx = NULL;
		ctxt->libusbInitialized = SWRNG_FALSE;
	}
}

/**
* Check if device is open
*
* @param ctxt - pointer to SwrngContext structure
* @return int - 0 when device is open
*/
int swrngIsOpen(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return SWRNG_FALSE;
	}
	return ctxt->deviceOpen;
}

/**
 * A function to handle SW device receive command
 * @param ctxt - pointer to SwrngContext structure
 * @param char *buff - a pointer to the data receive buffer
 * @param int length - how many bytes expected to receive
 * @param int opTimeoutSecs - device read time out value in seconds
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int swrng_chip_read_data(SwrngContext *ctxt, char *buff, int length, int opTimeoutSecs) {
	long secsWaited;
	int transferred;
	time_t start, end;
	int cnt;
	int i;
	int retval;

	start = time(NULL);

	cnt = 0;
	do {
#ifdef _WIN32
		if (ctxt->usbComPort->isConnected()) {
			retval = ctxt->usbComPort->receiveDeviceData(ctxt->bulk_in_buffer, length, &transferred);
		}
		else {
#endif
			retval = libusb_bulk_transfer(ctxt->devh, SWRNG_BULK_EP_IN, ctxt->bulk_in_buffer, length, &transferred, SWRNG_USB_BULK_READ_TIMEOUT_MLSECS);
#ifdef _WIN32
		}
#endif

#ifdef inDebugMode
		fprintf(stderr, "chip_read_data retval %d transferred %d, length %d\n", retval, transferred, length);
#endif
		if (retval) {
			return retval;
		}

		if (transferred > length) {
			swrng_printErrorMessage(ctxt, "Received unexpected bytes when processing USB device request");
			return -EFAULT;
		}

		end = time(NULL);
		secsWaited = (long)(end - start);
		if (transferred > 0) {
			for (i = 0; i < transferred; i++) {
				buff[cnt++] = ctxt->bulk_in_buffer[i];
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
 * @param ctxt - pointer to SwrngContext structure
 * @param char *snd -  a pointer to the command
 * @param int sizeSnd - how many bytes in command
 * @param char *rcv - a pointer to the data receive buffer
 * @param int sizeRcv - how many bytes expected to receive
 * @param int opTimeoutSecs - device read time out value in seconds
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int swrng_snd_rcv_usb_data(SwrngContext *ctxt, char *snd, int sizeSnd, char *rcv, int sizeRcv,
		int opTimeoutSecs) {
	int retry;
	int actualcCnt;
	int retval = SWRNG_SUCCESS;

	for (retry = 0; retry < SWRNG_USB_READ_MAX_RETRY_CNT; retry++) {
#ifdef _WIN32
		if (ctxt->usbComPort->isConnected()) {
			retval = ctxt->usbComPort->sendCommand(snd, sizeSnd, &actualcCnt);
		}
		else {
#endif
			retval = libusb_bulk_transfer(ctxt->devh, SWRNG_BULK_EP_OUT, (unsigned char*)snd, sizeSnd, &actualcCnt, 100);
#ifdef _WIN32
		}
#endif
		if (retval == SWRNG_SUCCESS && actualcCnt == sizeSnd) {
			retval = swrng_chip_read_data(ctxt, rcv, sizeRcv + 1, opTimeoutSecs);
			if (retval == SWRNG_SUCCESS) {
				if (rcv[sizeRcv] != 0) {
					retval = -EFAULT;
				} else {
					ctxt->ds.numGenBytes += sizeRcv;
					break;
				}
			}
		}
		#ifdef inDebugMode
			fprintf(stderr, "It was an error during data communication. Cleaning up the receiving queue and continue.\n");
		#endif
			ctxt->ds.totalRetries++;
		swrng_chip_read_data(ctxt, ctxt->buffRndIn, SWRNG_RND_IN_BUFFSIZE + 1, opTimeoutSecs);
	}
	if (retry >= SWRNG_USB_READ_MAX_RETRY_CNT) {
		if (retval == SWRNG_SUCCESS) {
			retval = -ETIMEDOUT;
		}
	}
	return retval;
}

/**
 * Open SwiftRNG USB specific device.
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param int devNum - device number (0 - for the first device or when there is only one device connected)
 * @return int 0 - when open successfully or error code
 */
int swrngOpen(SwrngContext *ctxt, int devNum) {
	int ret;
	int actualDeviceNum = devNum;
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		swrngInitializeContext(ctxt);
	}
	else {
		swrng_contextReset(ctxt);
	}

#ifdef _WIN32
	if (ctxt->usbComPort == NULL) {
		ctxt->usbComPort = new USBComPort;
		ctxt->usbComPort->initlalize();
	}
#endif


	rct_initialize(ctxt);
	apt_initialize(ctxt);

	sha256_initializeSerialNumber(ctxt, (uint32_t)ctxt->ds.beginTime);
	if (sha256_selfTest(ctxt) != SWRNG_SUCCESS) {
		swrng_printErrorMessage(ctxt, "SHA256 post processing logic failed the self-test");
		return -EPERM;
	}

	if (sha512_selfTest(ctxt) != SWRNG_SUCCESS) {
		swrng_printErrorMessage(ctxt, "SHA512 post processing logic failed the self-test");
		return -EPERM;
	}

	if (xorshift64_selfTest(ctxt) != SWRNG_SUCCESS) {
		swrng_printErrorMessage(ctxt, "Xorshift64 post processing logic failed the self-test");
		return -EPERM;
	}

#if defined(_WIN32)
	int portsConnected;
	int ports[SWRNG_MAX_CDC_COM_PORTS];
	// Retrieve devices connected as USB CDC in Windows
	ctxt->usbComPort->getConnectedPorts(ports, SWRNG_MAX_CDC_COM_PORTS, &portsConnected, SWRNG_HDWARE_ID);
	if (portsConnected > devNum) {
		WCHAR portName[80];
		ctxt->usbComPort->toPortName(ports[devNum], portName, 80);
		bool comPortStatus = ctxt->usbComPort->connect(portName);
		if (!comPortStatus) {
			swrng_printErrorMessage(ctxt, ctxt->usbComPort->getLastErrMsg());
			return -1;
		}
		else {
			return swrng_handleDeviceVersion(ctxt);
		}
	}
	else {
		actualDeviceNum -= portsConnected;
	}
#endif

	if (ctxt->libusbInitialized == SWRNG_FALSE) {
		int r = libusb_init(&ctxt->luctx);
		if (r < 0) {
			closeUSBLib(ctxt);
			swrng_printErrorMessage(ctxt, SWRNG_LIBUSB_INIT_FAILURE);
			return r;
		}
		ctxt->libusbInitialized = SWRNG_TRUE;
	}

	int curFoundDevNum = -1;
	ssize_t cnt = libusb_get_device_list(ctxt->luctx, &ctxt->devs);
	if (cnt < 0) {
		closeUSBLib(ctxt);
		return cnt;
	}
	int i = 0;
	while ((ctxt->dev = ctxt->devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(ctxt->dev, &desc);
		if (ret < 0) {
			closeUSBLib(ctxt);
			swrng_printErrorMessage(ctxt, SWRNG_CANNOT_READ_DEVICE_DESCRIPTOR);
			return ret;
		}
		uint16_t idVendorCur = desc.idVendor;
		uint16_t idProductCur = desc.idProduct;
#if defined(_WIN32)
		if (idVendorCur == SWRNG_VENDOR_ID && idProductCur == SWRNG_PRODUCT_ID) {
#else
		if (idVendorCur == SWRNG_VENDOR_ID && (idProductCur == SWRNG_PRODUCT_ID || idProductCur == SWRNG_PRODUCT_ID2)) {
#endif
			if (++curFoundDevNum == actualDeviceNum) {
				ret = libusb_open(ctxt->dev, &ctxt->devh);
				switch (ret) {
				case LIBUSB_ERROR_NO_MEM:
					closeUSBLib(ctxt);
					swrng_printErrorMessage(ctxt, "Memory allocation failure");
					return ret;
				case LIBUSB_ERROR_ACCESS:
					closeUSBLib(ctxt);
					swrng_printErrorMessage(ctxt, "User has insufficient permissions");
					return ret;
				case LIBUSB_ERROR_NO_DEVICE:
					closeUSBLib(ctxt);
					swrng_printErrorMessage(ctxt, "Device has been disconnected");
					return ret;
				}
				if (ret < 0) {
					closeUSBLib(ctxt);
					swrng_printErrorMessage(ctxt, "Failed to open USB device");
					return ret;
				}

				if (libusb_kernel_driver_active(ctxt->devh, 0) == 1) { //find out if kernel driver is attached
					if (libusb_detach_kernel_driver(ctxt->devh, 0) != 0) {
						closeUSBLib(ctxt);
						swrng_printErrorMessage(ctxt, "Could not detach kernel driver");
						return -1;
					}
				}
				
				ret = libusb_claim_interface(ctxt->devh, 0);
				if (ret < 0) {
					closeUSBLib(ctxt);
					swrng_printErrorMessage(ctxt, "Cannot claim the USB interface");
					return ret;
				}
				
				return swrng_handleDeviceVersion(ctxt);
			}
		}
	}
	closeUSBLib(ctxt);
	swrng_printErrorMessage(ctxt, "Could not find any SwiftRNG device");
	return -1;
}

static int swrng_handleDeviceVersion(SwrngContext* ctxt) {
	int status;

	// Clear the receive buffer
	swrng_clearReceiverBuffer(ctxt);
	ctxt->deviceOpen = SWRNG_TRUE;

	// Retrieve device version number
	status = swrngGetVersion(ctxt, &ctxt->curDeviceVersion);
	if (status != SWRNG_SUCCESS) {
		closeUSBLib(ctxt);
		return status;
	}
	ctxt->deviceVersionDouble = atof((char*)&ctxt->curDeviceVersion + 1);

	if (ctxt->deviceVersionDouble >= 2.0) {
		// By default, disable post processing for devices with versions 2.0+
		ctxt->postProcessingEnabled = SWRNG_FALSE;
		if (ctxt->deviceVersionDouble >= 3.0) {
			// SwiftRNG Z devices use built-in 'P. Lacharme' Linear Correction method
			ctxt->deviceEmbeddedCorrectionMethodId = SWRNG_EMB_CORR_METHOD_LINEAR;
		}
	}
	else {
		// Adjust APT and RCT tests to count for BIAS with older devices.
		// All tests are performed now before applying post-processing to comply
		// with NIST SP 800-90B (second draft)
		if (ctxt->deviceVersionDouble == 1.1) {
			ctxt->numFailuresThreshold = 6;
		}
		else if (ctxt->deviceVersionDouble == 1.0) {
			ctxt->numFailuresThreshold = 9;
		}
	}

	return SWRNG_SUCCESS;
}

/**
 * Clear potential random bytes from the receiver buffer if any
 * @param ctxt - pointer to SwrngContext structure
 */
static void swrng_clearReceiverBuffer(SwrngContext *ctxt) {
	int transferred;
	int retval;
	int i;
	for(i = 0; i < 10; i++) {

#ifdef _WIN32
		if (ctxt->usbComPort->isConnected()) {
			retval = ctxt->usbComPort->receiveDeviceData(ctxt->bulk_in_buffer, SWRNG_RND_IN_BUFFSIZE + 1, &transferred);
		}
		else {
#endif
			retval = libusb_bulk_transfer(ctxt->devh, SWRNG_BULK_EP_IN, ctxt->bulk_in_buffer, SWRNG_RND_IN_BUFFSIZE + 1, &transferred, SWRNG_USB_BULK_READ_TIMEOUT_MLSECS);
#ifdef _WIN32
		}
#endif



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
 * @param ctxt - pointer to SwrngContext structure
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int swrng_rcv_rnd_bytes(SwrngContext *ctxt) {
	int retval;
	int i, j;
	uint32_t *dst32, *src32;
	uint64_t *dst64, *src64;

	if (!ctxt->deviceOpen) {
		return -EPERM;
	}

	ctxt->bulk_out_buffer[0] = 'x';

	retval = swrng_snd_rcv_usb_data(ctxt, (char *)ctxt->bulk_out_buffer, 1, ctxt->buffRndIn,
			SWRNG_RND_IN_BUFFSIZE, SWRNG_USB_READ_TIMEOUT_SECS);
	if (retval == SWRNG_SUCCESS) {
		if (ctxt->statisticalTestsEnabled == SWRNG_TRUE) {
			rct_restart(ctxt);
			apt_restart(ctxt);
			test_samples(ctxt);
		}
		if (ctxt->postProcessingEnabled == SWRNG_TRUE) {
			if (ctxt->postProcessingMethodId == SWRNG_SHA256_PP_METHOD) {
				dst32 = (uint32_t *)ctxt->buffTRndOut;
				src32 = (uint32_t *)ctxt->buffRndIn;
				for (i = 0; i < SWRNG_RND_IN_BUFFSIZE / SWRNG_WORD_SIZE_BYTES; i
						+= SWRNG_MIN_INPUT_NUM_WORDS) {
					for (j = 0; j < SWRNG_MIN_INPUT_NUM_WORDS; j++) {
						ctxt->srcToHash32[j] = src32[i + j];
					}
					sha256_stampSerialNumber(ctxt, ctxt->srcToHash32);
					sha256_generateHash(ctxt, ctxt->srcToHash32, SWRNG_MIN_INPUT_NUM_WORDS + 1, dst32);
					dst32 += SWRNG_OUT_NUM_WORDS;
				}
			} else if (ctxt->postProcessingMethodId == SWRNG_SHA512_PP_METHOD) {
				dst64 = (uint64_t *)ctxt->buffTRndOut;
				src64 = (uint64_t *)ctxt->buffRndIn;
				for (i = 0; i < SWRNG_RND_IN_BUFFSIZE / (SWRNG_WORD_SIZE_BYTES * 2); i
						+= SWRNG_MIN_INPUT_NUM_WORDS) {
					for (j = 0; j < SWRNG_MIN_INPUT_NUM_WORDS; j++) {
						ctxt->srcToHash64[j] = src64[i + j];
					}
					sha512_generateHash(ctxt, ctxt->srcToHash64, SWRNG_MIN_INPUT_NUM_WORDS, dst64);
					dst64 += SWRNG_OUT_NUM_WORDS;
				}
			} else if (ctxt->postProcessingMethodId == SWRNG_XORSHIFT64_PP_METHOD) {
				memcpy(ctxt->buffTRndOut, ctxt->buffRndIn, SWRNG_TRND_OUT_BUFFSIZE);
				xorshift64_postProcess((uint8_t *)ctxt->buffTRndOut, SWRNG_TRND_OUT_BUFFSIZE);
			} else {
				swrng_printErrorMessage(ctxt, SWRNG_PP_OP_NOT_SUPPORTED_MSG);
				return -1;
			}
		} else {
			memcpy(ctxt->buffTRndOut, ctxt->buffRndIn, SWRNG_TRND_OUT_BUFFSIZE);
		}
		ctxt->curTrngOutIdx = 0;
		if (ctxt->rct.statusByte != SWRNG_SUCCESS) {
			swrng_printErrorMessage(ctxt, "Repetition Count Test failure");
			retval = -EPERM;
		} else if (ctxt->apt.statusByte != SWRNG_SUCCESS) {
			swrng_printErrorMessage(ctxt, "Adaptive Proportion Test failure");
			retval = -EPERM;
		}
	}

	return retval;
}

/**
 * A function to request new entropy bytes when running out of entropy in the local buffer
 *
 * @param ctxt - pointer to SwrngContext structure
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int swrng_getEntropyBytes(SwrngContext *ctxt) {
	int status;
	if (ctxt->curTrngOutIdx >= SWRNG_TRND_OUT_BUFFSIZE) {
		status = swrng_rcv_rnd_bytes(ctxt);
	} else {
		status = SWRNG_SUCCESS;
	}
	return status;
}

/**
 * A function to initialize the repetition count test
 *
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void rct_initialize(SwrngContext *ctxt) {
	memset(&ctxt->rct, 0x00, sizeof(ctxt->rct));
	ctxt->rct.statusByte = 0;
	ctxt->rct.signature = 1;
	ctxt->rct.maxRepetitions = 5;
	rct_restart(ctxt);
}

/**
 * A function to restart the repetition count test
 *
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void rct_restart(SwrngContext *ctxt) {
	ctxt->rct.isInitialized = SWRNG_FALSE;
	ctxt->rct.curRepetitions = 1;
	ctxt->rct.failureCount = 0;
}


/**
 * A function for testing a block of random bytes using 'repetition count'
 * and 'adaptive proportion' tests
 *
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void test_samples(SwrngContext *ctxt) {
	int i;
	uint8_t value;
	for (i = 0; i < SWRNG_TRND_OUT_BUFFSIZE; i++) {
		value = ctxt->buffRndIn[i];

		//
		// Run 'repetition count' test
		//
		if (!ctxt->rct.isInitialized) {
			ctxt->rct.isInitialized = SWRNG_TRUE;
			ctxt->rct.lastSample = value;
		} else {
			if (ctxt->rct.lastSample == value) {
				ctxt->rct.curRepetitions++;
				if (ctxt->rct.curRepetitions >= ctxt->rct.maxRepetitions) {
					ctxt->rct.curRepetitions = 1;
					if (++ctxt->rct.failureCount > ctxt->numFailuresThreshold) {
						if (ctxt->rct.statusByte == 0) {
							ctxt->rct.statusByte = ctxt->rct.signature;
						}
					}

					if (ctxt->rct.failureCount > ctxt->maxRctFailuresPerBlock) {
						// Record the maximum failures per block for reporting
						ctxt->maxRctFailuresPerBlock = ctxt->rct.failureCount;
					}

					#ifdef inDebugMode
					if (ctxt->rct.failureCount >= 1) {
						fprintf(stderr, "rct.failureCount: %d value: %d\n", ctxt->rct.failureCount, value);
					}
					#endif
				}

			} else {
				ctxt->rct.lastSample = value;
				ctxt->rct.curRepetitions = 1;
			}
		}

		//
		// Run 'adaptive proportion' test
		//
		if (!ctxt->apt.isInitialized) {
			ctxt->apt.isInitialized = SWRNG_TRUE;
			ctxt->apt.firstSample = value;
			ctxt->apt.curRepetitions = 0;
			ctxt->apt.curSamples = 0;
		} else {
			if (++ctxt->apt.curSamples >= ctxt->apt.windowSize) {
				ctxt->apt.isInitialized = SWRNG_FALSE;
				if (ctxt->apt.curRepetitions > ctxt->apt.cutoffValue) {
					// Check to see if we have reached the failure threshold
					if (++ctxt->apt.cycleFailures > ctxt->numFailuresThreshold) {
						if (ctxt->apt.statusByte == 0) {
							ctxt->apt.statusByte = ctxt->apt.signature;
						}
					}
					if (ctxt->apt.cycleFailures > ctxt->maxAptFailuresPerBlock) {
						// Record the maximum failures per block for reporting
						ctxt->maxAptFailuresPerBlock = ctxt->apt.cycleFailures;
					}

					#ifdef inDebugMode
					if (ctxt->apt.cycleFailures >= 1) {
						fprintf(stderr, "ctxt->apt.cycleFailures: %d value: %d\n", ctxt->apt.cycleFailures, value);
                    }
					#endif
				}

			} else {
				if (ctxt->apt.firstSample == value) {
					++ctxt->apt.curRepetitions;
				}
			}
		}


	}

}

/**
 * A function to initialize the adaptive proportion test
 *
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void apt_initialize(SwrngContext *ctxt) {
	memset(&ctxt->apt, 0x00, sizeof(ctxt->apt));
	ctxt->apt.statusByte = 0;
	ctxt->apt.signature = 2;
	ctxt->apt.windowSize = 64;
	ctxt->apt.cutoffValue = 5;
	apt_restart(ctxt);
}

/**
 * A function to restart the adaptive proportion test
 *
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void apt_restart(SwrngContext *ctxt) {
	ctxt->apt.isInitialized = SWRNG_FALSE;
	ctxt->apt.cycleFailures = 0;
}

/**
* A function to retrieve up to 100000 random bytes
*
* @param ctxt - pointer to SwrngContext structure
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive, max value is 100000
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetEntropy(SwrngContext *ctxt, unsigned char *buffer, long length) {
	int retval = SWRNG_SUCCESS;
	long act;
	long total;

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (length > SWRNG_MAX_REQUEST_SIZE_BYTES || length < 0) {
		retval = -EPERM;
	} else if (!ctxt->deviceOpen) {
		retval = -ENODATA;
	} else {
		total = 0;
		do {
			if (ctxt->curTrngOutIdx >= SWRNG_TRND_OUT_BUFFSIZE) {
				retval = swrng_getEntropyBytes(ctxt);
			}
			if (retval == SWRNG_SUCCESS) {
				act = SWRNG_TRND_OUT_BUFFSIZE - ctxt->curTrngOutIdx;
				if (act > (length - total)) {
					act = (length - total);
				}
				memcpy(buffer + total, ctxt->buffTRndOut + ctxt->curTrngOutIdx, act);
				ctxt->curTrngOutIdx += act;
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
* This function is an enhanced version of 'swrngGetEntropy'.
* Use it to retrieve more than 100000 random bytes in one call
*
* @param ctxt - pointer to SwrngContext structure
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetEntropyEx(SwrngContext *ctxt, unsigned char *buffer, long length) {
	int retval = SWRNG_SUCCESS;
	unsigned char *dst;
	long totalByteBlocks;
	long leftoverBytes;
	long l;

	if (length <= 0) {
		retval = -EPERM;
	} else {
		totalByteBlocks = length / SWRNG_MAX_REQUEST_SIZE_BYTES;
		leftoverBytes = length % SWRNG_MAX_REQUEST_SIZE_BYTES;
		dst = buffer;
		for (l = 0; l < totalByteBlocks; l++) {
			retval = swrngGetEntropy(ctxt, dst, SWRNG_MAX_REQUEST_SIZE_BYTES);
			if (retval != SWRNG_SUCCESS) {
				break;
			}
			dst += SWRNG_MAX_REQUEST_SIZE_BYTES;
		}
		retval = swrngGetEntropy(ctxt, dst, leftoverBytes);
	}

	return retval;
}

/**
 * Retrieve a complete list of SwiftRNG devices currently plugged into USB ports
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param DeviceInfoList* devInfoList - pointer to the structure for holding SwiftRNG
 * @return int - value 0 when retrieved successfully
 *
 */
int swrngGetDeviceList(SwrngContext *ctxt, DeviceInfoList *devInfoList) {

	libusb_device * *devs;
	libusb_device * dev;

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}
	
	if (ctxt->deviceOpen == SWRNG_TRUE) {
		swrng_printErrorMessage(ctxt, "Cannot invoke listDevices when there is a USB session in progress");
		return -1;
	}

	if (ctxt->libusbInitialized == SWRNG_FALSE) {
		int r = libusb_init(&ctxt->luctx);
		if (r < 0) {
			swrng_printErrorMessage(ctxt, SWRNG_LIBUSB_INIT_FAILURE);
			return r;
		}
		ctxt->libusbInitialized = SWRNG_TRUE;
	}
	
	memset((void*) devInfoList, 0, sizeof(DeviceInfoList));

	int curFoundDevNum = 0;

#if defined(_WIN32)
	USBComPort usbComPort;
	int portsConnected;
	int ports[SWRNG_MAX_CDC_COM_PORTS];
	// Add devices connected as USB CDC in Windows
	usbComPort.getConnectedPorts(ports, SWRNG_MAX_CDC_COM_PORTS, &portsConnected, SWRNG_HDWARE_ID);
	while (portsConnected-- > 0) {
		swrng_updateDevInfoList(devInfoList, &curFoundDevNum);
	}
#endif

	ssize_t cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0) {
		closeUSBLib(ctxt);
		return cnt;
	}
	int i = -1;
	while ((dev = devs[++i]) != NULL) {
		if (i > 127) {
			libusb_free_device_list(devs, 1);
			closeUSBLib(ctxt);
			// This should never happen, cannot have more than 127 USB devices
			swrng_printErrorMessage(ctxt, SWRNG_TOO_MANY_DEVICES);
			return -1;
		}
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			libusb_free_device_list(devs, 1);
			closeUSBLib(ctxt);
			swrng_printErrorMessage(ctxt, SWRNG_CANNOT_READ_DEVICE_DESCRIPTOR);
			return r;
		}
		uint16_t idVendorCur = desc.idVendor;
		uint16_t idProductCur = desc.idProduct;
#if defined(_WIN32)
		if (idVendorCur == SWRNG_VENDOR_ID && idProductCur == SWRNG_PRODUCT_ID) {
#else
		if (idVendorCur == SWRNG_VENDOR_ID && (idProductCur == SWRNG_PRODUCT_ID || idProductCur == SWRNG_PRODUCT_ID2)) {
#endif
			swrng_updateDevInfoList(devInfoList, &curFoundDevNum);
		}
	}
	libusb_free_device_list(devs, 1);
	closeUSBLib(ctxt);
	return 0;
}

static void swrng_updateDevInfoList(DeviceInfoList* devInfoList, int *curFoundDevNum) {
	devInfoList->devInfoList[*curFoundDevNum].devNum = *curFoundDevNum;
	SwrngContext localCtxt;
	swrngInitializeContext(&localCtxt);
	if (swrngOpen(&localCtxt, *curFoundDevNum) == 0) {
		swrngGetModel(&localCtxt, &devInfoList->devInfoList[*curFoundDevNum].dm);
		swrngGetVersion(&localCtxt, &devInfoList->devInfoList[*curFoundDevNum].dv);
		swrngGetSerialNumber(&localCtxt,
			&devInfoList->devInfoList[*curFoundDevNum].sn);
	}
	swrngClose(&localCtxt);
	devInfoList->numDevs++;
	(*curFoundDevNum)++;
}

/**
 * Retrieve SwiftRNG device serial number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param DeviceSerialNumber* serialNumber - pointer to a structure for holding SwiftRNG device S/N
 * @return int - 0 when serial number retrieved successfully
 *
 */
int swrngGetSerialNumber(SwrngContext *ctxt, DeviceSerialNumber *serialNumber) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data(ctxt, (char*) "s", 1, serialNumber->value,
			sizeof(DeviceSerialNumber) - 1, SWRNG_USB_READ_TIMEOUT_SECS);
	// Replace status byte with \0 for ASCIIZ
	serialNumber->value[sizeof(DeviceSerialNumber) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage(ctxt, "Could not retrieve device serial number");
	}
	return resp;
}

/**
* A function to retrieve RAW random bytes from a noise source.
* No data alteration, verification or quality tests will be performed when calling this function.
* It will retrieve 16,000 raw bytes of the sampled random data from one of the noise sources.
* It can be used for inspecting the quality of the noise sources and data acquisition components.
*
* @param ctxt - pointer to SwrngContext structure
* @param NoiseSourceRawData *noiseSourceRawData - a pointer to a structure for holding 16,000 of raw data
* @param int noiseSourceNum - noise source number (0 - first noise source, 1 - second one)
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetRawDataBlock(SwrngContext *ctxt, NoiseSourceRawData *noiseSourceRawData, int noiseSourceNum) {

	char deviceCmd;

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	if (noiseSourceNum < 0 || noiseSourceNum > 1) {
		swrng_printErrorMessage(ctxt, "Noise source number must be 0 or 1");
		return -1;
	}

	if (noiseSourceNum == 0 ) {
		deviceCmd = '<';
	} else {
		deviceCmd = '>';
	}

	int resp = swrng_snd_rcv_usb_data(ctxt, &deviceCmd, 1, noiseSourceRawData->value,
			sizeof(NoiseSourceRawData) - 1, SWRNG_USB_READ_TIMEOUT_SECS);
	// Replace status byte with \0 for ASCIIZ
	noiseSourceRawData->value[sizeof(NoiseSourceRawData) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage(ctxt, "Could not retrieve RAW data from a noise source");
	}
	return resp;
}

/**
 * Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param DeviceVersion* version - pointer to a structure for holding SwiftRNG device version
 * @return int - 0 when version retrieved successfully
 *
 */
int swrngGetVersion(SwrngContext *ctxt, DeviceVersion *version) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data(ctxt, (char*) "v", 1, version->value,
			sizeof(DeviceVersion) - 1, SWRNG_USB_READ_TIMEOUT_SECS);
	// Replace status byte with \0 for ASCIIZ
	version->value[sizeof(DeviceVersion) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage(ctxt, "Could not retrieve device version");
	}
	return resp;
}

/**
* Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
* currently connected or when the device is already in use.
*
* @param ctxt - pointer to SwrngContext structure
* @param double* version - pointer to a double for holding SwiftRNG device version
* @return int - 0 when version retrieved successfully
*
*/
int swrngGetVersionNumber(SwrngContext *ctxt, double *version) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	*version = ctxt->deviceVersionDouble;
	return SWRNG_SUCCESS;
}

/**
* Run device diagnostics
*
* @param ctxt - pointer to SwrngContext structure
* @return int - 0 when diagnostics ran successfully, otherwise the error code
*
*/
int swrngRunDeviceDiagnostics(SwrngContext *ctxt) {
	char cmd;

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}


	if (ctxt->deviceVersionDouble < 2.0) {
		swrng_printErrorMessage(ctxt, SWRNG_DIAG_OP_NOT_SUPPORTED_MSG);
		return -1;
	}
	cmd = 'd';

	int resp = swrng_snd_rcv_usb_data(ctxt, &cmd, 1, &cmd, 0, SWRNG_USB_READ_TIMEOUT_SECS);
	if (resp != 0) {
		swrng_printErrorMessage(ctxt, "Device diagnostics failed");
	}
	return resp;
}

/**
* Set device power profile
*
* @param ctxt - pointer to SwrngContext structure
* @param ppNum - new power profile number (0 through 9)
* @return int - 0 when power profile was set successfully
*
*/
int swrngSetPowerProfile(SwrngContext *ctxt, int ppNum) {

	char ppn[1];

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	ppn[0] = '0' + ppNum;

	int resp = swrng_snd_rcv_usb_data(ctxt, ppn, 1, ppn, 0, SWRNG_USB_READ_TIMEOUT_SECS);
	if (resp != 0) {
		swrng_printErrorMessage(ctxt, "Could not set power profile");
	}
	return resp;
}


/**
* Enable post processing method.
*
* @param ctxt - pointer to SwrngContext structure
* @param postProcessingMethodId - 0 for SHA256 (default), 1 - xorshift64 (devices with versions 1.2 and up), 2 - for SHA512
*
* @return int - 0 when post processing successfully enabled, otherwise the error code
*
*/
int swrngEnablePostProcessing(SwrngContext *ctxt, int postProcessingMethodId) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	switch(postProcessingMethodId) {
	case 0:
		ctxt->postProcessingMethodId = SWRNG_SHA256_PP_METHOD;
		break;
	case 1:
		if (ctxt->deviceVersionDouble < 1.2) {
			swrng_printErrorMessage(ctxt, SWRNG_PP_OP_NOT_SUPPORTED_MSG);
			return -1;
		}
		ctxt->postProcessingMethodId = SWRNG_XORSHIFT64_PP_METHOD;
		break;
	case 2:
		ctxt->postProcessingMethodId = SWRNG_SHA512_PP_METHOD;
		break;
	default:
		swrng_printErrorMessage(ctxt, SWRNG_PP_METHOD_NOT_SUPPORTED_MSG);
		return -1;
	}
	ctxt->postProcessingEnabled = SWRNG_TRUE;
	return SWRNG_SUCCESS;
}

/**
* Enable statistical tests for raw random data stream.
*
* @param ctxt - pointer to SwrngContext structure
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*
*/
int swrngEnableStatisticalTests(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	ctxt->statisticalTestsEnabled = SWRNG_TRUE;
	return SWRNG_SUCCESS;
}

/**
* Disable post processing of raw random data (applies only to devices with versions 1.2 and up)
* Post processing is initially enabled for all devices.
*
* To disable post processing, call this function immediately after device is successfully open.
*
* @param ctxt - pointer to SwrngContext structure
* @return int - 0 when post processing was successfully disabled, otherwise the error code
*
*/
int swrngDisablePostProcessing(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	if (ctxt->deviceVersionDouble < 1.2) {
		swrng_printErrorMessage(ctxt, SWRNG_CANNOT_DISABLE_PP_MSG);
		return -1;
	}

	ctxt->postProcessingEnabled = SWRNG_FALSE;
	return SWRNG_SUCCESS;

}

/**
* Disable statistical tests for raw random data stream.
* Statistical tests are initially enabled for all devices.
*
* To disable statistical tests, call this function immediately after device is successfully open.
*
* @param ctxt - pointer to SwrngContext structure
* @return int - 0 when statistical tests successfully disabled, otherwise the error code
*
*/
int swrngDisableStatisticalTests(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	ctxt->statisticalTestsEnabled = SWRNG_FALSE;
	return SWRNG_SUCCESS;

}

/**
* Check to see if raw data post processing is enabled for device.
*
* @param ctxt - pointer to SwrngContext structure
* @param postProcessingStatus - pointer to store the post processing status (1 - enabled or 0 - otherwise)
* @return int - 0 when post processing flag successfully retrieved, otherwise the error code
*
*/
int swrngGetPostProcessingStatus(SwrngContext *ctxt, int *postProcessingStatus) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	*postProcessingStatus = ctxt->postProcessingEnabled;
	return SWRNG_SUCCESS;
}

/**
* Check to see if statistical tests are enabled on raw data stream for device.
*
* @param ctxt - pointer to SwrngContext structure
* @param statisticalTestsEnabledStatus - pointer to store statistical tests status (1 - enabled or 0 - otherwise)
* @return int - 0 when statistical tests flag successfully retrieved, otherwise the error code
*
*/
int swrngGetStatisticalTestsStatus(SwrngContext *ctxt, int *statisticalTestsEnabledStatus) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	*statisticalTestsEnabledStatus = ctxt->statisticalTestsEnabled;
	return SWRNG_SUCCESS;
}

/**
* Retrieve post processing method
*
* @param ctxt - pointer to SwrngContext structure
* @param postProcessingMethodId - pointer to store the post processing method id
* @return int - 0 when post processing method successfully retrieved, otherwise the error code
*
*/
int swrngGetPostProcessingMethod(SwrngContext *ctxt, int *postProcessingMethodId) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}
	*postProcessingMethodId = ctxt->postProcessingMethodId;
	return SWRNG_SUCCESS;
}

/**
* Retrieve device embedded correction method
*
* @param ctxt - pointer to SwrngContext structure
* @param deviceEmbeddedCorrectionMethodId - pointer to store the device built-in correction method id:
* 			0 - none, 1 - Linear correction (P. Lacharme)
* @return int - 0 embedded correction method successfully retrieved, otherwise the error code
*
*/
int swrngGetEmbeddedCorrectionMethod(SwrngContext *ctxt, int *deviceEmbeddedCorrectionMethodId) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}
	*deviceEmbeddedCorrectionMethodId = ctxt->deviceEmbeddedCorrectionMethodId;
	return SWRNG_SUCCESS;
}

/**
* A function for retrieving frequency tables of the random numbers generated from random sources.
* There is one frequency table per noise source. Each table consist of 256 integers (16 bit) that represent
* frequencies for the random numbers generated between 0 and 255. These tables are used for checking the quality of the
* noise sources and for detecting partial or total failures associated with those sources.
*
* SwiftRNG has two frequency tables. You will need to call this method twice - one call per frequency table.
*
* @param ctxt - pointer to SwrngContext structure
* @param FrequencyTables *frequencyTables - a pointer to the frequency tables structure
* @param int tableNumber - a frequency table number (0 or 1)
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetFrequencyTables(SwrngContext *ctxt, FrequencyTables *frequencyTables) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	if (ctxt->deviceVersionDouble < 1.2) {
		swrng_printErrorMessage(ctxt, SWRNG_CANNOT_GET_FREQ_TABLE_FOR_DEVICE_MSG);
		return -1;
	}

	if (frequencyTables == NULL) {
		swrng_printErrorMessage(ctxt, SWRNG_FREQ_TABLE_INVALID_MSG);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data(ctxt, (char*) "f", 1, (char *)frequencyTables,
			sizeof(FrequencyTables) - 2, SWRNG_USB_READ_TIMEOUT_SECS);
	if (resp != 0) {
		swrng_printErrorMessage(ctxt, "Could not retrieve device frequency tables");
	}
	return resp;
}

/**
 * Retrieve SwiftRNG device model number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param DeviceModel* model - pointer to a structure for holding SwiftRNG device model number
 * @return int 0 - when model retrieved successfully
 *
 */
int swrngGetModel(SwrngContext *ctxt, DeviceModel *model) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	if (ctxt->deviceOpen == SWRNG_FALSE) {
		swrng_printErrorMessage(ctxt, SWRNG_DEV_NOT_OPEN_MSG);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data(ctxt, (char*) "m", 1, model->value,
			sizeof(DeviceModel) - 1, SWRNG_USB_READ_TIMEOUT_SECS);
	// Replace status byte with \0 for ASCIIZ
	model->value[sizeof(DeviceModel) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage(ctxt, "Could not retrieve device model");
	}
	return resp;
}


/**
 * Initialize the SHA256 data
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void sha256_initialize(SwrngContext *ctxt) {
	// Initialize H0, H1, H2, H3, H4, H5, H6 and H7
	ctxt->sd.h0 = 0x6a09e667;
	ctxt->sd.h1 = 0xbb67ae85;
	ctxt->sd.h2 = 0x3c6ef372;
	ctxt->sd.h3 = 0xa54ff53a;
	ctxt->sd.h4 = 0x510e527f;
	ctxt->sd.h5 = 0x9b05688c;
	ctxt->sd.h6 = 0x1f83d9ab;
	ctxt->sd.h7 = 0x5be0cd19;
}

/**
 * Initialize the SHA512 data
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void sha512_initialize(SwrngContext *ctxt) {
	int i;
	// Initialize H0, H1, H2, H3, H4, H5, H6 and H7
	ctxt->sd5.h0 = 0x6a09e667f3bcc908;
	ctxt->sd5.h1 = 0xbb67ae8584caa73b;
	ctxt->sd5.h2 = 0x3c6ef372fe94f82b;
	ctxt->sd5.h3 = 0xa54ff53a5f1d36f1;
	ctxt->sd5.h4 = 0x510e527fade682d1;
	ctxt->sd5.h5 = 0x9b05688c2b3e6c1f;
	ctxt->sd5.h6 = 0x1f83d9abfb41bd6b;
	ctxt->sd5.h7 = 0x5be0cd19137e2179;

	for (i = 0; i < 15; i++) {
		ctxt->sd5.w[i] = 0;
	}
}

/**
 * Stamp a new serial number for the input data block into the last word
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param void* inputBlock pointer to the input hashing block
 *
 */
static void sha256_stampSerialNumber(SwrngContext *ctxt, void *inputBlock) {
	uint32_t *inw = (uint32_t*) inputBlock;
	inw[SWRNG_MIN_INPUT_NUM_WORDS] = ctxt->sd.blockSerialNumber++;
}

/**
 * Initialize the serial number for hashing
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param uint32_t initValue - a startup random number for generating serial number for hashing
 *
 */
static void sha256_initializeSerialNumber(SwrngContext *ctxt, uint32_t initValue) {
	ctxt->sd.blockSerialNumber = initValue;
}


/**
 * Generate SHA512 value.
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param uint64_t* src - pointer to an array of 64 bit words used as hash input
 * @param uint64_t* dst - pointer to an array of 8 X 64 bit words used as hash output
 * @param int16_t len - number of 64 bit words available in array pointed by 'src'
 *
 * @return int 0 for successful operation, -1 for invalid parameters
 *
 */
static int sha512_generateHash(SwrngContext *ctxt, uint64_t *src, int16_t len, uint64_t *dst) {

	if (len <= 0 || len > 14) {
		return -1;
	}

	sha512_initialize(ctxt);

	int i = 0;
	for (; i < len; i++) {
		ctxt->sd5.w[i] = src[i];
	}
	ctxt->sd5.w[i] = 0x8000000000000000;
	ctxt->sd5.w[15] = len * 64;


	sha512_hashCurrentBlock(ctxt);

	// Save the results
	dst[0] = ctxt->sd5.h0;
	dst[1] = ctxt->sd5.h1;
	dst[2] = ctxt->sd5.h2;
	dst[3] = ctxt->sd5.h3;
	dst[4] = ctxt->sd5.h4;
	dst[5] = ctxt->sd5.h5;
	dst[6] = ctxt->sd5.h6;
	dst[7] = ctxt->sd5.h7;

	return 0;
}

/**
 * Generate SHA256 value.
 *
 * @param ctxt - pointer to SwrngContext structure
 * @param uint32_t* src - pointer to an array of 32 bit words used as hash input
 * @param uint32_t* dst - pointer to an array of 8 X 32 bit words used as hash output
 * @param int16_t len - number of 32 bit words available in array pointed by 'src'
 *
 * @return int 0 for successful operation, -1 for invalid parameters
 *
 */
static int sha256_generateHash(SwrngContext *ctxt, uint32_t *src, int16_t len, uint32_t *dst) {

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

	sha256_initialize(ctxt);

	initialMessageSize = len * 8 * 4;
	numCompleteDataBlocks = len / maxDataBlockSizeWords;
	reminder = len % maxDataBlockSizeWords;

	// Process complete blocks
	for (blockNum = 0; blockNum < numCompleteDataBlocks; blockNum++) {
		srcOffset = blockNum * maxDataBlockSizeWords;
		for (ui8 = 0; ui8 < maxDataBlockSizeWords; ui8++) {
			ctxt->sd.w[ui8] = src[ui8 + srcOffset];
		}
		// Hash the current block
		sha256_hashCurrentBlock(ctxt);
	}

	srcOffset = numCompleteDataBlocks * maxDataBlockSizeWords;
	needAdditionalBlock = 1;
	needToAddOneMarker = 1;
	if (reminder > 0) {
		// Process the last data block if any
		ui8 = 0;
		for (; ui8 < reminder; ui8++) {
			ctxt->sd.w[ui8] = src[ui8 + srcOffset];
		}
		// Append '1' to the message
		ctxt->sd.w[ui8++] = 0x80000000;
		needToAddOneMarker = 0;
		if (ui8 < maxDataBlockSizeWords - 1) {
			for (; ui8 < maxDataBlockSizeWords - 2; ui8++) {
				// Fill with zeros
				ctxt->sd.w[ui8] = 0x0;
			}
			// add the message size to the current block
			ctxt->sd.w[ui8++] = 0x0;
			ctxt->sd.w[ui8] = initialMessageSize;
			sha256_hashCurrentBlock(ctxt);
			needAdditionalBlock = 0;
		} else {
			// Fill the rest with '0'
			// Will need to create another block
			ctxt->sd.w[ui8] = 0x0;
			sha256_hashCurrentBlock(ctxt);
		}
	}

	if (needAdditionalBlock) {
		ui8 = 0;
		if (needToAddOneMarker) {
			ctxt->sd.w[ui8++] = 0x80000000;
		}
		for (; ui8 < maxDataBlockSizeWords - 2; ui8++) {
			ctxt->sd.w[ui8] = 0x0;
		}
		ctxt->sd.w[ui8++] = 0x0;
		ctxt->sd.w[ui8] = initialMessageSize;
		sha256_hashCurrentBlock(ctxt);
	}

	// Save the results
	dst[0] = ctxt->sd.h0;
	dst[1] = ctxt->sd.h1;
	dst[2] = ctxt->sd.h2;
	dst[3] = ctxt->sd.h3;
	dst[4] = ctxt->sd.h4;
	dst[5] = ctxt->sd.h5;
	dst[6] = ctxt->sd.h6;
	dst[7] = ctxt->sd.h7;

	return 0;
}

/**
 * Hash current block
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void sha512_hashCurrentBlock(SwrngContext *ctxt) {

	uint8_t t;

	// Process elements 16...79
	for (t = 16; t <= 79; t++) {
		ctxt->sd5.w[t] = sha512_sigma1(&ctxt->sd5.w[t-2]) + ctxt->sd5.w[t-7] + sha512_sigma0(&ctxt->sd5.w[t-15]) + ctxt->sd5.w[t-16];
	}

	// Initialize variables
	ctxt->sd5.a = ctxt->sd5.h0;
	ctxt->sd5.b = ctxt->sd5.h1;
	ctxt->sd5.c = ctxt->sd5.h2;
	ctxt->sd5.d = ctxt->sd5.h3;
	ctxt->sd5.e = ctxt->sd5.h4;
	ctxt->sd5.f = ctxt->sd5.h5;
	ctxt->sd5.g = ctxt->sd5.h6;
	ctxt->sd5.h = ctxt->sd5.h7;

	// Process elements 0...79
	for (t = 0; t <= 79; t++) {
		ctxt->sd5.tmp1 = ctxt->sd5.h + sha512_sum1(&ctxt->sd5.e) + sha512_ch(&ctxt->sd5.e, &ctxt->sd5.f, &ctxt->sd5.g) + sha512_k[t] + ctxt->sd5.w[t];
		ctxt->sd5.tmp2 = sha512_sum0(&ctxt->sd5.a) + sha512_maj(&ctxt->sd5.a, &ctxt->sd5.b, &ctxt->sd5.c);
		ctxt->sd5.h = ctxt->sd5.g;
		ctxt->sd5.g = ctxt->sd5.f;
		ctxt->sd5.f = ctxt->sd5.e;
		ctxt->sd5.e = ctxt->sd5.d + ctxt->sd5.tmp1;
		ctxt->sd5.d = ctxt->sd5.c;
		ctxt->sd5.c = ctxt->sd5.b;
		ctxt->sd5.b = ctxt->sd5.a;
		ctxt->sd5.a = ctxt->sd5.tmp1 + ctxt->sd5.tmp2;
	}

	// Calculate the final hash for the block
	ctxt->sd5.h0 += ctxt->sd5.a;
	ctxt->sd5.h1 += ctxt->sd5.b;
	ctxt->sd5.h2 += ctxt->sd5.c;
	ctxt->sd5.h3 += ctxt->sd5.d;
	ctxt->sd5.h4 += ctxt->sd5.e;
	ctxt->sd5.h5 += ctxt->sd5.f;
	ctxt->sd5.h6 += ctxt->sd5.g;
	ctxt->sd5.h7 += ctxt->sd5.h;
}

/**
 * Hash current block
 * @param ctxt - pointer to SwrngContext structure
 *
 */
static void sha256_hashCurrentBlock(SwrngContext *ctxt) {
	uint8_t t;

	// Process elements 16...63
	for (t = 16; t <= 63; t++) {
		ctxt->sd.w[t] = sha256_sigma1(&ctxt->sd.w[t - 2]) + ctxt->sd.w[t - 7] + sha256_sigma0(
				&ctxt->sd.w[t - 15]) + ctxt->sd.w[t - 16];
	}

	// Initialize variables
	ctxt->sd.a = ctxt->sd.h0;
	ctxt->sd.b = ctxt->sd.h1;
	ctxt->sd.c = ctxt->sd.h2;
	ctxt->sd.d = ctxt->sd.h3;
	ctxt->sd.e = ctxt->sd.h4;
	ctxt->sd.f = ctxt->sd.h5;
	ctxt->sd.g = ctxt->sd.h6;
	ctxt->sd.h = ctxt->sd.h7;

	// Process elements 0...63
	for (t = 0; t <= 63; t++) {
		ctxt->sd.tmp1 = ctxt->sd.h + sha256_sum1(&ctxt->sd.e) + sha256_ch(&ctxt->sd.e, &ctxt->sd.f, &ctxt->sd.g)
				+ sha256_k[t] + ctxt->sd.w[t];
		ctxt->sd.tmp2 = sha256_sum0(&ctxt->sd.a) + sha256_maj(&ctxt->sd.a, &ctxt->sd.b, &ctxt->sd.c);
		ctxt->sd.h = ctxt->sd.g;
		ctxt->sd.g = ctxt->sd.f;
		ctxt->sd.f = ctxt->sd.e;
		ctxt->sd.e = ctxt->sd.d + ctxt->sd.tmp1;
		ctxt->sd.d = ctxt->sd.c;
		ctxt->sd.c = ctxt->sd.b;
		ctxt->sd.b = ctxt->sd.a;
		ctxt->sd.a = ctxt->sd.tmp1 + ctxt->sd.tmp2;
	}

	// Calculate the final hash for the block
	ctxt->sd.h0 += ctxt->sd.a;
	ctxt->sd.h1 += ctxt->sd.b;
	ctxt->sd.h2 += ctxt->sd.c;
	ctxt->sd.h3 += ctxt->sd.d;
	ctxt->sd.h4 += ctxt->sd.e;
	ctxt->sd.h5 += ctxt->sd.f;
	ctxt->sd.h6 += ctxt->sd.g;
	ctxt->sd.h7 += ctxt->sd.h;
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
	return ROTR32(2, *x) ^ ROTR32(13, *x) ^ ROTR32(22, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.5)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t Sum1 value
 *
 */
static uint32_t sha256_sum1(uint32_t *x) {
	return ROTR32(6, *x) ^ ROTR32(11, *x) ^ ROTR32(25, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.6)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t sigma0 value
 *
 */
static uint32_t sha256_sigma0(uint32_t *x) {
	return ROTR32(7, *x) ^ ROTR32(18, *x) ^ ((*x) >> 3);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.7)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t sigma1 value
 *
 */
static uint32_t sha256_sigma1(uint32_t *x) {
	return ROTR32(17, *x) ^ ROTR32(19, *x) ^ ((*x) >> 10);
}


/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.8)
 *
 * @param uint64_t* x pointer to variable x
 * @param uint64_t* y pointer to variable y
 * @param uint64_t* z pointer to variable z
 * $return uint64_t ch value
 *
 */
static uint64_t sha512_ch(uint64_t *x, uint64_t *y, uint64_t *z) {
	return  ((*x) & (*y)) ^ (~(*x) & (*z));
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.9)
 *
 * @param uint64_t* x pointer to variable x
 * @param uint64_t* y pointer to variable y
 * @param uint64_t* z pointer to variable z
 * $return uint64_t maj value
 *
 */
static uint64_t sha512_maj(uint64_t *x, uint64_t *y, uint64_t *z) {
	return ((*x) & (*y)) ^ ((*x) & (*z)) ^ ((*y) & (*z));
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.10)
 *
 * @param uint64_t* x pointer to variable x
 * $return uint64_t sum0 value
 *
 */
static uint64_t sha512_sum0(uint64_t *x) {
	return ROTR64(28, *x) ^ ROTR64(34, *x) ^ ROTR64(39, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.11)
 *
 * @param uint64_t* x pointer to variable x
 * $return uint64_t sum1 value
 *
 */
static uint64_t sha512_sum1(uint64_t *x) {
	return ROTR64(14, *x) ^ ROTR64(18, *x) ^ ROTR64(41, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.12)
 *
 * @param uint64_t* x pointer to variable x
 * $return uint64_t sigma0 value
 *
 */
static uint64_t sha512_sigma0(uint64_t *x) {
	return ROTR64(1, *x) ^ ROTR64(8, *x) ^ ((*x) >> 7);
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.13)
 *
 * @param uint64_t* x pointer to variable x
 * $return uint64_t sigma1 value
 *
 */
static uint64_t sha512_sigma1(uint64_t *x) {
	return ROTR64(19, *x) ^ ROTR64(61, *x) ^ ((*x) >> 6);
}


/*
 * A function for running the self test for the SHA256 post processing method
 *
 * @param ctxt - pointer to SwrngContext structure
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int sha256_selfTest(SwrngContext *ctxt) {
	uint32_t results[8];
	int retVal;
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	retVal = sha256_generateHash(ctxt, (uint32_t*) sha256_testSeq1, (uint16_t) 11,
			(uint32_t*) results);
	if (retVal == 0) {
		// Compare the expected with actual results
		retVal = memcmp(results, sha256_exptHashSeq1, 8);
	}
	return retVal;
}

/*
 * A function for running the self test for the SHA512 post processing method
 *
 * @param ctxt - pointer to SwrngContext structure
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int sha512_selfTest(SwrngContext *ctxt) {
	uint64_t results[8];
	int retVal;
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	retVal = sha512_generateHash(ctxt,
			(uint64_t *)"8765432187654321876543218765432187654321876543218765432187654321",
			(uint16_t) 8, (uint64_t*) results);
	if (retVal == 0) {
		// Compare the expected with actual results
		retVal = memcmp(results, sha512_exptHashSeq1, 8);
	}
	return retVal;
}

/**
 * Generate and retrieve device statistics
 * @param ctxt - pointer to SwrngContext structure
 * @return a pointer to DeviceStatistics structure or NULL if the call failed
 */
DeviceStatistics* swrngGenerateDeviceStatistics(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return NULL;
	}

	time(&ctxt->ds.endTime); // Stop performance timer
	// Calculate performance
	ctxt->ds.totalTime = ctxt->ds.endTime - ctxt->ds.beginTime;
	if (ctxt->ds.totalTime == 0) {
		ctxt->ds.totalTime = 1;
	}
	ctxt->ds.downloadSpeedKBsec = (int) (ctxt->ds.numGenBytes / (int64_t) 1024
			/ (int64_t)ctxt->ds.totalTime);
	return &ctxt->ds;
}

/**
 * Reset statistics for the SWRNG device
 *
 * @param ctxt - pointer to SwrngContext structure
 */
void swrngResetStatistics(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return;
	}
	time(&ctxt->ds.beginTime); // Start performance timer
	ctxt->ds.downloadSpeedKBsec = 0;
	ctxt->ds.numGenBytes = 0;
	ctxt->ds.totalRetries = 0;
	ctxt->ds.endTime = 0;
	ctxt->ds.totalTime = 0;
}

/**
* Retrieve the last error message.
* The caller should make a copy of the error message returned immediately after calling this function.
* @param ctxt - pointer to SwrngContext structure
* @return - pointer to the error message
*/
const char* swrngGetLastErrorMessage(SwrngContext *ctxt) {

	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return ctxtNotInitializedErrMsg;
	}

	if (ctxt->lastError == (char*)NULL) {
		strcpy(ctxt->lastError, "");
	}
	return ctxt->lastError;
}

/**
* Call this function to enable printing error messages to the error stream
* @param ctxt - pointer to SwrngContext structure
*/
void swrngEnablePrintingErrorMessages(SwrngContext *ctxt) {
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return;
	}
	ctxt->enPrintErrorMessages = SWRNG_TRUE;
}

/**
 * Print and/or save error message
 * @param ctxt - pointer to SwrngContext structure
 * @param errMsg - pointer to error message
 */
static void swrng_printErrorMessage(SwrngContext *ctxt, const char* errMsg) {
	if (ctxt->enPrintErrorMessages) {
		fprintf(stderr, "%s", errMsg);
		fprintf(stderr, "\n");
	}
	if (strlen(errMsg) >= sizeof(ctxt->lastError)) {
		strcpy(ctxt->lastError, "Error message too long");
	} else {
		strcpy(ctxt->lastError, errMsg);
	}
}

/**
* Initialize SwrngContext context. This function must be called first!
* @param ctxt - pointer to SwrngContext structure
* @return 0 - if context initialized successfully
*/
int swrngInitializeContext(SwrngContext *ctxt) {
	if (ctxt != NULL) {
		memset(ctxt, 0, sizeof(SwrngContext));
		ctxt->numFailuresThreshold = 4;
		ctxt->maxAptFailuresPerBlock = 0;
		ctxt->maxRctFailuresPerBlock = 0;
		ctxt->enPrintErrorMessages = SWRNG_FALSE;
		ctxt->startSignature = SWRNG_START_CONTEXT_SIGNATURE;
		ctxt->endSignature = SWRNG_END_CONTEXT_SIGNATURE;
		ctxt->curTrngOutIdx = SWRNG_TRND_OUT_BUFFSIZE;
		strcpy(ctxt->lastError, "");
		ctxt->postProcessingEnabled = SWRNG_TRUE;
		ctxt->statisticalTestsEnabled = SWRNG_TRUE;
		ctxt->postProcessingMethodId = SWRNG_SHA256_PP_METHOD;
		ctxt->deviceEmbeddedCorrectionMethodId = SWRNG_EMB_CORR_METHOD_NONE;
		return SWRNG_SUCCESS;
	}
	return -1;
}

/**
* Check to see if the context has been initialized
* 
* @param ctxt - pointer to SwrngContext structure
* @return SWRNG_TRUE - context initialized
*/
static swrngBool isContextInitialized(SwrngContext *ctxt) {
	swrngBool retVal = SWRNG_FALSE;
	if (ctxt != NULL) {
		if (ctxt->startSignature == SWRNG_START_CONTEXT_SIGNATURE
			&& ctxt->endSignature == SWRNG_END_CONTEXT_SIGNATURE) {
			retVal = SWRNG_TRUE;
		}
	}
	return retVal;
}

/**
* Apply Xorshift64 (Marsaglia's PPRNG method) to the raw word
* @param rawWord - word to post process
*/
static uint64_t xorshift64_postProcessWord(uint64_t rawWord) {
	uint64_t trueWord = rawWord;

	trueWord ^= trueWord >> 12;
	trueWord ^= trueWord << 25;
	trueWord ^= trueWord >> 27;
	return trueWord * UINT64_C(2685821657736338717);
}

/**
*
* @param buffer - pointer to input data buffer
* @param numElements - number of elements in the input buffer
*/
static void xorshift64_postProcessWords(uint64_t *buffer, int numElements) {
	int i;
	for(i = 0; i < numElements; i++) {
		buffer[i] = xorshift64_postProcessWord(buffer[i]);
	}
}

/**
* @param buffer - pointer to input data buffer
* @param numElements - number of elements in the input buffer
*/
static void xorshift64_postProcess(uint8_t *buffer, int numElements) {
	xorshift64_postProcessWords((uint64_t *)buffer, numElements / 8);
}

/*
 * A function for running the self test for the xorshift64 post processing method
 *
 * @param ctxt - pointer to SwrngContext structure
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int xorshift64_selfTest(SwrngContext *ctxt) {
	uint64_t rawWord = 0x1212121212121212;
	uint64_t testWord = 0x2322d6d77d8b7b55;
	if (isContextInitialized(ctxt) == SWRNG_FALSE) {
		return -1;
	}

	xorshift64_postProcess((uint8_t*)&rawWord, 8);

	if (rawWord == testWord) {
		return SWRNG_SUCCESS;
	} else {
		return -1;
	}
}



