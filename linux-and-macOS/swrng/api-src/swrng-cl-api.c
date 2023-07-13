/*
 * swrng-cl-api.c
 * Ver. 2.2
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2023 TectroLabs, https://tectrolabs.com

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

#include <swrng-cl-api.h>

/**
 * Error messages
 */
static const char clusterAlreadyOpenErrMsg[] = "Cluster already open";
static const char clusterNotOpenErrMsg[] = "Cluster not open";
static const char clusterSizeInvalidErrMsg[] = "Cluster size must be between 1 and 10";
static const char clusterNotAvailableErrMsg[] =  "Failed to form a cluster, check for available SwiftRNG devices";
static const char lowMemoryErrMsg[] =  "Cannot allocate a memory block to continue";
static const char leakMemoryErrMsg[] = "Memory block already allocated";
static const char ctxtNotInitializedErrMsg[] = "SwrngCLContext not initialized";
static const char needMoreCPUsErrMsg[] = "Need more CPUs available to continue";
static const char threadCreationErrMsg[] = "Thread creation error";
#ifdef _WIN32
static const char eventCreationErrMsg[] = "Event creation error";
#endif

static const char eventSynchErrMsg[] = "Event synchronization error";

static const long c_out_data_buff_size = 100000L;

/* Seconds to wait before starting the fail-over event after device errors are detected */
static const int c_cl_failover_wait_secs = 6;

/* Seconds to wait before trying to re-cluster to get the preferred size */
static const int c_cl_resize_wait_secs = 60 * 60;

/* A cluster size beyond 10 has not been tested yet */
static const int c_max_sl_size = 10;

/* Context sanity check markers */
static const int c_cl_ctxt_sig_begin = 12321;
static const int c_cl_ctxt_sig_end = 57321;

/* ID for marking a thread status in error */
static int thread_event_err_id = 10;

/* Constants for true false values used by this API */
static const int c_cl_api_true = 1;
static const int c_cl_api_false = 0;

/**
 * Declarations for local functions
 */
static void wait_seconds(int seconds);
static int isContextCLInitialized(SwrngCLContext *ctxt);
static int isItTimeToResizeCluster(SwrngCLContext *ctxt);
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
static void errorCleanUpEventsAndThreads(SwrngCLContext *ctxt, int maxIndex);
#endif
static void wait_complete_download_req(SwrngThreadContext *ctxt);
static void wait_all_complete_download_reqs(SwrngCLContext *ctxt);
static int initializeCLThreads(SwrngCLContext *ctxt);
static void unInitializeCLThreads(SwrngCLContext *ctxt);
static void trigger_download_reqs(SwrngCLContext *ctxt);
static int getClusterDownloadStatus(SwrngCLContext *ctxt);
static int disableCLPostProcessing(SwrngCLContext *ctxt);
static int disableCLStatisticalTests(SwrngCLContext *ctxt);
static int enableCLPostProcessing(SwrngCLContext *ctxt, int postProcessingMethodId);
static int enableCLStatisticalTests(SwrngCLContext *ctxt);

/**
* Retrieve the last error message.
* The caller should make a copy of the error message returned immediately after calling this function.
* @param ctxt - pointer to SwrngCLContext structure
* @return - pointer to the error message
*/
const char* swrngGetCLLastErrorMessage(SwrngCLContext *ctxt) {

	if (isContextCLInitialized(ctxt) == c_cl_api_false) {
		return ctxtNotInitializedErrMsg;
	}

	if (ctxt->last_err_msg == (char*)NULL) {
		strcpy(ctxt->last_err_msg, "");
	}
	return ctxt->last_err_msg;
}

/**
* Trigger a new download requests
*
* @param ctxt - pointer to SwrngCLContext structure
*/
static void trigger_download_reqs(SwrngCLContext *ctxt) {
	int i;
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		ctxt->tctxts[i].dwnl_req_active = c_cl_api_true;
#ifndef _WIN32
		pthread_cond_signal(&ctxt->tctxts[i].dwnl_synch);
#else
		SetEvent(ctxt->tctxts[i].dwnl_thread_event);
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
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		if (ctxt->tctxts[i].dwnl_status != SWRNG_SUCCESS) {
			retval = ctxt->tctxts[i].dwnl_status;
			if (retval == thread_event_err_id) {
				strcpy(ctxt->last_err_msg, eventSynchErrMsg);
			}
			else {
				strcpy(ctxt->last_err_msg, swrngGetLastErrorMessage(&ctxt->tctxts[i].ctxt));
			}
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
		ctxt->cur_out_data_buff_idx = 0;
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

	if (swrngIsCLOpen(ctxt) != c_cl_api_true) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -ENODEV;
	}

	total = 0;
	do {
		if (ctxt->cur_out_data_buff_idx >= (ctxt->actual_cluster_size * c_out_data_buff_size)) {

			if (isItTimeToResizeCluster(ctxt)) {
				ctxt->num_cl_resize_events++;
				swrngCLClose(ctxt);
				wait_seconds(c_cl_failover_wait_secs);
				status = swrngCLOpen(ctxt, ctxt->cluster_size);
				if (status != SWRNG_SUCCESS) {
					return status;
				}
			}
			retval = getEntropyBytes(ctxt);
			if (retval != SWRNG_SUCCESS) {
				strcpy(ctxt->tmp_err_msg, ctxt->last_err_msg);
				status = retval;
				/* Got en error, restart the cluster */
				ctxt->num_cl_failover_events++;
				swrngCLClose(ctxt);
				wait_seconds(c_cl_failover_wait_secs);
				retval = swrngCLOpen(ctxt, ctxt->cluster_size);
				if (retval != SWRNG_SUCCESS) {
					strcpy(ctxt->last_err_msg, ctxt->tmp_err_msg);
					return status;
				}
				retval = getEntropyBytes(ctxt);
			}
		}
		if (retval == SWRNG_SUCCESS) {
			act = (ctxt->actual_cluster_size * c_out_data_buff_size) - ctxt->cur_out_data_buff_idx;
			if (act > (length - total)) {
				act = (length - total);
			}
			memcpy(buffer + total, ctxt->out_data_buff + ctxt->cur_out_data_buff_idx, act);
			ctxt->cur_out_data_buff_idx += act;
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
* @param int clusterSize - preferred cluster size
* @return int - 0 when processed successfully
*/
int swrngCLOpen(SwrngCLContext *ctxt, int clusterSize) {
	SwrngContext ctxtSearch;
	DeviceInfoList devInfoList;
	DeviceVersion version;

	if (isContextCLInitialized(ctxt) == c_cl_api_false) {
		initializeCLContext(ctxt);
	} else {
		if (swrngIsCLOpen(ctxt) == c_cl_api_true) {
			printCLErrorMessage(ctxt, clusterAlreadyOpenErrMsg);
			return -1;
		}
		if (clusterSize <= 0 || clusterSize > c_max_sl_size) {
			printCLErrorMessage(ctxt, clusterSizeInvalidErrMsg);
			return -1;
		}
	}

	ctxt->cluster_size = clusterSize;
#ifndef _WIN32
	int numCores = (int)sysconf(_SC_NPROCESSORS_ONLN);
	if (numCores <= clusterSize / 2) {
		printCLErrorMessage(ctxt, needMoreCPUsErrMsg);
		return -1;
	}
#endif
	/* Look for compatible devices for the cluster */
	swrngInitializeContext(&ctxtSearch);
	int retVal = swrngGetDeviceList(&ctxtSearch, &devInfoList);
	if (retVal != SWRNG_SUCCESS) {
		swrngDestroyContext(&ctxtSearch);
		return retVal;
	}

	if (devInfoList.numDevs == 0) {
		printCLErrorMessage(ctxt, clusterNotAvailableErrMsg);
		swrngDestroyContext(&ctxtSearch);
		return -1;
	}

	if (ctxt->tctxts != NULL) {
		printCLErrorMessage(ctxt, leakMemoryErrMsg);
		swrngDestroyContext(&ctxtSearch);
		return -1;
	}
	ctxt->tctxts = (SwrngThreadContext *)calloc(ctxt->cluster_size, sizeof(SwrngThreadContext));
	if (ctxt->tctxts == NULL) {
		printCLErrorMessage(ctxt, lowMemoryErrMsg);
		swrngDestroyContext(&ctxtSearch);
		return -1;
	}

	int i;
	ctxt->actual_cluster_size = 0;
	for (i = 0; i < devInfoList.numDevs && ctxt->actual_cluster_size < ctxt->cluster_size; i++) {
		swrngInitializeContext(&ctxt->tctxts[i].ctxt);
		retVal = swrngOpen(&ctxt->tctxts[i].ctxt, devInfoList.devInfoList[i].devNum);
		if (retVal != SWRNG_SUCCESS) {
			swrngDestroyContext(&ctxt->tctxts[i].ctxt);
			continue;
		}
		retVal = swrngGetVersion(&ctxt->tctxts[i].ctxt, &version);
		if (retVal != SWRNG_SUCCESS) {
			swrngDestroyContext(&ctxt->tctxts[i].ctxt);
			continue;
		}
		if (i > ctxt->actual_cluster_size) {
			ctxt->tctxts[ctxt->actual_cluster_size].ctxt = ctxt->tctxts[i].ctxt;
		}
		ctxt->actual_cluster_size++;
	}

	if (ctxt->actual_cluster_size == 0) {
		printCLErrorMessage(ctxt, clusterNotAvailableErrMsg);
		swrngDestroyContext(&ctxtSearch);
		freeAllocatedMemory(ctxt);
		return -1;
	}

	int status = allocateMemory(ctxt);
	if ( status != SWRNG_SUCCESS) {
		swrngDestroyContext(&ctxtSearch);
		freeAllocatedMemory(ctxt);
		return status;
	}

	swrngDestroyContext(&ctxtSearch);

	ctxt->cur_out_data_buff_idx = ctxt->actual_cluster_size * c_out_data_buff_size;
	status = initializeCLThreads(ctxt);
	if ( status != SWRNG_SUCCESS) {
		freeAllocatedMemory(ctxt);
		return status;
	}

	if (ctxt->ppn_changed == c_cl_api_true) {
	 	/* Ignore the error */
		setCLPowerProfile(ctxt, ctxt->ppn_number);
	}

	if (ctxt->data_post_process_enabled == c_cl_api_false) {
		/* Ignore any error */
		disableCLPostProcessing(ctxt);
	} else {
		if (ctxt->post_processing_method_id != -1) {
			/* Ignore the error */
			enableCLPostProcessing(ctxt, ctxt->post_processing_method_id);
		}
	}

	if (ctxt->stat_tests_enabled == c_cl_api_false) {
		/* Ignore any error */
		disableCLStatisticalTests(ctxt);
	}

	ctxt->cl_start_time_secs = time(NULL);
	ctxt->is_cluster_open = c_cl_api_true;

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
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		ctxt->tctxts[i].destroy_dwnl_thread_req = c_cl_api_false;
		ctxt->tctxts[i].dwnl_req_active = c_cl_api_false;
		ctxt->tctxts[i].dwnl_status = SWRNG_SUCCESS;
		ctxt->tctxts[i].thread_device_data_buffer = ctxt->out_data_buff + (i * c_out_data_buff_size);
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
				ctxt->tctxts[j].destroy_dwnl_thread_req = c_cl_api_true;
				pthread_cond_signal(&ctxt->tctxts[j].dwnl_synch);
				pthread_join(ctxt->tctxts[j].dwnl_thread, &th_retval);
				pthread_mutex_destroy(&ctxt->tctxts[j].dwnl_mutex);
				pthread_cond_destroy(&ctxt->tctxts[j].dwnl_synch);

			}
			return -1;
		}
#else
		ctxt->tctxts[i].dwnl_thread_event = CreateEvent(NULL, FALSE, FALSE, TEXT("Thread Wakeup Event"));
		if (ctxt->tctxts[i].dwnl_thread_event == NULL) {
			printCLErrorMessage(ctxt, eventCreationErrMsg);
			errorCleanUpEventsAndThreads(ctxt, i);
			return -1;
		}
		ctxt->tctxts[i].dwnl_thread = (HANDLE)_beginthreadex(0, 0, &download_thread, (void*)&ctxt->tctxts[i], CREATE_SUSPENDED, 0);
		if (ctxt->tctxts[i].dwnl_thread == NULL) {
			CloseHandle(ctxt->tctxts[i].dwnl_thread_event);
			printCLErrorMessage(ctxt, threadCreationErrMsg);
			errorCleanUpEventsAndThreads(ctxt, i);
			return -1;
		}
		ResumeThread(ctxt->tctxts[i].dwnl_thread);
#endif
	}

	return status;
}

#ifdef _WIN32
/**
* Clean up all previous threads and events 
*
* @param ctxt - pointer to SwrngCLContext structure
* @param maxIndex - upper bound of threads and events to close
*/
static void errorCleanUpEventsAndThreads(SwrngCLContext *ctxt, int maxIndex) {
	for (int j = 0; j < maxIndex; j++) {
		ctxt->tctxts[j].destroyDwnlThreadReq = c_cl_api_true;
		SetEvent(ctxt->tctxts[j].dwnl_thread_event);
		WaitForSingleObject(ctxt->tctxts[j].dwnl_thread, INFINITE);
		CloseHandle(ctxt->tctxts[j].dwnl_thread);
		CloseHandle(ctxt->tctxts[j].dwnl_thread_event);
	}
}
#endif

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

	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		wait_complete_download_req(&ctxt->tctxts[i]);
		ctxt->tctxts[i].destroy_dwnl_thread_req = c_cl_api_true;
#ifndef _WIN32
		pthread_cond_signal(&ctxt->tctxts[i].dwnl_synch);
		pthread_join(ctxt->tctxts[i].dwnl_thread, &th_retval);
		pthread_mutex_destroy(&ctxt->tctxts[i].dwnl_mutex);
		pthread_cond_destroy(&ctxt->tctxts[i].dwnl_synch);
#else
		SetEvent(ctxt->tctxts[i].dwnl_thread_event);
		WaitForSingleObject(ctxt->tctxts[i].dwnl_thread, INFINITE);
		CloseHandle(ctxt->tctxts[i].dwnl_thread);
		CloseHandle(ctxt->tctxts[i].dwnl_thread_event);
#endif
		ctxt->tctxts[i].dwnl_req_active = c_cl_api_false;
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
	int status;

	if (swrngIsCLOpen(ctxt) == c_cl_api_false) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	unInitializeCLThreads(ctxt);

	int i;
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		/* Ignore the previous error */
		status = swrngDestroyContext(&ctxt->tctxts[i].ctxt);
		if (status != SWRNG_SUCCESS) {
			retVal = status;
		}
	}

	freeAllocatedMemory(ctxt);
	strcpy(ctxt->last_err_msg, "");

	ctxt->is_cluster_open = c_cl_api_false;
	return retVal;
}

/**
* Retrieve number of devices currently in use by the cluster
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of devices used by the cluster
*/
int swrngGetCLSize(SwrngCLContext *ctxt) {

	if (swrngIsCLOpen(ctxt) != c_cl_api_true) {
		return 0;
	}
	return ctxt->actual_cluster_size;
}

/**
* Retrieve number of cluster fail-over events. That number will be incremented each
* time the cluster is trying to recover as a result of device errors.
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of cluster fail-over events
*/
long swrngGetCLFailoverEventCount(SwrngCLContext *ctxt) {
	if (swrngIsCLOpen(ctxt) != c_cl_api_true) {
		return 0;
	}
	return ctxt->num_cl_failover_events;
}

/**
* Retrieve number of cluster resize attempts. That number will be incremented each
* time the cluster is trying to resize to reach the preferred size.
* @param ctxt - pointer to SwrngCLContext structure
* @return - number of cluster resize attempts
*/
long swrngGetCLResizeAttemptCount(SwrngCLContext *ctxt) {
	if (swrngIsCLOpen(ctxt) != c_cl_api_true) {
		return 0;
	}
	return ctxt->num_cl_resize_events;
}



/**
* Allocated memory resources
*
* @param ctxt - pointer to SwrngCLContext structure
* @return int - 0 when processed successfully
*/
static int allocateMemory(SwrngCLContext *ctxt) {
	int status = SWRNG_SUCCESS;

	if (ctxt->out_data_buff != NULL) {
		status = -1;
	}

	if (status != SWRNG_SUCCESS) {
		printCLErrorMessage(ctxt, leakMemoryErrMsg);
		return -1;
	}

	ctxt->out_data_buff = (unsigned char *)calloc(ctxt->actual_cluster_size, c_out_data_buff_size);
	if (ctxt->out_data_buff == NULL) {
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
		if (ctxt->out_data_buff != NULL) {
			free(ctxt->out_data_buff);
			ctxt->out_data_buff = NULL;
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
* @return c_cl_api_true - context SI initialized
*/
static int isContextCLInitialized(SwrngCLContext *ctxt) {
	int retVal = c_cl_api_false;
	if (ctxt != NULL) {
		if (ctxt->sig_begin_data == c_cl_ctxt_sig_begin
			&& ctxt->sig_end_block == c_cl_ctxt_sig_end) {
			retVal = c_cl_api_true;
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
		ctxt->sig_begin_data = c_cl_ctxt_sig_begin;
		ctxt->sig_end_block = c_cl_ctxt_sig_end;
		ctxt->data_post_process_enabled = c_cl_api_true;
		ctxt->stat_tests_enabled = c_cl_api_true;
		ctxt->post_processing_method_id = -1;
		return SWRNG_SUCCESS;
	}
	return -1;
}

/**
* Check if cluster is open
* @param ctxt - pointer to SwrngCLContext structure
* @return int - 0 when cluster is open
*/
int swrngIsCLOpen(SwrngCLContext *ctxt) {

	if (isContextCLInitialized(ctxt) == c_cl_api_false) {
		return c_cl_api_false;
	}
	return ctxt->is_cluster_open;
}

/**
 * Print and/or save error message
 * @param ctxt - pointer to SwrngCLContext structure
 * @param errMsg - pointer to error message
 */
static void printCLErrorMessage(SwrngCLContext *ctxt, const char* errMsg) {
	if (ctxt->enable_print_err_msg) {
		fprintf(stderr, "%s", errMsg);
		fprintf(stderr, "\n");
	}
	if (strlen(errMsg) >= sizeof(ctxt->last_err_msg)) {
		strcpy(ctxt->last_err_msg, "Error message too long");
	} else {
		strcpy(ctxt->last_err_msg, errMsg);
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
		ctxt->dwnl_req_active = c_cl_api_false;
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

	while (1) {
		start = time(NULL);
		timeout.tv_sec = start + 1;
		timeout.tv_nsec = 0;
		rc = pthread_cond_timedwait(&tctxt->dwnl_synch, &tctxt->dwnl_mutex, &timeout);
		switch (rc) {
		case 0:
		case ETIMEDOUT:
			if (tctxt->destroy_dwnl_thread_req == c_cl_api_true) {
				goto exit;
			} else {
				if (tctxt->dwnl_req_active == c_cl_api_true) {
					tctxt->dwnl_status = swrngGetEntropy(&tctxt->ctxt, tctxt->thread_device_data_buffer, c_out_data_buff_size);
					tctxt->dwnl_req_active = c_cl_api_false;
				}
			}
			break;
		default:
			tctxt->dwnl_status = thread_event_err_id;
			tctxt->dwnl_req_active = c_cl_api_false;
			if (tctxt->destroy_dwnl_thread_req == c_cl_api_true) {
				goto exit;
			}
		}
	}
exit:
	pthread_exit(NULL);
	pthread_cleanup_pop(0);
}
#endif

#ifdef _WIN32
unsigned int __stdcall download_thread(void *th_params) {
	SwrngThreadContext *tctxt = (SwrngThreadContext *)th_params;
	DWORD dwWaitResult;

	while (true) {
		dwWaitResult = WaitForSingleObject(tctxt->dwnl_thread_event, 1);
		switch (dwWaitResult) {
		case WAIT_OBJECT_0:
		case WAIT_TIMEOUT:
			if (tctxt->destroyDwnlThreadReq == c_cl_api_true) {
				return 0;
			} else {
				if (tctxt->dwnlRequestActive == c_cl_api_true) {
					tctxt->dwnl_status = swrngGetEntropy(&tctxt->ctxt, tctxt->trngOutBuffer, c_out_data_buff_size);
					tctxt->dwnlRequestActive = c_cl_api_false;
				}
			}
			break;
		default:
			tctxt->dwnl_status = thread_event_err_id;
			tctxt->dwnlRequestActive = c_cl_api_false;
			if (tctxt->destroyDwnlThreadReq == c_cl_api_true) {
				return 0;
			}
		}
	}
}
#endif

/**
* Wait for download request to complete
*
* @param ctxt - pointer to SwrngThreadContext structure
* @return c_cl_api_true - context initialized
*/
static void wait_complete_download_req(SwrngThreadContext *ctxt) {
	while(ctxt->dwnl_req_active == c_cl_api_true) {
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
* @return c_cl_api_true - context initialized
*/
static void wait_all_complete_download_reqs(SwrngCLContext *ctxt) {
	int i;
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
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
* @param postProcessingMethodId - 0 for SHA256 (default), 1 - xorshift64 (devices with versions 1.2 or grater), 2 - for SHA512
*
* @return int - 0 when post processing successfully enabled, otherwise the error code
*
*/
int swrngEnableCLPostProcessing(SwrngCLContext *ctxt, int postProcessingMethodId) {
	if (swrngIsCLOpen(ctxt) == c_cl_api_false) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	/* Ignore any error */
	enableCLPostProcessing(ctxt, postProcessingMethodId);
	ctxt->post_processing_method_id = postProcessingMethodId;

	return SWRNG_SUCCESS;
}

/**
* Disable post processing of raw random data for each device in a cluster.
* Post processing can only be disabled for devices with versions 1.2 or greater.
*
* To disable post processing, call this function immediately after cluster is successfully open.
*
* @param ctxt - pointer to SwrngCLContext structure
* @return int - 0 when post processing was successfully disabled, otherwise the error code
*
*/
int swrngDisableCLPostProcessing(SwrngCLContext *ctxt) {

	if (swrngIsCLOpen(ctxt) == c_cl_api_false) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	/* Ignore any error */
	disableCLPostProcessing(ctxt);
	ctxt->data_post_process_enabled = c_cl_api_false;

	return SWRNG_SUCCESS;
}

/**
* Disable statistical tests on raw random data stream for each device in a cluster.
* Statistical tests are initially enabled for all devices.
*
* To disable statistical tests, call this function immediately after cluster is successfully open.
*
* @param ctxt - pointer to SwrngContext structure
* @return int - 0 when statistical tests successfully disabled, otherwise the error code
*
*/
int swrngDisableCLStatisticalTests(SwrngCLContext *ctxt) {
	if (swrngIsCLOpen(ctxt) == c_cl_api_false) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	/* Ignore any error */
	disableCLStatisticalTests(ctxt);
	ctxt->stat_tests_enabled = c_cl_api_false;

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
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		status = swrngDisablePostProcessing(&ctxt->tctxts[i].ctxt);
	}
	return status;
}

/**
* Disable statistical tests on raw random data for each device in a cluster.
*
* @param ctxt - pointer to SwrngCLContext structure
* @return int - 0 when statistical tests successfully disabled, otherwise the error code
*
*/
static int disableCLStatisticalTests(SwrngCLContext *ctxt) {
	int status = SWRNG_SUCCESS;
	int i;
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		status = swrngDisableStatisticalTests(&ctxt->tctxts[i].ctxt);
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
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		status = swrngEnablePostProcessing(&ctxt->tctxts[i].ctxt, postProcessingMethodId);
	}
	return status;
}

/**
* Enable statistical tests on raw random data stream for each device in a cluster.
*
* @param ctxt - pointer to SwrngContext structure
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*
*/
int swrngEnableCLStatisticaTests(SwrngCLContext *ctxt) {
	if (swrngIsCLOpen(ctxt) == c_cl_api_false) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	/* Ignore any error */
	enableCLStatisticalTests(ctxt);
	ctxt->stat_tests_enabled = c_cl_api_true;

	return SWRNG_SUCCESS;
}

/**
* Enable statistical tests on raw random data stream for each device in a cluster.
*
* @param ctxt - pointer to SwrngContext structure
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*
*/
static int enableCLStatisticalTests(SwrngCLContext *ctxt) {
	int status = SWRNG_SUCCESS;
	int i;
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		status = swrngEnableStatisticalTests(&ctxt->tctxts[i].ctxt);
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

	if (swrngIsCLOpen(ctxt) == c_cl_api_false) {
		printCLErrorMessage(ctxt, clusterNotOpenErrMsg);
		return -1;
	}

	/* Ignore the error */
	setCLPowerProfile(ctxt, ppNum);
	ctxt->ppn_changed = c_cl_api_true;
	ctxt->ppn_number = ppNum;

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
	for (i = 0; i < ctxt->actual_cluster_size; i++) {
		status = swrngSetPowerProfile(&ctxt->tctxts[i].ctxt, ppNum);
	}
	return status;
}

/**
* Check to see if it is the time to re-size the cluster to preferred size
*
* @param ctxt - pointer to SwrngCLContext structure
* @return c_cl_api_true - it is time to resize the cluster
*
*/
static int isItTimeToResizeCluster(SwrngCLContext *ctxt) {
	if (swrngIsCLOpen(ctxt) == c_cl_api_false) {
		return c_cl_api_false;
	}

	time_t now = time(NULL);
	if (now - ctxt->cl_start_time_secs > c_cl_resize_wait_secs &&
			ctxt->actual_cluster_size < ctxt->cluster_size) {
		/* Time to try re-sizing the cluster */
		return c_cl_api_true;
	}
	return c_cl_api_false;
}

/**
* Call this function to enable printing error messages to the error stream
* @param ctxt - pointer to SwrngCLContext structure
*/
void swrngEnableCLPrintingErrorMessages(SwrngCLContext *ctxt) {
	if (isContextCLInitialized(ctxt) == c_cl_api_false) {
		return;
	}
	ctxt->enable_print_err_msg = c_cl_api_true;
}
