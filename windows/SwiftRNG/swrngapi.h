/*
 * swrngapi.h
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
#ifndef SWRNGAPI_H_
#define SWRNGAPI_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
	#include "libusb.h"
#else
	#include <libusb-1.0/libusb.h>
#endif

#define swrngBool int
#define SWRNG_TRUE (1)
#define SWRNG_FALSE (0)
#define SWRNG_SUCCESS (0)
#define BUFF_FILE_SIZE_BYTES (10000 * 10)


/**
 * Global structures
 */

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

/**
 * Global function declaration section
 */

#ifdef __cplusplus

extern "C" int swrngOpen(int deviceNum); // Open SwiftRNG USB specific device (0 - first SwiftRNG device)
extern "C" int swrngClose(); // Close SwiftRNG USB device if open
extern "C" int swrngGetEntropy(unsigned char *buffer, long length);
extern "C" int swrngGetDeviceList(DeviceInfoList *devInfoList); // Retrieve a list of SwiftRNG devices connected to the host
extern "C" int swrngGetModel(DeviceModel *model); // Retrieve device model
extern "C" int swrngGetVersion(DeviceVersion *version); // Retrieve device version
extern "C" int swrngGetSerialNumber(DeviceSerialNumber *serialNumber); // Retrieve unique serial number of the SWRNG device
extern "C" void swrngResetStatistics();
extern "C" int swrngSetPowerProfile(int ppNum);
extern "C" DeviceStatistics* swrngGenerateDeviceStatistics();
extern "C" const char* swrngGetLastErrorMessage();
extern "C" void swrngEnablePrintingErrorMessages();

#else

extern int swrngOpen(int deviceNum); // Open SwiftRNG USB specific device (0 - first SwiftRNG device)
extern int swrngClose(); // Close SwiftRNG USB device if open
extern int swrngGetEntropy(unsigned char *buffer, long length);
extern int swrngGetDeviceList(DeviceInfoList *devInfoList); // Retrieve a list of SwiftRNG devices connected to the host
extern int swrngGetModel(DeviceModel *model); // Retrieve device model
extern int swrngGetVersion(DeviceVersion *version); // Retrieve device version
extern int swrngGetSerialNumber(DeviceSerialNumber *serialNumber); // Retrieve unique serial number of the SWRNG device
extern void swrngResetStatistics();
extern int swrngSetPowerProfile(int ppNum);
extern DeviceStatistics* swrngGenerateDeviceStatistics();
extern const char* swrngGetLastErrorMessage();
extern void swrngEnablePrintingErrorMessages();

#endif

#endif /* SWRNGAPI_H_ */ 
