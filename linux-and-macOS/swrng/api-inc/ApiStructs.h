/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.

 This file defines data structures used in SwiftRNG API implementation.

 */

/**
 *    @file ApiStructs.h
 *    @date 9/17/2023
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief Data structures used in the API implementation.
 */

#ifndef SWIFTRNG_API_STRUCTURES_H_
#define SWIFTRNG_API_STRUCTURES_H_

#include <stdint.h>
#include <time.h>

#define SWRNG_SUCCESS (0)

typedef struct {

	/* Total number of random bytes generated */
	int64_t numGenBytes;

	/* Total number of download re-transmissions */
	int64_t totalRetries;

	/* Used for measuring performance */
	time_t beginTime;
	time_t endTime;
	time_t totalTime;

	/* Measured download speed in KB/sec */
	int downloadSpeedKBsec;
} DeviceStatistics;

typedef struct {
	/* Device serial number (ASCIIZ) */
	char value[16];
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
	/* Device version (ASCIIZ) */
	char value[5];
} DeviceVersion;

typedef struct {
	/* Device model (ASCIIZ) */
	char value[9];
} DeviceModel;

typedef struct {

	/* Logical SwiftRNG device number */
	int devNum;

	/* SwiftRNG device model */
	DeviceModel dm;

	/* SwiftRNG device version */
	DeviceVersion dv;

	/* SwiftRNG device serial number */
	DeviceSerialNumber sn;
} DeviceInfo;

typedef struct {
	/* Array of DeviceInfo */
	DeviceInfo devInfoList[127];

	/* Actual number of elements in devInfoList array */
	int numDevs;
} DeviceInfoList;


#endif /* SWIFTRNG_API_STRUCTURES_H_ */
