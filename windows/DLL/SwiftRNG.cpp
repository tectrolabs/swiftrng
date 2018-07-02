/*
 * SwiftRNG.cpp
 * ver. 1.0
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

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include "SwiftRNG.h"
#include "swrngapi.h"

/**
 * Open SwiftRNG USB specific device.
 *
 * @param int deviceNum - device number (0 - for the first device)
 * @return int 0 - when open successfully or error code
 */
int swftOpen(int deviceNum) {
	return swrngOpen(deviceNum);
}

/**
 * Close device if open
 *
 * @return int - 0 when processed successfully
 */
int swftClose() {
	return swrngClose();
}

/**
 * A function to retrieve random bytes
 * @param insigned char *buffer - a pointer to the data receive buffer
 * @param long length - how many bytes expected to receive
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int swftGetEntropy(unsigned char *buffer, long length) {
	return swrngGetEntropy(buffer, length);
}

/**
 * A function to retrieve a random byte
 * @return random byte value between 0 and 255 (a value of 256 or greater will indicate an error) 
 *
 */
int swftGetEntropyByte() {
	unsigned char val[2];
	int status = swrngGetEntropy(val, 1);
	if (status == 0) {
		return val[0];
	} else {
		return 256;
	}
}


/**
 * A function to set power profile
 * @param int ppNum - power profile number (between 0 and 9)
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int swftSetPowerProfile(int ppNum) {
	return swrngSetPowerProfile(ppNum);
}

/**
 * Retrieve the last error messages
 * @return - pointer to the error message
 */
const char* swftGetLastErrorMessage() {
	return swrngGetLastErrorMessage();
}

/**
 * Retrieve SwiftRNG device model number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param char model[9] - pointer to a 9 bytes char array for holding SwiftRNG device model number
 * @return int 0 - when model retrieved successfully
 *
 */
int swftGetModel(char model[9]) {
	int ret = 0;
	DeviceModel deviceModel;
	ret = swrngGetModel(&deviceModel);
	if (ret == 0) {
		strcpy(model, deviceModel.value);
	}
	return ret;
}

/**
 * Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param char version[5] - pointer to a 5 bytes array for holding SwiftRNG device version
 * @return int - 0 when version retrieved successfully
 *
 */
int swftGetVersion(char version[5]) {
	int ret = 0;
	DeviceVersion deviceVersion;
	ret = swrngGetVersion(&deviceVersion);
	if (ret == 0) {
		strcpy(version, deviceVersion.value);
	}
	return ret;
}

/**
 * Retrieve SwiftRNG device serial number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param char serialNumber[16] - pointer to a 16 bytes array for holding SwiftRNG device S/N
 * @return int - 0 when serial number retrieved successfully
 *
 */
int swftGetSerialNumber(char serialNumber[16]) {
	int ret = 0;
	DeviceSerialNumber deviceSerNum;
	ret = swrngGetSerialNumber(&deviceSerNum);
	if (ret == 0) {
		strcpy(serialNumber, deviceSerNum.value);
	}
	return ret;
}