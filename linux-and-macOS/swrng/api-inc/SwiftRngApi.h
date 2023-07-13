/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the API for interacting with the AlphaRNG device.

 */

/**
 *    @file SwiftRngApi.h
 *    @date 7/8/2022
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Implements the API for interacting with the SwiftRNG device.
 */
#ifndef SWIFTRNGAPI_H_
#define SWIFTRNGAPI_H_

#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cerrno>

#include <ApiStructs.h>

#define SWRNG_HDWARE_ID L"USB\\VID_1FC9&PID_8111"



#if defined _WIN32
	#include "libusb.h"
	#include <WinUsbSerialDevice.h>
#elif defined __FreeBSD__
	#include <libusb.h>
    #include <USBSerialDevice.h>
#else
	#include <libusb-1.0/libusb.h>
	#include <USBSerialDevice.h>
#endif

using namespace std;

namespace swiftrng {

class SwiftRngApi {

public:
	SwiftRngApi();
	int open(int devNum);
	int close();
	int is_open();
	int get_entropy(unsigned char *buffer, long length);
	int get_entropy_ex(unsigned char *buffer, long length);
	int get_raw_data_block(NoiseSourceRawData *noise_source_raw_data, int noise_source_num);
	int get_frequency_tables(FrequencyTables *frequency_tables);
	int get_device_list(DeviceInfoList *dev_info_list);
	int get_model(DeviceModel *model);
	int get_version(DeviceVersion *version);
	int get_version_number(double *version);
	int get_serial_number(DeviceSerialNumber *serial_number);
	void reset_statistics();
	int set_power_profile(int power_profile_number);
	int run_device_diagnostics();
	int disable_post_processing();
	int disable_statistical_tests();
	int get_post_processing_status(int *post_processing_status);
	int get_statistical_tests_status(int *statistical_tests_enabled_status);
	int get_post_processing_method(int *post_processing_method_id);
	int get_embedded_correction_method(int *dev_emb_corr_method_id);
	int enable_post_processing(int post_processing_method_id);
	int enable_statistical_tests();
	DeviceStatistics* generate_device_statistics();
	const char* get_last_error_message();
	string get_last_error_log() {return m_last_error_log_oss.str();}
	void enable_printing_error_messages();
	int get_max_apt_failures_per_block(uint16_t *max_apt_failures_per_block);
	int get_max_rct_failures_per_block(uint16_t *max_rct_failures_per_block);


	virtual	~SwiftRngApi();


private:
	bool is_context_initialized() { return m_is_initialized; }
	void initialize();
	void clear_last_error_msg();
	void close_USB_lib();
	void rct_initialize();
	void rct_restart();
	void apt_initialize();
	void apt_restart();

	void sha256_initialize();
	void sha512_initialize();
	void sha256_stampSerialNumber(void *input_block);
	void sha256_initializeSerialNumber(uint32_t init_value);
	int sha512_generateHash(uint64_t *src, int16_t len, uint64_t *dst);
	void sha512_hashCurrentBlock();
	uint64_t sha512_sigma1(uint64_t *x);
	int sha256_selfTest();
	int sha512_selfTest();
	int xorshift64_selfTest();
	uint32_t sha256_ch(uint32_t *x, uint32_t *y, uint32_t *z);
	uint32_t sha256_maj(uint32_t *x, uint32_t *y, uint32_t *z);
	uint32_t sha256_sum0(uint32_t *x);
	uint32_t sha256_sum1(uint32_t *x);
	uint32_t sha256_sigma0(uint32_t *x);
	uint32_t sha256_sigma1(uint32_t *x);
	uint64_t sha512_ch(uint64_t *x, uint64_t *y, uint64_t *z);
	uint64_t sha512_maj(uint64_t *x, uint64_t *y, uint64_t *z);
	uint64_t sha512_sum0(uint64_t *x);
	uint64_t sha512_sum1(uint64_t *x);
	uint64_t sha512_sigma0(uint64_t *x);
	void sha256_hashCurrentBlock();
	int sha256_generateHash(uint32_t *src, int16_t len, uint32_t *dst);
	void swrng_printErrorMessage(const string &err_msg);
	int swrng_handleDeviceVersion();
	void swrng_clearReceiverBuffer();
	uint64_t xorshift64_postProcessWord(uint64_t raw_word);
	void xorshift64_postProcessWords(uint64_t *buffer, int num_elements);
	void xorshift64_postProcess(uint8_t *buffer, int num_elements);
	int swrng_snd_rcv_usb_data(char *snd, int size_snd, char *rcv, int size_rcv, int op_timeout_secs);
	int swrng_chip_read_data(char *buff, int length, int op_timeout_secs);
	void swrng_contextReset();
	int swrng_getEntropyBytes();
	int swrng_rcv_rnd_bytes();
	void test_samples();
	void swrng_updateDevInfoList(DeviceInfoList* dev_info_list, int *curt_found_dev_num);
	uint32_t rotr32(uint32_t sb, uint32_t w) { return (((w) >> (sb)) | ((w) << (32-(sb)))); }
	uint64_t rotr64(uint64_t sb, uint64_t w) { return (((w) >> (sb)) | ((w) << (64-(sb)))); }

private:

	// USB vendor id for SwiftRNG devices
	const int c_usb_vendor_id {0x1fc9};

	// USB product id exclusively used with older SwiftRNG device
	const int c_usb_product_id {0x8110};

	// USB product id exclusively used with newer CDC SwiftRNG device
	const int c_usb_cdc_product_id {0x8111};

	// Device specific USB bulk end point number for OUT operation
	const int SWRNG_BULK_EP_OUT {0x01};

	// Device specific USB bulk end point number for IN operation
	const int SWRNG_BULK_EP_IN {0x81};

	// Method id for using SHA-256 for post processing
	const int c_sha256_pp_method_id {0};

	// Method id for using no post processing
	const int c_emb_corr_method_none_id {0};

	// Method id for using no embedded correction
	const int c_emb_corr_method_linear_id {1};

	// Method id for using XORSHIFT64 for post processing
	const int c_xorshift64_pp_method_id {1};

	// Method id for using SHA-512 for post processing
	const int c_sha512_pp_method_id {2};

	// Constants used as temporary storages when generating random byte content
	const int c_word_size_bytes {4};
	const int c_num_chunks {500};
	const int c_min_input_num_words {8};
	const int c_out_num_words {8};
	const int c_rnd_out_buff_size {c_num_chunks * c_out_num_words * c_word_size_bytes};
	const int c_rnd_in_buff_size {c_num_chunks * c_min_input_num_words * c_word_size_bytes};

	// There could be many CDC COM devices connected, limit the amount of devices to search
	const int c_max_cdc_com_ports {80};

	// Sometimes read operations from device may timeout, limit the number of times to read before giving up
	const int c_usb_read_max_retry_count {15};

	// Expected timeout interval in seconds for device read operations
	const int c_usb_read_timeout_secs {2};

	// Expected timeout interval for USB bulk read operations
	const int c_usb_bulk_read_timeout_mlsecs {100};

	// Max amount of bytes to limit by the API when downloading random bytes from device
	const int c_max_request_size_bytes {100000L};

	// A structure used for generating SHA-256 hash
	struct {
		uint32_t a, b, c, d, e, f, g, h;
		uint32_t h0, h1, h2, h3, h4, h5, h6, h7;
		uint32_t tmp1, tmp2;
		uint32_t w[64];
		uint32_t blockSerialNumber;
	} m_sha256_ctxt;

	// A structure used for generating SHA-512 hash
	struct {
		uint64_t a,b,c,d,e,f,g,h;
		uint64_t h0,h1,h2,h3,h4,h5,h6,h7;
		uint64_t tmp1, tmp2;
		uint64_t w[80];
	} m_sha512_ctxt;

	// Repetition Count Test data
	struct {
		uint32_t maxRepetitions;
		uint32_t curRepetitions;
		uint8_t lastSample;
		uint8_t statusByte;
		uint8_t signature;
		bool isInitialized;
		uint16_t failureCount;
	} m_rct;

	// Adaptive Proportion Test data
	struct {
		uint16_t windowSize;
		uint16_t cutoffValue;
		uint16_t curRepetitions;
		uint16_t curSamples;
		uint8_t statusByte;
		uint8_t signature;
		bool isInitialized;
		uint8_t firstSample;
		uint16_t cycleFailures;
	} m_apt;

	// True if the class data have been initialized
	bool m_is_initialized = false;

	// These pointers are used with libusb
	libusb_device **m_libusb_devs {nullptr};
	libusb_device *m_libusb_dev {nullptr};
	libusb_device_handle *m_libusb_devh {nullptr};
	libusb_context *m_libusb_luctx {nullptr};

	// True if libusb has been initialized
	bool m_libusb_initialized {false};

	// True if device successfully open
	bool m_device_open = false;

	// Random output buffer
	char *m_buff_rnd_out;

	// Current index for the m_buff_rnd_out buffer
	int m_cur_rng_out_idx {c_rnd_out_buff_size};

	// A buffer for collecting data received from USB device
	unsigned char *m_bulk_in_buffer;

	// A buffer for sending commands to USB device
	unsigned char m_bulk_out_buffer[16];

	// Random input buffer used with hashing or post processing
	char *m_buff_rnd_in;

	// The source of one block of data to hash with SHA-256
	uint32_t *m_src_to_hash_32;

	// The size of one block of data words used with SHA-256 hashing
	const uint8_t m_max_data_block_size_words {16};

	// Initialization data for SHA-256 (FIPS PUB 180-4)
	const uint32_t *c_sha256_k {nullptr};
	const uint32_t *c_sha256_test_seq_1 {nullptr};
	const uint32_t *c_sha256_expt_hash_seq_1 {nullptr};

	// The source of one block of data to hash with SHA-512
	uint64_t *m_src_to_hash_64;

	// Initialization data for SHA-512 (FIPS PUB 180-4)
	const uint64_t *c_sha512_k {nullptr};
	const uint64_t *c_sha512_expt_hash_seq1 {nullptr};

	// How many statistical test failures allowed per data block (16000 random bytes)
	uint8_t m_num_failures_threshold {4};

	// Max number of repetition count test failures encountered per data block (16000 random bytes)
	uint16_t m_max_rct_failures_per_block {0};

	// Max number of adaptive proportion test failures encountered per data block (16000 random bytes)
	uint16_t m_max_apt_failures_per_block {0};

	// A storage for maintaining statistics such as how many bytes retrieved from USB device, transfer speed, e.t.c
	DeviceStatistics m_device_stats;

	// A storage for holding the last error message. Many operations, when failed, will store the error message in this buffer.
	// The error text message can be retrieved with `get_last_error_message()` method.
	ostringstream m_last_error_log_oss;

	// Number of bytes to allocate for `m_last_error_log_char`;
	const int m_max_last_error_log_char {2565};

	// C style char message same as `m_last_error_log_oss`
	char *m_last_error_log_char;

	// How many bytes to allocate for `m_last_error_buff`
	const int c_last_error_buff_size {256};

	// When set to true, each error message generated will automatically be printed on error output stream.
	bool m_print_error_messages {false};

	// A storage used for retrieving the version of the current device
	DeviceVersion m_cur_device_version;

	// A variable for storing both device major and minor versions, for example: 2.0
	double m_device_version_double = 0;

	// True if post processing is enabled for the final random output stream.
	// By default the post processing is enabled with the exception of devices with versions 2.0 or newer.
	bool m_post_processing_enabled {true};

	// Post processing method used with the device.
	// 0 - SHA256 method, 1 - xorshift64, 2 - SHA512 method.
	// This doesn't have effect if post processing is disabled by `m_post_processing_enabled`
	int m_post_processing_method_id {c_sha256_pp_method_id};

	// True if statistical tests (APT, RCT) are enabled for the random data stream retrieved from device.
	// By default all statistical tests are enabled.
	bool m_stat_tests_enabled {true};

	// Device internal correction used. Currently only SwiftRNG Z implements an internal correction.
	// 0 - none, 1 - Linear correction (P. Lacharme).
	int m_dev_embedded_corr_method_id {c_emb_corr_method_none_id};

	// Use specific implementation for Windows or other platforms
#ifdef _WIN32
	// Use specific implementation for the USB COM port when using Windows
	USBComPort *m_usb_com_port;
#else
	// Use specific implementation for the USB ACM device when using Linux, macOS or FreeBSD
	USBSerialDevice *m_usb_serial_device {nullptr};
#endif

	static const string c_dev_not_open_msg;
	static const string c_freq_table_invalid_msg;
	static const string c_cannot_disable_pp_msg;
	static const string c_pp_op_not_supported_msg;
	static const string c_diag_op_not_supported_msg;
	static const string c_pp_method_not_supported_msg;
	static const string c_cannot_get_freq_table_for_device_msg;
	static const string c_too_many_devices_msg;
	static const string c_cannot_read_device_descriptor_msg;
	static const string c_libusb_init_failure_msg;

};


} /* namespace swiftrng */

#endif /* SWIFTRNGAPI_H_ */
