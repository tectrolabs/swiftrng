/*
 * swrng-cl-api.h
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
 Please read the provided documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with TectroLabs devices.

 This program may require 'sudo' permissions when running on Linux or macOS.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef SWRNG_CL_API_H_
#define SWRNG_CL_API_H_

#include <swrngapi.h>

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
 #include <string.h>
 #include <errno.h>
 #include <stdlib.h>
 #include <stdio.h>
#endif


/**
 * Thread context structure
 */
typedef struct {

	/* A context reference to a single device */
	SwrngContext ctxt;

	/* Thread handle and synchronization variables */
#if defined(_WIN32)
	HANDLE dwnl_thread;
	HANDLE dwnl_thread_event;
#else
	pthread_t dwnl_thread;
	pthread_mutex_t dwnl_mutex;
	pthread_cond_t dwnl_synch;
#endif

	/* 1 - thread is marked for destruction, 0 - otherwise */
	volatile int destroy_dwnl_thread_req;

	/* 1 - thread has a device operation in progress, 0 - otherwise */
	volatile int dwnl_req_active;

	/* 1 - thread is in a good state, 0 - thread encountered a device error */
	volatile int dwnl_status;

	/* A pointer to a memory location used for retrieving random byte stream from the device */
	unsigned char *thread_device_data_buffer;

} SwrngThreadContext;

/**
 * Cluster context structure
 */
typedef struct {
	/* Used for context sanity check */
	int sig_begin_data;

	/* 1 - to print error messages, 0 - otherwise */
	int enable_print_err_msg;

	/* Last cluster recorded error message */
	char last_err_msg[256];

	/* Cluster temporary storage for recorded error message */
	char tmp_err_msg[256];

	/* An array of SwrngThreadContext entries */
	SwrngThreadContext *tctxts;

	/* 1 - if cluster was successfully open, 0 - otherwise */
	int is_cluster_open;

	/* Preferred cluster size - number of devices in a cluster */
	int cluster_size;

	/* Actual cluster size - actual number of devices in a cluster */
	int actual_cluster_size;

	/* Final TRNG output buffer for cluster */
	unsigned char *out_data_buff;

	/* Current index used with `out_data_buff` */
	int cur_out_data_buff_idx;

	/* Power profile number of the cluster */
	int ppn_number;

	/* 1 - if the power profile number changed for the cluster, 0 - otherwise */
	int ppn_changed;

	/* How many times cluster recovered from errors */
	long num_cl_failover_events;

	/* How many times cluster attempted to resize */
	long num_cl_resize_events;

	/* Time when cluster open successfully */
	time_t cl_start_time_secs;

	/* 1 - if post processing is enabled for all cluster devices, 0 - otherwise */
	int data_post_process_enabled;

	/* 1 - if statistical tests are enabled for all cluster devices, 0 - otherwise */
	int stat_tests_enabled;

	/* -1 if not in use, 0 for SHA256 (default), 1 for xorshift64 (devices with versions 1.2 and up), 2 for SHA512 */
	int post_processing_method_id;

	/* Used for context sanity check */
	int sig_end_block;
} SwrngCLContext;


/**
 * API function declaration section
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
* @param int cluster_size - preferred cluster size
* @return int - 0 when processed successfully
*/
int swrngCLOpen(SwrngCLContext *ctxt, int cluster_size);

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
* Post processing can only be disabled for devices with versions 1.2 or greater.
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
* @param pp_method_id - 0 for SHA256 (default), 1 - xorshift64 (devices with versions 1.2 or grater), 2 - for SHA512
*
* @return int - 0 when post processing successfully enabled, otherwise the error code
*
*/
int swrngEnableCLPostProcessing(SwrngCLContext *ctxt, int pp_method_id);

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
int swrngDisableCLStatisticalTests(SwrngCLContext *ctxt);

/**
* Enable statistical tests on raw random data stream for each device in a cluster.
*
* @param ctxt - pointer to SwrngContext structure
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*
*/
int swrngEnableCLStatisticaTests(SwrngCLContext *ctxt);

#ifdef __cplusplus
}
#endif


#endif /* SWRNG_CL_API_H_ */
