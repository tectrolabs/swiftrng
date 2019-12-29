/*
 * SwiftRNG.h
 * ver. 1.5
 *
 */

 /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, http://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for interacting with the hardware random data generator device SwiftRNG for the purpose of
 downloading true random bytes.

 This program requires the libusb-1.0 library when communicating with the SwiftRNG device. Please read the provided
 documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with the SwiftRNG device.

 This DLL was designed to be shared between applications concurrently. To get a secured and dedicated
 connection to SwiftRNG device utilize SwiftRNG API library directly.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/



#ifndef SWIFTRNG_H_
#define SWIFTRNG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* A process-safe and thread-safe function to retrieve random bytes from the SwiftRNG device
* @param unsigned char *buffer - a pointer to the data receive buffer (must hold length + 1 bytes)
* @param long length - how many bytes expected to receive (must not be greater than 10,000,000)
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftGetEntropySynchronized(unsigned char *buffer, long length);

/**
*
* A process-safe and thread-safe function to retrieve a random byte from the SwiftRNG device.
* @return random byte value between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
__declspec(dllexport) int swftGetEntropyByteSynchronized();

/**
* A process-safe and thread-safe function for setting device power profile
* @param int ppNum - power profile number (between 0 and 9)
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftSetPowerProfileSynchronized(int ppNum);

/**
* A process-safe and thread-safe function to retrieve SwiftRNG device model number.
* @param char model[9] - pointer to a 9 bytes char array for holding SwiftRNG device model number
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftGetModelSynchronized(char model[9]);

/**
* A process-safe and thread-safe function to retrieve SwiftRNG device version.
* @param char version[5] - pointer to a 5 bytes array for holding SwiftRNG device version
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftGetVersionSynchronized(char version[5]);

/**
* A process-safe and thread-safe function to retrieve SwiftRNG device serial number.
* @param char serialNumber[16] - pointer to a 16 bytes array for holding SwiftRNG device S/N
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftGetSerialNumberSynchronized(char serialNumber[16]);

/**
* A process-safe and thread-safe function to retrieve 16,000 of RAW random bytes from a noise source of the SwiftRNG device.
* No data alteration, verification or quality tests will be performed when calling this function.
* It will retrieve 16,000 raw bytes of the sampled random data from one of the noise sources.
* It can be used for inspecting the quality of the noise sources and data acquisition components.
*
* @param unsigned char rawBytes[16000] - pointer to a 16,000 bytes array for holding RAW random bytes
* @param int noiseSourceNum - noise source number (0 - first noise source, 1 - second one)
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swrngGetRawDataBlockSynchronized(unsigned char rawBytes[16000], int noiseSourceNum);

/**
* A process-safe and thread-safe function for enabling post processing of raw random data regardless of the device version.
* For devices with versions 1.2 and up the post processing can be disabled after device is open.
*
* To enable post processing, call this function any time
*
* @return int - 0 when post processing was successfully enabled, otherwise the error code
*
*/
__declspec(dllexport) int swrngEnableDataPostProcessing();

/**
* Check to see if raw data post processing is enabled for device.
*
* @return int - 1 when post processing is enabled, 0 if disabled, negative number if error
*/
__declspec(dllexport) int swrngGetDataPostProcessingStatus();

/**
* A process-safe and thread-safe function for disabling post processing of raw random data. 
* It takes effect only for devices with versions 1.2 and up.
*
* To disable post processing, call this function any time
*
* @return int - 0 when post processing was successfully disabled, otherwise the error code
*
*/
__declspec(dllexport) int swrngDisableDataPostProcessing();

/**
* A process-safe and thread-safe function for disabling statistical tests for raw random data.
*
* @return int - 0 when statistical tests were successfully disabled, otherwise the error code
*
*/
__declspec(dllexport) int swrngDisableDataStatisticalTests();

/**
* A process-safe and thread-safe function for enabling statistical tests for raw random data.
*
* @return int - 0 when statistical tests were successfully enabled, otherwise the error code
*
*/
__declspec(dllexport) int swrngEnableDataStatisticalTests();

/**
* Check to see if statistical tests are enabled for device.
*
* @return int - 1 when statistical tests are enabled, 0 if disabled, negative number if error
*/
__declspec(dllexport) int swrngGetDataStatisticalTestsStatus();

/**
*
* A process-safe and thread-safe function to retrieve a random byte from wiftRNG entropy server.
* There should be an entropy server running to successfully call the function.
* @return random byte value between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
__declspec(dllexport) int swftGetByteFromEntropyServerSynchronized();

/**
* A process-safe and thread-safe function to retrieve random bytes from SwiftRNG entropy server.
* There should be an entropy server running to successfully call the function.
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive (must not be greater than 100,000)
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftGetEntropyFromEntropyServer(unsigned char *buffer, long length);

/**
* A process-safe and thread-safe function to set the pipe endpoint to the SwiftRNG entropy server.
* When used, it should be called immediately after loading the DLL in order to override the default endpoint
* and before any other call to the DLL is made.
* @param char *pipeEndpoint - a pointer to an ASCIIZ pipe endpoint
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftSetEntropyServerPipeEndpointSynchronized(char *pipeEndpoint);

#ifdef __cplusplus
}
#endif

#endif /* SWIFTRNG_H_ */ 