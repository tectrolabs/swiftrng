/*
 * swrngapi.h
 * ver. 4.0
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
#ifndef SWRNGAPI_H_
#define SWRNGAPI_H_

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined _WIN32
	#include "libusb.h"
	#include "USBComPort.h"
#else
	#include <libusb-1.0/libusb.h>
	#include "USBSerialDevice.h"
#endif

#define swrngBool int
#define SWRNG_TRUE (1)
#define SWRNG_FALSE (0)
#define SWRNG_SHA256_PP_METHOD (0)
#define SWRNG_EMB_CORR_METHOD_NONE (0)
#define SWRNG_EMB_CORR_METHOD_LINEAR (1)
#define SWRNG_XORSHIFT64_PP_METHOD (1)
#define SWRNG_SHA512_PP_METHOD (2)
#define SWRNG_SUCCESS (0)
#define SWRNG_BUFF_FILE_SIZE_BYTES (10000 * 10)
#define SWRNG_WORD_SIZE_BYTES (4)
#define SWRNG_NUM_CHUNKS (500)
#define SWRNG_MIN_INPUT_NUM_WORDS (8)
#define SWRNG_OUT_NUM_WORDS (8)
#define SWRNG_TRND_OUT_BUFFSIZE (SWRNG_NUM_CHUNKS * SWRNG_OUT_NUM_WORDS * SWRNG_WORD_SIZE_BYTES)
#define SWRNG_RND_IN_BUFFSIZE (SWRNG_NUM_CHUNKS * SWRNG_MIN_INPUT_NUM_WORDS * SWRNG_WORD_SIZE_BYTES)
#define SWRNG_MAX_CDC_COM_PORTS (40)

 //
 // Structure definitions
 //

typedef struct {
	int64_t numGenBytes; // Total number of random bytes generated
	int64_t totalRetries; // Total number of download re-transmissions
	time_t beginTime, endTime, totalTime; // Used for measuring performance
	int downloadSpeedKBsec; // Measured download speed in KB/sec
} DeviceStatistics;

typedef struct {
	char value[16]; // Device serial number (ASCIIZ)
} DeviceSerialNumber;

typedef struct {
	uint16_t freqTable1[256];
	uint16_t freqTable2[256];
	uint16_t none;
} FrequencyTables;

typedef struct {
	char value[16001];
} NoiseSourceRawData;

typedef struct {
	char value[5]; // Device version (ASCIIZ)
} DeviceVersion;

typedef struct {
	char value[9]; // Device model (ASCIIZ)
} DeviceModel;

typedef struct {
	int devNum; // Logical SwiftRNG device number
	DeviceModel dm; // SwiftRNG device model
	DeviceVersion dv; // SwiftRNG device version
	DeviceSerialNumber sn; // SwiftRNG device serial number
} DeviceInfo;

typedef struct {
	DeviceInfo devInfoList[127]; // Array of DeviceInfo
	int numDevs; // Actual number of elements in devInfoList array
} DeviceInfoList;


//
// Context structure for maintaining the state
//
typedef struct {
	int startSignature;
	struct sha256_data {
		uint32_t a, b, c, d, e, f, g, h;
		uint32_t h0, h1, h2, h3, h4, h5, h6, h7;
		uint32_t tmp1, tmp2;
		uint32_t w[64];
		uint32_t blockSerialNumber;
	} sd;

	struct sha512_data {
		uint64_t a,b,c,d,e,f,g,h;
		uint64_t h0,h1,h2,h3,h4,h5,h6,h7;
		uint64_t tmp1, tmp2;
		uint64_t w[80];
	} sd5;

	// Repetition Count Test data
	struct rct_data {
		uint32_t maxRepetitions;
		uint32_t curRepetitions;
		uint8_t lastSample;
		uint8_t statusByte;
		uint8_t signature;
		swrngBool isInitialized;
		uint16_t failureCount;
	} rct;

	// Adaptive Proportion Test data
	struct apt_data {
		uint16_t windowSize;
		uint16_t cutoffValue;
		uint16_t curRepetitions;
		uint16_t curSamples;
		uint8_t statusByte;
		uint8_t signature;
		swrngBool isInitialized;
		uint8_t firstSample;
		uint16_t cycleFailures;
	} apt;

	// Other variables
	libusb_device **devs;
	libusb_device *dev;
	libusb_device_handle *devh;
	swrngBool libusbInitialized; // True if libusb has been initialized
	libusb_context *luctx;
	swrngBool deviceOpen; // True if device successfully open
	int curTrngOutIdx; // Current index for the buffTRndOut buffer
	unsigned char bulk_in_buffer[SWRNG_RND_IN_BUFFSIZE + 1];
	unsigned char bulk_out_buffer[16];
	char buffRndIn[SWRNG_RND_IN_BUFFSIZE + 1]; // Random input buffer
	char buffTRndOut[SWRNG_TRND_OUT_BUFFSIZE]; // Random output buffer
	uint32_t srcToHash32[SWRNG_MIN_INPUT_NUM_WORDS + 1]; // The source of one block of data to hash
	uint64_t srcToHash64[SWRNG_MIN_INPUT_NUM_WORDS]; // The source of one block of data to hash
	uint8_t numFailuresThreshold;// How many statistical test failures allowed per block (16000 random bytes)
	uint16_t maxRctFailuresPerBlock; // Max number of repetition count test failures encountered per data block
	uint16_t maxAptFailuresPerBlock; // Max number of adaptive proportion test failures encountered per data block
	DeviceStatistics ds;
	char lastError[512];
	swrngBool enPrintErrorMessages;
	int endSignature;
	DeviceVersion curDeviceVersion;
	double deviceVersionDouble;
	swrngBool postProcessingEnabled;
	swrngBool statisticalTestsEnabled;
	int postProcessingMethodId;	// 0 - SHA256 method, 1 - xorshift64 post processing method, 2 - SHA512 method
	int deviceEmbeddedCorrectionMethodId;	// 0 - none, 1 - Linear correction (P. Lacharme)
#ifdef _WIN32
	USBComPort *usbComPort;
#else
	USBSerialDevice *usbSerialDevice;
#endif

} SwrngContext;

/**
 * Global function declaration section
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
* Initialize SwrngContext context. This function must be called first!
* @param ctxt - pointer to SwrngContext structure
* @return 0 - if context initialized successfully
*/
int swrngInitializeContext(SwrngContext *ctxt);

/**
* Open SwiftRNG USB specific device.
*
* @param ctxt - pointer to SwrngContext structure
* @param int devNum - device number (0 - for the first device or only one device)
* @return int 0 - when open successfully or error code
*/
int swrngOpen(SwrngContext *ctxt, int devNum);

/**
* Close device if open
*
* @param ctxt - pointer to SwrngContext structure
* @return int - 0 when processed successfully
*/
int swrngClose(SwrngContext *ctxt);

/**
* Check if device is open
*
* @param ctxt - pointer to SwrngContext structure
* @return int - 0 when device is open
*/
int swrngIsOpen(SwrngContext *ctxt);

/**
* A function to retrieve up to 100000 random bytes
*
* @param ctxt - pointer to SwrngContext structure
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive, max value is 100000
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetEntropy(SwrngContext *ctxt, unsigned char *buffer, long length);

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
int swrngGetEntropyEx(SwrngContext *ctxt, unsigned char *buffer, long length);

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
int swrngGetRawDataBlock(SwrngContext *ctxt, NoiseSourceRawData *noiseSourceRawData, int noiseSourceNum);

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
int swrngGetFrequencyTables(SwrngContext *ctxt, FrequencyTables *frequencyTables);

/**
* Retrieve a complete list of SwiftRNG devices currently plugged into USB ports
*
* @param ctxt - pointer to SwrngContext structure
* @param DeviceInfoList* devInfoList - pointer to the structure for holding SwiftRNG
* @return int - value 0 when retrieved successfully, otherwise the error code
*
*/
int swrngGetDeviceList(SwrngContext *ctxt, DeviceInfoList *devInfoList);

/**
* Retrieve SwiftRNG device model number. This call will fail when there is no SwiftRNG device
* currently connected or when the device is already in use.
*
* @param ctxt - pointer to SwrngContext structure
* @param DeviceModel* model - pointer to a structure for holding SwiftRNG device model number
* @return int 0 - when model retrieved successfully, otherwise the error code
*
*/
int swrngGetModel(SwrngContext *ctxt, DeviceModel *model);

/**
* Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
* currently connected or when the device is already in use.
*
* @param ctxt - pointer to SwrngContext structure
* @param DeviceVersion* version - pointer to a structure for holding SwiftRNG device version
* @return int - 0 when version retrieved successfully, otherwise the error code
*
*/
int swrngGetVersion(SwrngContext *ctxt, DeviceVersion *version);

/**
* Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
* currently connected or when the device is already in use.
*
* @param ctxt - pointer to SwrngContext structure
* @param double* version - pointer to a double for holding SwiftRNG device version
* @return int - 0 when version retrieved successfully, otherwise the error code
*
*/
int swrngGetVersionNumber(SwrngContext *ctxt, double *version);

/**
* Retrieve SwiftRNG device serial number. This call will fail when there is no SwiftRNG device
* currently connected or when the device is already in use.
*
* @param ctxt - pointer to SwrngContext structure
* @param DeviceSerialNumber* serialNumber - pointer to a structure for holding SwiftRNG device S/N
* @return int - 0 when serial number retrieved successfully, otherwise the error code
*
*/
int swrngGetSerialNumber(SwrngContext *ctxt, DeviceSerialNumber *serialNumber);

/**
* Reset statistics for the SWRNG device
*
* @param ctxt - pointer to SwrngContext structure
*/
void swrngResetStatistics(SwrngContext *ctxt);

/**
* Set device power profile
*
* @param ctxt - pointer to SwrngContext structure
* @param ppNum - new power profile number (0 through 9)
* @return int - 0 when power profile was set successfully, otherwise the error code
*
*/
int swrngSetPowerProfile(SwrngContext *ctxt, int ppNum);

/**
* Run device diagnostics
*
* @param ctxt - pointer to SwrngContext structure
* @return int - 0 when diagnostics ran successfully, otherwise the error code
*
*/
int swrngRunDeviceDiagnostics(SwrngContext *ctxt);

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
int swrngDisablePostProcessing(SwrngContext *ctxt);

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
int swrngDisableStatisticalTests(SwrngContext *ctxt);

/**
* Check to see if raw data post processing is enabled for device.
*
* @param ctxt - pointer to SwrngContext structure
* @param postProcessingStatus - pointer to store the post processing status (1 - enabled or 0 - otherwise)
* @return int - 0 when post processing flag successfully retrieved, otherwise the error code
*
*/
int swrngGetPostProcessingStatus(SwrngContext *ctxt, int *postProcessingStatus);

/**
* Check to see if statistical tests are enabled on raw data stream for device.
*
* @param ctxt - pointer to SwrngContext structure
* @param statisticalTestsEnabledStatus - pointer to store statistical tests status (1 - enabled or 0 - otherwise)
* @return int - 0 when statistical tests flag successfully retrieved, otherwise the error code
*
*/
int swrngGetStatisticalTestsStatus(SwrngContext *ctxt, int *statisticalTestsEnabledStatus);

/**
* Retrieve post processing method
*
* @param ctxt - pointer to SwrngContext structure
* @param postProcessingMethodId - pointer to store the post processing method id
* @return int - 0 when post processing method successfully retrieved, otherwise the error code
*
*/
int swrngGetPostProcessingMethod(SwrngContext *ctxt, int *postProcessingMethodId);

/**
* Retrieve device embedded correction method
*
* @param ctxt - pointer to SwrngContext structure
* @param deviceEmbeddedCorrectionMethodId - pointer to store the device built-in correction method id:
* 			0 - none, 1 - Linear correction (P. Lacharme)
* @return int - 0 embedded correction method successfully retrieved, otherwise the error code
*
*/
int swrngGetEmbeddedCorrectionMethod(SwrngContext *ctxt, int *deviceEmbeddedCorrectionMethodId);


/**
* Enable post processing method.
*
* @param ctxt - pointer to SwrngContext structure
* @param postProcessingMethodId - 0 for SHA256 (default), 1 - xorshift64 (devices with versions 1.2 and up), 2 - for SHA512
*
* @return int - 0 when post processing successfully enabled, otherwise the error code
*
*/
int swrngEnablePostProcessing(SwrngContext *ctxt, int postProcessingMethodId);

/**
* Enable statistical tests for raw random data stream.
*
* @param ctxt - pointer to SwrngContext structure
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*
*/
int swrngEnableStatisticalTests(SwrngContext *ctxt);

/**
* Generate and retrieve device statistics
* @param ctxt - pointer to SwrngContext structure
* @return a pointer to DeviceStatistics structure or NULL if the call failed
*/
DeviceStatistics* swrngGenerateDeviceStatistics(SwrngContext *ctxt);

/**
* Retrieve the last error message.
* The caller should make a copy of the error message returned immediately after calling this function.
* @param ctxt - pointer to SwrngContext structure
* @return - pointer to the error message
*/
const char* swrngGetLastErrorMessage(SwrngContext *ctxt);

/**
* Call this function to enable printing error messages to the error stream
* @param ctxt - pointer to SwrngContext structure
*/
void swrngEnablePrintingErrorMessages(SwrngContext *ctxt);

#ifdef __cplusplus
}
#endif

#endif /* SWRNGAPI_H_ */ 
