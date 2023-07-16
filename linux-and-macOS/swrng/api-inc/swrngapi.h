/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements a C wrapper around the C++ API for interacting with the SwiftRNG device.

 */

/**
 *    @file SwiftRngApiCWrapper.h
 *    @date 7/8/2023
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Implements a C API wrapper around the C++ API for interacting with the SwiftRNG device.
 */
#ifndef _SWRNGAPI_H_
#define _SWRNGAPI_H_

#include <ApiStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define a type for referencing the API context */
typedef struct SwrngContext {
	uint32_t sig_begin;
	void *api;
	uint32_t sig_end;
} SwrngContext;

/**
* Initialize SwrngContext context. The context must be initialized before making any other API calls.
*
* @param ctxt - pointer to SwrngContext structure
* @return 0 - if context initialized successfully, -1 if context is null
*/
int swrngInitializeContext(SwrngContext *ctxt);

/**
* Destroy SwrngContext context. It will cleanup all allocated resources associated with the context.
*
* @param ctxt - pointer to SwrngContext structure
* @return 0 - if context destroyed successfully
*/
int swrngDestroyContext(SwrngContext *ctxt);

/**
* Open SwiftRNG USB specific device.
*
* @param ctxt - pointer to SwrngContext structure
* @param int dev_num - device number (0 - for the first device or only one device)
* @return int 0 - when open successfully or error code
*/
int swrngOpen(SwrngContext *ctxt, int dev_num);

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
* @param NoiseSourceRawData *noise_source_raw_data - a pointer to a structure for holding 16,000 of raw data
* @param int noise_source_num - noise source number (0 - first noise source, 1 - second one)
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetRawDataBlock(SwrngContext *ctxt, NoiseSourceRawData *noise_source_raw_data, int noise_source_num);

/**
* A function for retrieving frequency tables of the random numbers generated from random sources.
* There is one frequency table per noise source. Each table consist of 256 integers (16 bit) that represent
* frequencies for the random numbers generated between 0 and 255. These tables are used for checking the quality of the
* noise sources and for detecting partial or total failures associated with those sources.
*
* SwiftRNG has two frequency tables. You will need to call this method twice - one call per frequency table.
*
* @param ctxt - pointer to SwrngContext structure
* @param FrequencyTables *frequency_tables - a pointer to the frequency tables structure
* @param int tableNumber - a frequency table number (0 or 1)
* @return 0 - successful operation, otherwise the error code
*
*/
int swrngGetFrequencyTables(SwrngContext *ctxt, FrequencyTables *frequency_tables);

/**
* Retrieve a complete list of SwiftRNG devices currently plugged into USB ports.
* It only should be called when there is no connection established for the context.
*
* @param ctxt - pointer to SwrngContext structure
* @param DeviceInfoList* dev_info_list - pointer to the structure for holding SwiftRNG info list
* @return int - value 0 when retrieved successfully, otherwise the error code
*
*/
int swrngGetDeviceList(SwrngContext *ctxt, DeviceInfoList *dev_info_list);

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
* @param DeviceSerialNumber* serial_number - pointer to a structure for holding SwiftRNG device S/N
* @return int - 0 when serial number retrieved successfully, otherwise the error code
*
*/
int swrngGetSerialNumber(SwrngContext *ctxt, DeviceSerialNumber *serial_number);

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
* @param power_profile_number - new power profile number (0 through 9)
* @return int - 0 when power profile was set successfully, otherwise the error code
*
*/
int swrngSetPowerProfile(SwrngContext *ctxt, int power_profile_number);

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
* @param post_processing_status - pointer to store the post processing status (1 - enabled or 0 - otherwise)
* @return int - 0 when post processing flag successfully retrieved, otherwise the error code
*
*/
int swrngGetPostProcessingStatus(SwrngContext *ctxt, int *post_processing_status);

/**
* Check to see if statistical tests are enabled on raw data stream for device.
*
* @param ctxt - pointer to SwrngContext structure
* @param statistical_tests_enabled_status - pointer to store statistical tests status (1 - enabled or 0 - otherwise)
* @return int - 0 when statistical tests flag successfully retrieved, otherwise the error code
*
*/
int swrngGetStatisticalTestsStatus(SwrngContext *ctxt, int *statistical_tests_enabled_status);

/**
* Retrieve post processing method
*
* @param ctxt - pointer to SwrngContext structure
* @param post_processing_method_id - pointer to store the post processing method id
* @return int - 0 when post processing method successfully retrieved, otherwise the error code
*
*/
int swrngGetPostProcessingMethod(SwrngContext *ctxt, int *post_processing_method_id);

/**
* Retrieve device embedded correction method
*
* @param ctxt - pointer to SwrngContext structure
* @param dev_emb_corr_method_id - pointer to store the device built-in correction method id:
* 			0 - none, 1 - Linear correction (P. Lacharme)
* @return int - 0 embedded correction method successfully retrieved, otherwise the error code
*
*/
int swrngGetEmbeddedCorrectionMethod(SwrngContext *ctxt, int *dev_emb_corr_method_id);

/**
* Enable post processing method.
*
* @param ctxt - pointer to SwrngContext structure
* @param post_processing_method_id - 0 for SHA256 (default), 1 - xorshift64 (devices with versions 1.2 and up), 2 - for SHA512
*
* @return int - 0 when post processing successfully enabled, otherwise the error code
*
*/
int swrngEnablePostProcessing(SwrngContext *ctxt, int post_processing_method_id);

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

/**
* Retrieve Max number of adaptive proportion test failures encountered per data block
*
* @param ctxt - pointer to SwrngContext structure
* @param max_apt_failures_per_block pointer to the max number of failures
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*/
int swrngGetMaxAptFailuresPerBlock(SwrngContext *ctxt, uint16_t *max_apt_failures_per_block);

/**
* Retrieve Max number of repetition count test failures encountered per data block
*
* @param ctxt - pointer to SwrngContext structure
* @param max_rct_failures_per_block pointer to the max number of failures
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*/
int swrngGetMaxRctFailuresPerBlock(SwrngContext *ctxt, uint16_t *max_rct_failures_per_block);



#ifdef __cplusplus
}
#endif

#endif /* _SWRNGAPI_H_ */
