/*
 * swrng-cl-api.h
 * Ver. 1.10
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2018 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for interacting with a cluster of SwiftRNG devices for the purpose of
 downloading true random bytes.

 This program requires the libusb-1.0 library when communicating with a cluster of SwiftRNG devices.
 Please read the provided documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with TectroLabs devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef SWRNG_CL_API_H_
#define SWRNG_CL_API_H_

#include "swrngapi.h"
#if defined(_WIN32)
 #include <windows.h> 
 #include <stdio.h>
 #include <conio.h>
 #include <sys/types.h>
 #include <stdint.h>
 #include <stdlib.h>
 #include <sys/types.h>
 #include <stdint.h>
 #include <time.h>
 #include <errno.h>
 #include <process.h>
#else
 #include <pthread.h>
 #include <sched.h>
 #include <unistd.h>
#endif


#define SWRNG_MAX_ERR_MSG_SIZE_BYTES (256)
#define SWRNG_THREAD_EVENT_ERROR (10)


//
// Context thread structure
//
typedef struct {
	SwrngContext ctxt;

	// Thread variables
#if defined(_WIN32)
	HANDLE dwnl_thread;
	HANDLE dwnl_thread_event;
#else
	pthread_t dwnl_thread;
	pthread_mutex_t dwnl_mutex;
	pthread_cond_t dwnl_synch;
#endif
	int devNum; // Logical SwiftRNG device number
	volatile int destroyDwnlThreadReq;
	volatile int dwnlRequestActive;
	volatile int dwnl_status;
	unsigned char *trngOutBuffer;
} SwrngThreadContext;

//
// Clustered context thread structure
//
typedef struct {
	int startSignature;

	swrngBool enPrintErrorMessages;
	char lastError[SWRNG_MAX_ERR_MSG_SIZE_BYTES];	// Last cluster recorded error message
	char lastTmpError[SWRNG_MAX_ERR_MSG_SIZE_BYTES];	// Last cluster temporary storage for recorded error message
	SwrngThreadContext *tctxts;	// An array of SwrngThreadContext entries
	swrngBool clusterOpen; 	// True if cluster successfully open
	int clusterSize;		// Preferred cluster size - number of devices in a cluster
	int actClusterSize;		// Actual cluster size - actual number of devices in a cluster
	unsigned char *trngOutBuffer;	// Final TRNG output buffer for cluster
	int curTrngOutIdx; // Current index for the trngOutBuffer buffer
	int ppNum;			// Power profile number of the cluster
	swrngBool ppnChanged;
	long numClusterFailoverEvents;	// How many times cluster recovered from errors
	long numClusterResizeAttempts;	// How many times cluster attempted to resize
	time_t clusterStartedSecs;	// The time when cluster open successfully
	swrngBool postProcessingEnabled; // A flag indicating if post processing is enabled for all cluster devices
	int postProcessingMethodId;	// -1 - not in use, 0 - default SHA256 method, 1 - xorshift64 post processing method


	int endSignature;
} SwrngCLContext;

/**
 * Global function declaration section
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
* Initialize SwrngCLContext context. This function must be called first when cluster is used!
* @param ctxt - pointer to SwrngCLContext structure
* @return 0 - if context initialized successfully
*/
int swrngInitializeCLContext(SwrngCLContext *ctxt);

/**
* Open SwiftRNG USB cluster.
*
* @param ctxt - pointer to SwrngCLContext structure
* @param clusterSize - preferred cluster size
* @return int 0 - when open successfully or error code
*/
int swrngCLOpen(SwrngCLContext *ctxt, int clusterSize);

/**
* Check if cluster is open
* @param ctxt - pointer to SwrngCLContext structure
* @return int - 0 when cluster is open
*/
int swrngIsCLOpen(SwrngCLContext *ctxt);

/**
 * Close the cluster if open
 *
 * @param ctxt - pointer to SwrngCLContext structure
 * @return int - 0 when processed successfully
 */
int swrngCLClose(SwrngCLContext *ctxt);

/**
* Retrieve the last error message.
* The caller should make a copy of the error message returned immediately after calling this function.
* @param ctxt - pointer to SwrngCLContext structure
* @return - pointer to the error message
*/
const char* swrngGetCLLastErrorMessage(SwrngCLContext *ctxt);

/**
* Retrieve number of devices currently in use by the cluster
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of devices used by the cluster
*/
int swrngGetCLSize(SwrngCLContext *ctxt);

/**
* Retrieve number of cluster fail-over events. That number will be incremented each
* time the cluster is trying to recover as a result of device errors.
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of cluster fail-over events
*/
long swrngGetCLFailoverEventCount(SwrngCLContext *ctxt);

/**
* Retrieve number of cluster resize attempts. That number will be incremented each
* time the cluster is trying to resize to reach the preferred size.
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of cluster resize attempts
*/
long swrngGetCLResizeAttemptCount(SwrngCLContext *ctxt);

/**
* A function to retrieve random bytes from a cluster of SwiftRNG devices
*
* @param ctxt - pointer to SwrngCLContext structure
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetCLEntropy(SwrngCLContext *ctxt, unsigned char *buffer, long length);

/**
* Set power profile for each device in the cluster
*
* @param ctxt - pointer to SwrngCLContext structure
* @param ppNum - new power profile number (0 through 9)
* @return int - 0 when power profile was set successfully
*
*/
int swrngSetCLPowerProfile(SwrngCLContext *ctxt, int ppNum);

/**
* Call this function to enable printing error messages to the error stream
* @param ctxt - pointer to SwrngCLContext structure
*/
void swrngEnableCLPrintingErrorMessages(SwrngCLContext *ctxt);

/**
* Disable post processing of raw random data for each device in a cluster.
* For devices with versions 1.2 and up the post processing can be disabled
* after device is open.
*
* To disable post processing, call this function immediately after cluster is successfully open.
*
* @param ctxt - pointer to SwrngCLContext structure
* @return int - 0 when post processing was successfully disabled, otherwise the error code
*
*/
int swrngDisableCLPostProcessing(SwrngCLContext *ctxt);

/**
* Enable post processing method.
*
* @param ctxt - pointer to SwrngContext structure
* @param postProcessingMethodId - 0 for SHA256 (default), 1 - xorshift64 (devices with versions 1.2 and up), 2 - for SHA512
*
* @return int - 0 when post processing successfully enabled, otherwise the error code
*
*/
int swrngEnableCLPostProcessing(SwrngCLContext *ctxt, int postProcessingMethodId);


#ifdef __cplusplus
}
#endif


#endif /* SWRNG_CL_API_H_ */
