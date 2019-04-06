#include "stdafx.h"

/*
 * swrng-cl-api.c
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
 Please read the SwiftRNG documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with TectroLabs devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "swrng-cl-api.h"

//
// Error messages
//

static const char clusterAlreadyOpenErrMsg[] = "Cluster already open";
static const char clusterNotOpenErrMsg[] = "Cluster not open";
static const char clusterSizeInvalidErrMsg[] = "Cluster size must be between 1 and 10";
static const char clusterNotAvailableErrMsg[] =  "Failed to form a cluster, check for available SwiftRNG devices";
static const char lowMemoryErrMsg[] =  "Cannot allocate a memory block to continue";
static const char leakMemoryErrMsg[] = "Memory block already allocated";
static const char ctxtNotInitializedErrMsg[] = "SwrngCLContext not initialized";
static const char needMoreCPUsErrMsg[] = "Need more CPUs available to continue";
static const char threadCreationErrMsg[] = "Thread creation error";

static const long outputBuffSizeBytes = 100000L;

// Seconds to wait before starting the fail-over event after device errors are detected
static const int clusterFailoverWaitSecs = 6;

// Seconds to wait before trying to re-cluster to get the preferred size
static const int clusterResizeWaitSecs = 60 * 60;

// A cluster size beyond 10 has not been tested yet
static const int maxClusterSize = 10;

// Context markers
static const int clContextStartSignature = 12321;
static const int clContextEndSignature = 57321;

//
// Declarations for local functions
//

static void wait_seconds(int seconds);
static swrngBool isContextCLInitialized(SwrngCLContext *ctxt);
static swrngBool isItTimeToResizeCluster(SwrngCLContext *ctxt);
static int initializeCLContext(SwrngCLContext *ctxt);
static void printCLErrorMessage(SwrngCLContext *ctxt, const char* errMsg);
static void freeAllocatedMemory(SwrngCLContext *ctxt);
static int allocateMemory(SwrngCLContext *ctxt);
static int getEntropyBytes(SwrngCLContext *ctxt);
static int setCLPowerProfile(SwrngCLContext *ctxt, int ppNum);

static void cleanup_download_thread(void *param);
#ifndef _WIN32
static void *download_thread(void *th_params);
#else
unsigned int __stdcall download_thread(void *th_params);
#endif
static void wait_complete_download_req(SwrngThreadContext *ctxt);
static void wait_all_complete_download_reqs(SwrngCLContext *ctxt);
static int initializeCLThreads(SwrngCLContext *ctxt);
static void unInitializeCLThreads(SwrngCLContext *ctxt);
static void trigger_download_reqs(SwrngCLContext *ctxt);
static int getClusterDownloadStatus(SwrngCLContext *ctxt);
static int disableCLPostProcessing(SwrngCLContext *ctxt);
static int enableCLPostProcessing(SwrngCLContext *ctxt, int postProcessingMethodId);


/**
* Retrieve the last error message.
* The caller should make a copy of the error message returned immediately after calling this function.
* @param ctxt - pointer to SwrngCLContext structure
* @return - pointer to the error message
*/
const char* swrngGetCLLastErrorMessage(SwrngCLContext *ctxt) {

	if (isContextCLInitialized(ctxt) == SWRNG_FALSE) {
		return ctxtNotInitializedErrMsg;
	}

	if (ctxt->lastError == (char*)NULL) {
		strcpy(ctxt->lastError, "");
	}
	return ctxt->lastError;
}



/**
* Trigger a new download requests
*
* @param ctxt - pointer to SwrngCLContext structure
*/
static void trigger_download_reqs(SwrngCLContext *ctxt) {
	int i;
	for (i = 0; i < ctxt->actClusterSize; i++) {
		ctxt->tctxts[i].dwnlRequestActive = SWRNG_TRUE;
#ifndef _WIN32
		pthread_cond_signal(&ctxt->tctxts[i].dwnl_synch);
#endif
	}
}

/**
* A function to retrieve the cluster download status
*
* @param ctxt - pointer to SwrngCLContext structure
* @return 0 - successful operation, otherwise the error code
*
*/
static int getClusterDownloadStatus(SwrngCLContext *ctxt) {
	int retval = SWRNG_SUCCESS;
	int i;
	for (i = 0; i < ctxt->actClusterSize; i++) {
		if (ctxt->tctxts[i].dwnl_status != SWRNG_SUCCESS) {
			retval = ctxt->tctxts[i].dwnl_status;
			strcpy(ctxt->lastError, ctxt->tctxts[i].ctxt.lastError);
		}
	}
	return retval;
}

/**
* A function to retrieve random bytes from a cluster
*
* @param ctxt - pointer to SwrngCLContext structure
* @return 0 - successful operation, otherwise the error code
*
*/
static int getEntropyBytes(SwrngCLContext *ctxt) {
	int retval = SWRNG_SUCCESS;
	trigger_download_reqs(ctxt);
	wait_all_complete_download_reqs(ctxt);
	retval = getClusterDownloadStatus(ctxt);
	if ( retval == SWRNG_SUCCESS) {
		ctxt->curTrngOutIdx = 0;
	}
	return retval;
}

/**
* A function to retrieve random bytes from a cluster of SwiftRNG devices
*
* @param ctxt - pointer to SwrngCLContext structure
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetCLEntropy(SwrngCLContext *ctxt, unsigned char *buffer, long length) {
	int retval = SWRNG_SUCCESS;
	int status;
	long act;
	long total;

	if (swrngIsCLOpen(ctxt) != SWRNG_TRUE) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -ENODATA;
	}

	total = 0;
	do {
		if (ctxt->curTrngOutIdx >= (ctxt->actClusterSize * outputBuffSizeBytes)) {

			if (isItTimeToResizeCluster(ctxt)) {
				ctxt->numClusterResizeAttempts++;
				swrngCLClose(ctxt);
				wait_seconds(clusterFailoverWaitSecs);
				status = swrngCLOpen(ctxt, ctxt->clusterSize);
				if (status != SWRNG_SUCCESS) {
					return status;
				}
			}
			retval = getEntropyBytes(ctxt);
			if (retval != SWRNG_SUCCESS) {
				strcpy(ctxt->lastTmpError, ctxt->lastError);
				status = retval;
				// Got error, restart the cluster
				ctxt->numClusterFailoverEvents++;
				swrngCLClose(ctxt);
				wait_seconds(clusterFailoverWaitSecs);
				retval = swrngCLOpen(ctxt, ctxt->clusterSize);
				if (retval != SWRNG_SUCCESS) {
					strcpy(ctxt->lastError, ctxt->lastTmpError);
					return status;
				}
				retval = getEntropyBytes(ctxt);
			}
		}
		if (retval == SWRNG_SUCCESS) {
			act = (ctxt->actClusterSize * outputBuffSizeBytes) - ctxt->curTrngOutIdx;
			if (act > (length - total)) {
				act = (length - total);
			}
			memcpy(buffer + total, ctxt->trngOutBuffer + ctxt->curTrngOutIdx, act);
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

	return retval;
}

/**
* Initialize SwrngCLContext context. This function must be called first when cluster is used!
* @param ctxt - pointer to SwrngCLContext structure
* @return 0 - if context initialized successfully
*/
int swrngInitializeCLContext(SwrngCLContext *ctxt) {
	return initializeCLContext(ctxt);
}

/**
* Open SwiftRNG USB cluster.
*
* @param ctxt - pointer to SwrngCLContext structure
* @param clusterSize - preferred cluster size
* @return int 0 - when open successfully or error code
*/
int swrngCLOpen(SwrngCLContext *ctxt, int clusterSize) {
	SwrngContext ctxtSearch;
	DeviceInfoList devInfoList;
	DeviceVersion version;

	if (isContextCLInitialized(ctxt) == SWRNG_FALSE) {
		initializeCLContext(ctxt);
	} else {
		if (swrngIsCLOpen(ctxt) == SWRNG_TRUE) {
			printCLErrorMessage(ctxt, clusterAlreadyOpenErrMsg);
			return -1;
		}
		if (clusterSize <= 0 || clusterSize > maxClusterSize) {
			printCLErrorMessage(ctxt, clusterSizeInvalidErrMsg);
			return -1;
		}
	}

	ctxt->clusterSize = clusterSize;
#ifndef _WIN32
	int numCores = (int)sysconf(_SC_NPROCESSORS_ONLN);
	if (numCores <= clusterSize / 2) {
		printCLErrorMessage(ctxt, needMoreCPUsErrMsg);
		return -1;
	}
#endif
	// Look for compatible devices for the cluster
	swrngInitializeContext(&ctxtSearch);
	int retVal = swrngGetDeviceList(&ctxtSearch, &devInfoList);
	if (retVal != SWRNG_SUCCESS) {
		swrngClose(&ctxtSearch);
		return retVal;
	}

	if (devInfoList.numDevs == 0) {
		printCLErrorMessage(ctxt, clusterNotAvailableErrMsg);
		swrngClose(&ctxtSearch);
		return -1;
	}

	if (ctxt->tctxts != NULL) {
		printCLErrorMessage(ctxt, leakMemoryErrMsg);
		swrngClose(&ctxtSearch);
		return -1;
	}
	ctxt->tctxts = (SwrngThreadContext *)calloc(ctxt->clusterSize, sizeof(SwrngThreadContext));
	if (ctxt->tctxts == NULL) {
		printCLErrorMessage(ctxt, lowMemoryErrMsg);
		swrngClose(&ctxtSearch);
		return -1;
	}

	int i;
	ctxt->actClusterSize = 0;
	for (i = 0; i < devInfoList.numDevs && ctxt->actClusterSize < ctxt->clusterSize; i++) {
		retVal = swrngOpen(&ctxt->tctxts[i].ctxt, devInfoList.devInfoList[i].devNum);
		if (retVal != SWRNG_SUCCESS) {
			swrngClose(&ctxt->tctxts[i].ctxt);
			continue;
		}
		retVal = swrngGetVersion(&ctxt->tctxts[i].ctxt, &version);
		if (retVal != SWRNG_SUCCESS) {
			swrngClose(&ctxt->tctxts[i].ctxt);
			continue;
		}
		if (i > ctxt->actClusterSize) {
			ctxt->tctxts[ctxt->actClusterSize].ctxt = ctxt->tctxts[i].ctxt;
		}
		ctxt->actClusterSize++;
	}

	if (ctxt->actClusterSize == 0) {
		printCLErrorMessage(ctxt, clusterNotAvailableErrMsg);
		swrngClose(&ctxtSearch);
		freeAllocatedMemory(ctxt);
		return -1;
	}

	int status = allocateMemory(ctxt);
	if ( status != SWRNG_SUCCESS) {
		swrngClose(&ctxtSearch);
		freeAllocatedMemory(ctxt);
		return status;
	}

	swrngClose(&ctxtSearch);

	ctxt->curTrngOutIdx = ctxt->actClusterSize * outputBuffSizeBytes;
	status = initializeCLThreads(ctxt);
	if ( status != SWRNG_SUCCESS) {
		freeAllocatedMemory(ctxt);
		return status;
	}

	if (ctxt->ppnChanged == SWRNG_TRUE) {
		setCLPowerProfile(ctxt, ctxt->ppNum); 	// Ignore the error
	}

	if (ctxt->postProcessingEnabled == SWRNG_FALSE) {
		disableCLPostProcessing(ctxt);	// Ignore the error
	} else {
		if (ctxt->postProcessingMethodId != -1) {
			enableCLPostProcessing(ctxt, ctxt->postProcessingMethodId); // Ignore the error
		}
	}

	ctxt->clusterStartedSecs = time(NULL);
	ctxt->clusterOpen = SWRNG_TRUE;

	return SWRNG_SUCCESS;
}

/**
 * Initialize cluster threads
 *
 * @param ctxt - pointer to SwrngCLContext structure
 * @return int - 0 when processed successfully
 */
static int initializeCLThreads(SwrngCLContext *ctxt) {
	int status = SWRNG_SUCCESS;

	int i;
	for (i = 0; i < ctxt->actClusterSize; i++) {
		ctxt->tctxts[i].destroyDwnlThreadReq = SWRNG_FALSE;
		ctxt->tctxts[i].dwnlRequestActive = SWRNG_FALSE;
		ctxt->tctxts[i].dwnl_status = SWRNG_SUCCESS;
		ctxt->tctxts[i].trngOutBuffer = ctxt->trngOutBuffer + (i * outputBuffSizeBytes);
#ifndef _WIN32
		pthread_mutex_init(&ctxt->tctxts[i].dwnl_mutex, NULL);
		pthread_cond_init(&ctxt->tctxts[i].dwnl_synch, NULL);
		int rc = pthread_create(&ctxt->tctxts[i].dwnl_thread, NULL, download_thread, (void*)&ctxt->tctxts[i]);
		if (rc != 0) {
			void *th_retval;
			pthread_mutex_destroy(&ctxt->tctxts[i].dwnl_mutex);
			pthread_cond_destroy(&ctxt->tctxts[i].dwnl_synch);
			printCLErrorMessage(ctxt, threadCreationErrMsg);
			int j;
			for (j = 0; j < i; j++) {
				ctxt->tctxts[i].destroyDwnlThreadReq = SWRNG_TRUE;
				pthread_join(ctxt->tctxts[j].dwnl_thread, &th_retval);
			}
			return -1;
		}
#else
		ctxt->tctxts[i].dwnl_thread = (HANDLE)_beginthreadex(0, 0, &download_thread, (void*)&ctxt->tctxts[i], 0, 0);
		if (ctxt->tctxts[i].dwnl_thread == NULL) {
			printCLErrorMessage(ctxt, threadCreationErrMsg);
			int j;
			for (j = 0; j < i; j++) {
				ctxt->tctxts[i].destroyDwnlThreadReq = SWRNG_TRUE;
				WaitForSingleObject(ctxt->tctxts[i].dwnl_thread, INFINITE);
				CloseHandle(ctxt->tctxts[i].dwnl_thread);
			}
			return -1;
		}
#endif
	}

	return status;
}

/**
 * Shutdown and clean up all threads
 *
 * @param ctxt - pointer to SwrngCLContext structure
 */
static void unInitializeCLThreads(SwrngCLContext *ctxt) {
#ifndef _WIN32
	void *th_retval;
#endif
	int i;

	for (i = 0; i < ctxt->actClusterSize; i++) {
		wait_complete_download_req(&ctxt->tctxts[i]);
		ctxt->tctxts[i].destroyDwnlThreadReq = SWRNG_TRUE;
#ifndef _WIN32
		pthread_cond_signal(&ctxt->tctxts[i].dwnl_synch);
		pthread_join(ctxt->tctxts[i].dwnl_thread, &th_retval);
		pthread_mutex_destroy(&ctxt->tctxts[i].dwnl_mutex);
		pthread_cond_destroy(&ctxt->tctxts[i].dwnl_synch);
#else
		WaitForSingleObject(ctxt->tctxts[i].dwnl_thread, INFINITE);
		CloseHandle(ctxt->tctxts[i].dwnl_thread);
#endif
		ctxt->tctxts[i].dwnlRequestActive = SWRNG_FALSE;
	}
}

/**
 * Close the cluster if open
 *
 * @param ctxt - pointer to SwrngCLContext structure
 * @return int - 0 when processed successfully
 */
int swrngCLClose(SwrngCLContext *ctxt) {

	int retVal = SWRNG_SUCCESS;

	if (swrngIsCLOpen(ctxt) == SWRNG_FALSE) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	unInitializeCLThreads(ctxt);

	int i;
	for (i = 0; i < ctxt->actClusterSize; i++) {
		// Ignore the previous error
		retVal = swrngClose(&ctxt->tctxts[i].ctxt);
	}

	freeAllocatedMemory(ctxt);
	strcpy(ctxt->lastError, "");

	ctxt->clusterOpen = SWRNG_FALSE;
	return retVal;
}

/**
* Retrieve number of devices currently in used by the cluster
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of current devices used by the cluster
*/
int swrngGetCLSize(SwrngCLContext *ctxt) {

	if (swrngIsCLOpen(ctxt) != SWRNG_TRUE) {
		return 0;
	}
	return ctxt->actClusterSize;
}

/**
* Retrieve number of cluster fail-over events. That number will be incremented each
* time the cluster is trying to recover as a result of device errors.
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of cluster fail-over events
*/
long swrngGetCLFailoverEventCount(SwrngCLContext *ctxt) {
	if (swrngIsCLOpen(ctxt) != SWRNG_TRUE) {
		return 0;
	}
	return ctxt->numClusterFailoverEvents;
}

/**
* Retrieve number of cluster resize attempts. That number will be incremented each
* time the cluster is trying to resize to reach the preferred size.
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of cluster resize attempts
*/
long swrngGetCLResizeAttemptCount(SwrngCLContext *ctxt) {
	if (swrngIsCLOpen(ctxt) != SWRNG_TRUE) {
		return 0;
	}
	return ctxt->numClusterResizeAttempts;
}



/**
* Allocated memory resources
*
* @param ctxt - pointer to SwrngCLContext structure
* @return int - 0 when processed successfully
*/
static int allocateMemory(SwrngCLContext *ctxt) {
	int status = SWRNG_SUCCESS;

	if (ctxt->trngOutBuffer != NULL) {
		status = -1;
	}

	if (status != SWRNG_SUCCESS) {
		printCLErrorMessage(ctxt, leakMemoryErrMsg);
		return -1;
	}

	ctxt->trngOutBuffer = (unsigned char *)calloc(ctxt->actClusterSize, outputBuffSizeBytes);
	if (ctxt->trngOutBuffer == NULL) {
		status = -1;
	}

	if (status != SWRNG_SUCCESS) {
		printCLErrorMessage(ctxt, lowMemoryErrMsg);
		return -1;
	}

	return SWRNG_SUCCESS;
}

/**
* Free allocated memory resources
*
* @param ctxt - pointer to SwrngCLContext structure
*/
static void freeAllocatedMemory(SwrngCLContext *ctxt) {
	if (ctxt != NULL) {
		if (ctxt->trngOutBuffer != NULL) {
			free(ctxt->trngOutBuffer);
			ctxt->trngOutBuffer = NULL;
		}
		if (ctxt->tctxts != NULL) {
			free(ctxt->tctxts);
			ctxt->tctxts = NULL;
		}
	}
}

/**
* Check to see if the cluster context has been initialized
*
* @param ctxt - pointer to SwrngCLContext structure
* @return SWRNG_TRUE - context SI initialized
*/
static swrngBool isContextCLInitialized(SwrngCLContext *ctxt) {
	swrngBool retVal = SWRNG_FALSE;
	if (ctxt != NULL) {
		if (ctxt->startSignature == clContextStartSignature
			&& ctxt->endSignature == clContextEndSignature) {
			retVal = SWRNG_TRUE;
		}
	}
	return retVal;
}

/**
* Initialize SwrngCLContext context.
* @param ctxt - pointer to SwrngCLContext structure
* @return 0 - if context initialized successfully
*/
static int initializeCLContext(SwrngCLContext *ctxt) {
	if (ctxt != NULL) {
		memset(ctxt, 0, sizeof(SwrngCLContext));
		ctxt->startSignature = clContextStartSignature;
		ctxt->endSignature = clContextEndSignature;
		ctxt->postProcessingEnabled = SWRNG_TRUE;
		ctxt->postProcessingMethodId = -1;
		return SWRNG_SUCCESS;
	}
	return -1;
}

/**
* Check if cluster is open
*
* @return int - 0 when cluster is open
*/
int swrngIsCLOpen(SwrngCLContext *ctxt) {

	if (isContextCLInitialized(ctxt) == SWRNG_FALSE) {
		return SWRNG_FALSE;
	}
	return ctxt->clusterOpen;
}

/**
 * Print and/or save error message
 * @param ctxt - pointer to SwrngCLContext structure
 * @param errMsg - pointer to error message
 */
static void printCLErrorMessage(SwrngCLContext *ctxt, const char* errMsg) {
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
* Cleanup thread
* @param param - pointer to thread parameters
*/
static void cleanup_download_thread(void *param) {
	SwrngThreadContext *ctxt = (SwrngThreadContext *)param;
	if (ctxt != NULL) {
#ifndef _WIN32
		pthread_mutex_unlock(&ctxt->dwnl_mutex);
#endif
		ctxt->dwnlRequestActive = SWRNG_FALSE;
	}
}

/**
* Download thread
* @param th_params - pointer to thread parameters
*/

#ifndef _WIN32
static void *download_thread(void *th_params) {

	int rc;
	time_t start;
	struct timespec timeout;
	SwrngThreadContext *tctxt = (SwrngThreadContext *)th_params;

	pthread_cleanup_push(cleanup_download_thread, (void*)tctxt);

	rc = pthread_mutex_lock(&tctxt->dwnl_mutex);
	if (rc) {
		pthread_exit(NULL);
	}

	while(tctxt->destroyDwnlThreadReq == SWRNG_FALSE) {
		if (tctxt->dwnlRequestActive == SWRNG_TRUE) {
			tctxt->dwnl_status = swrngGetEntropy(&tctxt->ctxt, tctxt->trngOutBuffer, outputBuffSizeBytes);
			tctxt->dwnlRequestActive = SWRNG_FALSE;
		}
		usleep(100);
		start = time(NULL);
		timeout.tv_sec = start;
		timeout.tv_nsec = 1000;
		pthread_cond_timedwait(&tctxt->dwnl_synch, &tctxt->dwnl_mutex, &timeout);
	}

	pthread_exit(NULL);
	pthread_cleanup_pop(0);
}
#endif

#ifdef _WIN32
unsigned int __stdcall download_thread(void *th_params) {
	SwrngThreadContext *tctxt = (SwrngThreadContext *)th_params;

	while (tctxt->destroyDwnlThreadReq == SWRNG_FALSE) {
		if (tctxt->dwnlRequestActive == SWRNG_TRUE) {
			tctxt->dwnl_status = swrngGetEntropy(&tctxt->ctxt, tctxt->trngOutBuffer, outputBuffSizeBytes);
			tctxt->dwnlRequestActive = SWRNG_FALSE;
		}
		Sleep(0);
	}
	return 0;
}
#endif

/**
* Wait for download request to complete
*
* @param ctxt - pointer to SwrngThreadContext structure
* @return SWRNG_TRUE - context initialized
*/
static void wait_complete_download_req(SwrngThreadContext *ctxt) {
	while(ctxt->dwnlRequestActive == SWRNG_TRUE) {
#ifndef _WIN32
		sched_yield();
		usleep(50);
#else
		Sleep(0);
#endif
	}
}

/**
* Wait for all download requests to complete
*
* @param ctxt - pointer to SwrngCLContext structure
* @return SWRNG_TRUE - context initialized
*/
static void wait_all_complete_download_reqs(SwrngCLContext *ctxt) {
	int i;
	for (i = 0; i < ctxt->actClusterSize; i++) {
		wait_complete_download_req(&ctxt->tctxts[i]);
	}
}

/**
 * A function to wait for specific seconds
 * @param int seconds - how many bytes expected to receive
 *
 */
static void wait_seconds(int seconds) {
	time_t start, end;
	start = time(NULL);
	do {
		end = time(NULL);
	} while(end - start < seconds);
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
int swrngEnableCLPostProcessing(SwrngCLContext *ctxt, int postProcessingMethodId) {
	if (swrngIsCLOpen(ctxt) == SWRNG_FALSE) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	enableCLPostProcessing(ctxt, postProcessingMethodId); 	// Ignore the error
	ctxt->postProcessingMethodId = postProcessingMethodId;

	return SWRNG_SUCCESS;
}

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
int swrngDisableCLPostProcessing(SwrngCLContext *ctxt) {

	if (swrngIsCLOpen(ctxt) == SWRNG_FALSE) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	disableCLPostProcessing(ctxt); 	// Ignore the error
	ctxt->postProcessingEnabled = SWRNG_FALSE;

	return SWRNG_SUCCESS;

}

/**
* Disable post processing of raw random data for each device in a cluster.
* Applies only to devices of V1.2 and up
*
* @param ctxt - pointer to SwrngCLContext structure
* @return int - 0 when post processing was successfully disabled, otherwise the error code
*
*/
static int disableCLPostProcessing(SwrngCLContext *ctxt) {
	int status = SWRNG_SUCCESS;
	int i;
	for (i = 0; i < ctxt->actClusterSize; i++) {
		status = swrngDisablePostProcessing(&ctxt->tctxts[i].ctxt);
	}
	return status;
}

/**
* Enable post processing method.
*
* @param ctxt - pointer to SwrngContext structure
* @param postProcessingMethodId - post processing method id
*
* @return int - 0 when post processing successfully enabled, otherwise the error code
*
*/
static int enableCLPostProcessing(SwrngCLContext *ctxt, int postProcessingMethodId) {
	int status = SWRNG_SUCCESS;
	int i;
	for (i = 0; i < ctxt->actClusterSize; i++) {
		status = swrngEnablePostProcessing(&ctxt->tctxts[i].ctxt, postProcessingMethodId);
	}
	return status;
}

/**
* Set power profile for each device in the cluster
*
* @param ctxt - pointer to SwrngCLContext structure
* @param ppNum - new power profile number (0 through 9)
* @return int - 0 when power profile was set successfully
*
*/
int swrngSetCLPowerProfile(SwrngCLContext *ctxt, int ppNum) {

	if (swrngIsCLOpen(ctxt) == SWRNG_FALSE) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	setCLPowerProfile(ctxt, ppNum); 	// Ignore the error
	ctxt->ppnChanged = SWRNG_TRUE;
	ctxt->ppNum = ppNum;

	return SWRNG_SUCCESS;
}

/**
* Set power profile for each device in the cluster
*
* @param ctxt - pointer to SwrngCLContext structure
* @param ppNum - new power profile number (0 through 9)
* @return int - 0 when power profile was set successfully
*
*/
static int setCLPowerProfile(SwrngCLContext *ctxt, int ppNum) {
	int status = SWRNG_SUCCESS;
	int i;
	for (i = 0; i < ctxt->actClusterSize; i++) {
		status = swrngSetPowerProfile(&ctxt->tctxts[i].ctxt, ppNum);
	}
	return status;
}

/**
* Check to see if it is the time to re-size the cluster to preferred size
*
* @param ctxt - pointer to SwrngCLContext structure
* @return SWRNG_TRUE - it is time to resize the cluster
*
*/
static swrngBool isItTimeToResizeCluster(SwrngCLContext *ctxt) {
	if (swrngIsCLOpen(ctxt) == SWRNG_FALSE) {
		return SWRNG_FALSE;
	}

	time_t now = time(NULL);
	if (now - ctxt->clusterStartedSecs > clusterResizeWaitSecs &&
			ctxt->actClusterSize < ctxt->clusterSize) {
		// Time to try re-sizing the cluster
		return SWRNG_TRUE;
	}
	return SWRNG_FALSE;
}

/**
* Call this function to enable printing error messages to the error stream
* @param ctxt - pointer to SwrngCLContext structure
*/
void swrngEnableCLPrintingErrorMessages(SwrngCLContext *ctxt) {
	if (isContextCLInitialized(ctxt) == SWRNG_FALSE) {
		return;
	}
	ctxt->enPrintErrorMessages = SWRNG_TRUE;
}
