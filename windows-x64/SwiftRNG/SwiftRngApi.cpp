/**
 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the API for interacting with the SwiftRNG device.

 */

/**
 *    @file SwiftRngApi.h
 *    @date 05/01/2024
 *    @Author: Andrian Belinski
 *    @version 1.2
 *
 *    @brief Implements the API for interacting with the SwiftRNG device.
 */
#include <SwiftRngApi.h>

using namespace std;

namespace swiftrng {

SwiftRngApi::SwiftRngApi() {
	initialize();
}

void SwiftRngApi::initialize() {

	if (is_context_initialized() == true) {
		// This should never happen, just a sanity check
		return;
	}

	std::memset(&m_device_stats, 0, sizeof(DeviceStatistics));
	std::memset(&m_cur_device_version, 0, sizeof(DeviceVersion));

	c_sha256_k = new (nothrow) uint32_t [64] {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
		0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
		0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
		0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
		0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
		0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
		0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
		0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
		0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};

	if (c_sha256_k == nullptr) {
		return;
	}

	c_sha256_test_seq_1 = new (nothrow) uint32_t [11] {
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0x428a2f98,
		0x71374491, 0xb5c0fbcf
	};

	if (c_sha256_test_seq_1 == nullptr) {
		return;
	}


	c_sha256_expt_hash_seq_1 = new (nothrow) uint32_t [8] {
		0x114c3052, 0x76410592, 0xc024566b,
		0xa492b1a2, 0xb0559389, 0xb7c41156, 0x2ec8d6c3, 0x3dcb02dd
	};

	if (c_sha256_expt_hash_seq_1 == nullptr) {
		return;
	}


	c_sha512_k = new (nothrow) uint64_t [80] {
		0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
		0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
		0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
		0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
		0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
		0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
		0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
		0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
		0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
		0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
		0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
		0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
		0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
		0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
		0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
		0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
		0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
		0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
		0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
		0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
	};

	if (c_sha512_k == nullptr) {
		return;
	}

	c_sha512_expt_hash_seq1 = new (nothrow) uint64_t [8] {
		0x6cbce8f347e8d1b3, 0xd3517b27fdc4ee1c, 0x71d8406ab54e2335,
		0xf3a39732fa0009d2, 0x2193c41677d18504, 0xe90b4c1138c32e7c, 0xc1aa7500597ba99c, 0xacd525ef2c44e9dc
	};

	if (c_sha512_expt_hash_seq1 == nullptr) {
		return;
	}

	m_buff_rnd_out = new (nothrow) char[c_rnd_out_buff_size];
	if (m_buff_rnd_out == nullptr) {
		return;
	}

	m_bulk_in_buffer = new (nothrow) unsigned char [c_rnd_in_buff_size + 1];
	if (m_bulk_in_buffer == nullptr) {
		return;
	}

	m_buff_rnd_in = new (nothrow) char [c_rnd_in_buff_size + 1];
	if (m_buff_rnd_in == nullptr) {
		return;
	}

	m_src_to_hash_32 = new (nothrow) uint32_t [c_min_input_num_words + 1];
	if (m_src_to_hash_32 == nullptr) {
		return;
	}

	m_src_to_hash_64 = new (nothrow) uint64_t[c_min_input_num_words];
	if (m_src_to_hash_64 == nullptr) {
		return;
	}

	m_last_error_log_char = new (nothrow) char [c_max_last_error_log_size];
	if (m_last_error_log_char == nullptr) {
		return;
	}

	m_is_initialized = true;
	clear_last_error_msg();

}

void SwiftRngApi::close_USB_lib() {

	if (m_libusb_devh) {
		libusb_release_interface(m_libusb_devh, 0); //release the claimed interface
		libusb_close(m_libusb_devh);
		m_libusb_devh = nullptr;
	}
	if (m_libusb_devs) {
		libusb_free_device_list(m_libusb_devs, 1);
		m_libusb_devs = nullptr;
	}
	if (m_libusb_initialized == true) {
		libusb_exit(m_libusb_luctx);
		m_libusb_luctx = nullptr;
		m_libusb_initialized = false;
	}
}

/**
 * Print and/or save error message
 * @param err_msg - pointer to error message
 */
void SwiftRngApi::swrng_printErrorMessage(const string &err_msg) {
	if (m_print_error_messages) {
		cerr << err_msg << endl;
	}
	clear_last_error_msg();
	m_last_error_log_oss << err_msg;

	auto err_msg_size = (int) err_msg.size();
	if (err_msg_size >= c_max_last_error_log_size) {
		// Truncate the message to fit the char storage
		err_msg_size = c_max_last_error_log_size - 1;
	}
	memcpy(m_last_error_log_char, err_msg.c_str(), err_msg_size);
	m_last_error_log_char[err_msg_size] = '\0';
}

int SwiftRngApi::open(int devNum) {
	int ret;
	int actualDeviceNum = devNum;

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_usb_serial_device != nullptr) {
		m_usb_serial_device->disconnect();
	}

	close_USB_lib();
	clear_last_error_msg();
	m_device_open = false;


#ifdef _WIN32
	if (m_usb_serial_device == nullptr) {
		m_usb_serial_device = new (nothrow) USBComPort;
		m_usb_serial_device->initialize();
	}
#else
	if (m_usb_serial_device == nullptr) {
		m_usb_serial_device = new (nothrow) USBSerialDevice;
		m_usb_serial_device->initialize();
	}

#endif


	rct_initialize();
	apt_initialize();

	sha256_initializeSerialNumber((uint32_t)m_device_stats.beginTime);
	if (sha256_selfTest() != SWRNG_SUCCESS) {
		swrng_printErrorMessage("SHA256 post processing logic failed the self-test");
		return -EPERM;
	}

	if (sha512_selfTest() != SWRNG_SUCCESS) {
		swrng_printErrorMessage("SHA512 post processing logic failed the self-test");
		return -EPERM;
	}

	if (xorshift64_selfTest() != SWRNG_SUCCESS) {
		swrng_printErrorMessage("Xorshift64 post processing logic failed the self-test");
		return -EPERM;
	}

#if defined(_WIN32)
	int portsConnected;
	int ports[c_max_cdc_com_ports];
	// Retrieve devices connected as USB CDC in Windows
	m_usb_serial_device->get_connected_ports(ports, c_max_cdc_com_ports, &portsConnected, (WCHAR*)c_hardware_id.c_str(), (WCHAR*)L"SWRNG");
	if (portsConnected > devNum) {
		WCHAR portName[80];
		m_usb_serial_device->toPortName(ports[devNum], portName, 80);
		bool comPortStatus = m_usb_serial_device->connect(portName);
		if (!comPortStatus) {
			swrng_printErrorMessage(m_usb_serial_device->get_error_log());
			return -1;
		}
		else {
			return swrng_handleDeviceVersion();
		}
	}
	else {
		actualDeviceNum -= portsConnected;
	}
#else
	// Retrieve devices connected as USB CDC in Linux/macOS
	m_usb_serial_device->scan_available_devices();
	int portsConnected = m_usb_serial_device->get_device_count();
	if (portsConnected > devNum) {
		char devicePath[128];
		m_usb_serial_device->retrieve_device_path(devicePath, devNum);
		bool comPortStatus = m_usb_serial_device->connect(devicePath);
		if (!comPortStatus) {
			swrng_printErrorMessage(m_usb_serial_device->get_error_log().c_str());
			return -1;
		}
		else {
			return swrng_handleDeviceVersion();
		}
	}
	else {
		actualDeviceNum -= portsConnected;
	}
#endif

	if (m_libusb_initialized == false) {
		int r = libusb_init(&m_libusb_luctx);
		if (r < 0) {
			close_USB_lib();
			swrng_printErrorMessage(c_libusb_init_failure_msg);
			return r;
		}
		m_libusb_initialized = true;
	}

	int curFoundDevNum = -1;
	ssize_t cnt = libusb_get_device_list(m_libusb_luctx, &m_libusb_devs);
	if (cnt < 0) {
		close_USB_lib();
		return (int)cnt;
	}
	int i = 0;
	while ((m_libusb_dev = m_libusb_devs[i++]) != nullptr) {
		struct libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(m_libusb_dev, &desc);
		if (ret < 0) {
			close_USB_lib();
			swrng_printErrorMessage(c_cannot_read_device_descriptor_msg);
			return ret;
		}
		uint16_t idVendorCur = desc.idVendor;
		uint16_t idProductCur = desc.idProduct;
		if (idVendorCur == c_usb_vendor_id && idProductCur == c_usb_product_id) {
			if (++curFoundDevNum == actualDeviceNum) {
				ret = libusb_open(m_libusb_dev, &m_libusb_devh);
				switch (ret) {
				case LIBUSB_ERROR_NO_MEM:
					close_USB_lib();
					swrng_printErrorMessage("Memory allocation failure");
					return ret;
				case LIBUSB_ERROR_ACCESS:
					close_USB_lib();
					swrng_printErrorMessage("User has insufficient permissions");
					return ret;
				case LIBUSB_ERROR_NO_DEVICE:
					close_USB_lib();
					swrng_printErrorMessage("Device has been disconnected");
					return ret;
				}
				if (ret < 0) {
					close_USB_lib();
					swrng_printErrorMessage("Failed to open USB device");
					return ret;
				}

				if (libusb_kernel_driver_active(m_libusb_devh, 0) == 1) { //find out if kernel driver is attached
					close_USB_lib();
					swrng_printErrorMessage("Device is already in use by kernel driver");
					return -1;
				}

				ret = libusb_claim_interface(m_libusb_devh, 0);
				if (ret < 0) {
					close_USB_lib();
					swrng_printErrorMessage("Cannot claim the USB interface");
					return ret;
				}

				return swrng_handleDeviceVersion();
			}
		}
	}
	close_USB_lib();
	swrng_printErrorMessage("Could not find any SwiftRNG device");
	return -1;

}

/**
 * Close device if open
 *
 * @return int - 0 when processed successfully
 */
int SwiftRngApi::close() {

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_usb_serial_device != nullptr) {
		m_usb_serial_device->disconnect();
		delete m_usb_serial_device;
		m_usb_serial_device = nullptr;
	}
	close_USB_lib();
	m_device_open = false;
	return SWRNG_SUCCESS;
}

void SwiftRngApi::swrng_contextReset() {
	if (m_usb_serial_device != nullptr) {
		m_usb_serial_device->disconnect();
	}
	close_USB_lib();
	clear_last_error_msg();
	m_device_open = false;
}

/**
* Check if device is open
*
* @return int - 0 when device is open
*/
int SwiftRngApi::is_open() const {

	if (is_context_initialized() == false) {
		return -1;
	}

	return m_device_open;
}

/**
 * A function to request new entropy bytes when running out of entropy in the local buffer
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int SwiftRngApi::swrng_getEntropyBytes() {
	int status;
	if (m_cur_rng_out_idx >= c_rnd_out_buff_size) {
		status = swrng_rcv_rnd_bytes();
	} else {
		status = SWRNG_SUCCESS;
	}
	return status;
}

/**
 * A function to fill the buffer with new entropy bytes
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int SwiftRngApi::swrng_rcv_rnd_bytes() {
	int retval;
	uint32_t *dst32, *src32;
	uint64_t *dst64, *src64;

	if (!m_device_open) {
		return -EPERM;
	}

	m_bulk_out_buffer[0] = 'x';

	retval = swrng_snd_rcv_usb_data((char *)m_bulk_out_buffer, 1, m_buff_rnd_in,
			c_rnd_in_buff_size, c_usb_read_timeout_secs);
	if (retval == SWRNG_SUCCESS) {
		if (m_stat_tests_enabled == true) {
			rct_restart();
			apt_restart();
			test_samples();
		}
		if (m_post_processing_enabled == true) {
			if (m_post_processing_method_id == c_sha256_pp_method_id) {
				dst32 = (uint32_t *)m_buff_rnd_out;
				src32 = (uint32_t *)m_buff_rnd_in;
				for (int i = 0; i < c_rnd_in_buff_size / c_word_size_bytes; i
						+= c_min_input_num_words) {
					for (int j = 0; j < c_min_input_num_words; j++) {
						m_src_to_hash_32[j] = src32[i + j];
					}
					sha256_stampSerialNumber(m_src_to_hash_32);
					sha256_generateHash(m_src_to_hash_32, (int16_t)(c_min_input_num_words + 1), dst32);
					dst32 += c_out_num_words;
				}
			} else if (m_post_processing_method_id == c_sha512_pp_method_id) {
				dst64 = (uint64_t *)m_buff_rnd_out;
				src64 = (uint64_t *)m_buff_rnd_in;
				for (int i = 0; i < c_rnd_in_buff_size / (c_word_size_bytes * 2); i
						+= c_min_input_num_words) {
					for (int j = 0; j < c_min_input_num_words; j++) {
						m_src_to_hash_64[j] = src64[i + j];
					}
					sha512_generateHash(m_src_to_hash_64, (int16_t)c_min_input_num_words, dst64);
					dst64 += c_out_num_words;
				}
			} else if (m_post_processing_method_id == c_xorshift64_pp_method_id) {
				memcpy(m_buff_rnd_out, m_buff_rnd_in, c_rnd_out_buff_size);
				xorshift64_postProcess((uint8_t *)m_buff_rnd_out, c_rnd_out_buff_size);
			} else {
				swrng_printErrorMessage(c_pp_op_not_supported_msg);
				return -1;
			}
		} else {
			memcpy(m_buff_rnd_out, m_buff_rnd_in, c_rnd_out_buff_size);
		}
		m_cur_rng_out_idx = 0;
		if (m_rct.statusByte != SWRNG_SUCCESS) {
			swrng_printErrorMessage("Repetition Count Test failure");
			retval = -EPERM;
		} else if (m_apt.statusByte != SWRNG_SUCCESS) {
			swrng_printErrorMessage("Adaptive Proportion Test failure");
			retval = -EPERM;
		}
	}

	return retval;
}

/**
 * A function for testing a block of random bytes using 'repetition count'
 * and 'adaptive proportion' tests
 *
 */
void SwiftRngApi::test_samples() {
	uint8_t value;
	for (int i = 0; i < c_rnd_out_buff_size; i++) {
		value = m_buff_rnd_in[i];

		//
		// Run 'repetition count' test
		//
		if (!m_rct.isInitialized) {
			m_rct.isInitialized = true;
			m_rct.lastSample = value;
		} else {
			if (m_rct.lastSample == value) {
				m_rct.curRepetitions++;
				if (m_rct.curRepetitions >= m_rct.maxRepetitions) {
					m_rct.curRepetitions = 1;
					if (++m_rct.failureCount > m_num_failures_threshold) {
						if (m_rct.statusByte == 0) {
							m_rct.statusByte = m_rct.signature;
						}
					}

					if (m_rct.failureCount > m_max_rct_failures_per_block) {
						// Record the maximum failures per block for reporting
						m_max_rct_failures_per_block = m_rct.failureCount;
					}

					#ifdef inDebugMode
					if (m_rct.failureCount >= 1) {
						fprintf(stderr, "rct.failureCount: %d value: %d\n", m_rct.failureCount, value);
					}
					#endif
				}

			} else {
				m_rct.lastSample = value;
				m_rct.curRepetitions = 1;
			}
		}

		//
		// Run 'adaptive proportion' test
		//
		if (!m_apt.isInitialized) {
			m_apt.isInitialized = true;
			m_apt.firstSample = value;
			m_apt.curRepetitions = 0;
			m_apt.curSamples = 0;
		} else {
			if (++m_apt.curSamples >= m_apt.windowSize) {
				m_apt.isInitialized = false;
				if (m_apt.curRepetitions > m_apt.cutoffValue) {
					// Check to see if we have reached the failure threshold
					if (++m_apt.cycleFailures > m_num_failures_threshold) {
						if (m_apt.statusByte == 0) {
							m_apt.statusByte = m_apt.signature;
						}
					}
					if (m_apt.cycleFailures > m_max_apt_failures_per_block) {
						// Record the maximum failures per block for reporting
						m_max_apt_failures_per_block = m_apt.cycleFailures;
					}

					#ifdef inDebugMode
					if (m_apt.cycleFailures >= 1) {
						fprintf(stderr, "ctxt->apt.cycleFailures: %d value: %d\n", m_apt.cycleFailures, value);
                    }
					#endif
				}

			} else {
				if (m_apt.firstSample == value) {
					++m_apt.curRepetitions;
				}
			}
		}


	}

}

/**
* This function is an enhanced version of 'swrngGetEntropy'.
* Use it to retrieve more than 100000 random bytes in one call
*
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive
* @return 0 - successful operation, otherwise the error code
*
*/
int SwiftRngApi::get_entropy_ex(unsigned char *buffer, long length) {
	int retval = SWRNG_SUCCESS;
	unsigned char *dst;
	long total_byte_blocks;
	long leftover_bytes;

	if (length <= 0) {
		retval = -EPERM;
	} else {
		total_byte_blocks = length / c_max_request_size_bytes;
		leftover_bytes = length % c_max_request_size_bytes;
		dst = buffer;
		for (long l = 0; l < total_byte_blocks; l++) {
			retval = get_entropy(dst, c_max_request_size_bytes);
			if (retval != SWRNG_SUCCESS) {
				break;
			}
			dst += c_max_request_size_bytes;
		}
		retval = get_entropy(dst, leftover_bytes);
	}

	return retval;
}

/**
* A function to retrieve RAW random bytes from a noise source.
* No data alteration, verification or quality tests will be performed when calling this function.
* It will retrieve 16,000 raw bytes of the sampled random data from one of the noise sources.
* It can be used for inspecting the quality of the noise sources and data acquisition components.
*
* @param noise_source_raw_data - a pointer to a structure for holding 16,000 of raw data
* @param int noise_source_num - noise source number (0 - first noise source, 1 - second one)
* @return 0 - successful operation, otherwise the error code
*
*/
int SwiftRngApi::get_raw_data_block(NoiseSourceRawData *noise_source_raw_data, int noise_source_num) {

	char device_command;

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	if (noise_source_num < 0 || noise_source_num > 1) {
		swrng_printErrorMessage("Noise source number must be 0 or 1");
		return -1;
	}

	if (noise_source_num == 0 ) {
		device_command = '<';
	} else {
		device_command = '>';
	}

	int resp = swrng_snd_rcv_usb_data(&device_command, 1, noise_source_raw_data->value,
			sizeof(NoiseSourceRawData) - 1, c_usb_read_timeout_secs);
	// Replace status byte with \0 for ASCIIZ
	noise_source_raw_data->value[sizeof(NoiseSourceRawData) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage("Could not retrieve RAW data from a noise source");
	}
	return resp;
}

/**
* A function for retrieving frequency tables of the random numbers generated from random sources.
* There is one frequency table per noise source. Each table consist of 256 integers (16 bit) that represent
* frequencies for the random numbers generated between 0 and 255. These tables are used for checking the quality of the
* noise sources and for detecting partial or total failures associated with those sources.
*
* SwiftRNG has two frequency tables. You will need to call this method twice - one call per frequency table.
*
* @param frequency_tables - a pointer to the frequency tables structure
* @return 0 - successful operation, otherwise the error code
*
*/
int SwiftRngApi::get_frequency_tables(FrequencyTables *frequency_tables) {
	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	if (m_device_version_double < 1.2) {
		swrng_printErrorMessage(c_cannot_get_freq_table_for_device_msg);
		return -1;
	}

	if (frequency_tables == nullptr) {
		swrng_printErrorMessage(c_freq_table_invalid_msg);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data("f", 1, (char *)frequency_tables,
			sizeof(FrequencyTables) - 2, c_usb_read_timeout_secs);
	if (resp != 0) {
		swrng_printErrorMessage("Could not retrieve device frequency tables");
	}
	return resp;
}

/**
 * Retrieve a complete list of SwiftRNG devices currently plugged into USB ports
 *
 * @param dev_info_list - pointer to the structure for holding SwiftRNG
 * @return int - value 0 when retrieved successfully
 *
 */
int SwiftRngApi::get_device_list(DeviceInfoList *dev_info_list) {

	libusb_device * *devs;
	libusb_device * dev;

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == true) {
		swrng_printErrorMessage("Cannot invoke listDevices when there is a USB session in progress");
		return -1;
	}

	if (m_libusb_initialized == false) {
		int r = libusb_init(&m_libusb_luctx);
		if (r < 0) {
			swrng_printErrorMessage(c_libusb_init_failure_msg);
			return r;
		}
		m_libusb_initialized = true;
	}

	memset((void*) dev_info_list, 0, sizeof(DeviceInfoList));

	int curFoundDevNum = 0;

#if defined(_WIN32)
	USBComPort usbComPort;
	int portsConnected;
	int ports[c_max_cdc_com_ports];
	// Add devices connected as USB CDC in Windows
	usbComPort.get_connected_ports(ports, c_max_cdc_com_ports, &portsConnected, (WCHAR*)c_hardware_id.c_str(), (WCHAR*)L"SWRNG");
	while (portsConnected-- > 0) {
		swrng_updateDevInfoList(dev_info_list, &curFoundDevNum);
	}
#else
	USBSerialDevice usbSerialDevice;
	// Add devices connected as USB CDC in Linux/macOS
	usbSerialDevice.scan_available_devices();
	int portsConnected = usbSerialDevice.get_device_count();
	while (portsConnected-- > 0) {
		swrng_updateDevInfoList(dev_info_list, &curFoundDevNum);
	}
#endif

	ssize_t cnt = libusb_get_device_list(nullptr, &devs);
	if (cnt < 0) {
		close_USB_lib();
		return (int)cnt;
	}
	int i = -1;
	while ((dev = devs[++i]) != nullptr) {
		if (i > 127) {
			libusb_free_device_list(devs, 1);
			close_USB_lib();
			// This should never happen, cannot have more than 127 USB devices
			swrng_printErrorMessage(c_too_many_devices_msg);
			return -1;
		}
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			libusb_free_device_list(devs, 1);
			close_USB_lib();
			swrng_printErrorMessage(c_cannot_read_device_descriptor_msg);
			return r;
		}
		uint16_t idVendorCur = desc.idVendor;
		uint16_t idProductCur = desc.idProduct;
		if (idVendorCur == c_usb_vendor_id && idProductCur == c_usb_product_id) {
			swrng_updateDevInfoList(dev_info_list, &curFoundDevNum);
		}
	}
	libusb_free_device_list(devs, 1);
	close_USB_lib();
	return 0;
}

void SwiftRngApi::swrng_updateDevInfoList(DeviceInfoList* dev_info_list, int *curt_found_dev_num) const {
	dev_info_list->devInfoList[*curt_found_dev_num].devNum = *curt_found_dev_num;
	SwiftRngApi api;
	if (api.is_context_initialized() == false) {
		return;
	}
	if (api.open(*curt_found_dev_num) == 0) {
		api.get_model(&dev_info_list->devInfoList[*curt_found_dev_num].dm);
		api.get_version(&dev_info_list->devInfoList[*curt_found_dev_num].dv);
		api.get_serial_number(&dev_info_list->devInfoList[*curt_found_dev_num].sn);
	}
	api.close();
	dev_info_list->numDevs++;
	(*curt_found_dev_num)++;
}

/**
 * Retrieve SwiftRNG device serial number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param serial_number - pointer to a structure for holding SwiftRNG device S/N
 * @return int - 0 when serial number retrieved successfully
 *
 */
int SwiftRngApi::get_serial_number(DeviceSerialNumber *serial_number) {

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data("s", 1, serial_number->value, sizeof(DeviceSerialNumber) - 1, c_usb_read_timeout_secs);
	// Replace status byte with \0 for ASCIIZ
	serial_number->value[sizeof(DeviceSerialNumber) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage("Could not retrieve device serial number");
	}
	return resp;
}

/**
* Set device power profile
*
* @param power_profile_number - new power profile number (0 through 9)
* @return int - 0 when power profile was set successfully
*
*/
int SwiftRngApi::set_power_profile(int power_profile_number) {

	char ppn[1];

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	ppn[0] = '0' + (char)power_profile_number;

	int resp = swrng_snd_rcv_usb_data(ppn, 1, ppn, 0, c_usb_read_timeout_secs);
	if (resp != 0) {
		swrng_printErrorMessage("Could not set power profile");
	}
	return resp;
}

/**
* Disable post processing of raw random data (applies only to devices with versions 1.2 and up)
* Post processing is initially enabled for all devices.
*
* To disable post processing, call this function immediately after device is successfully open.
*
* @return int - 0 when post processing was successfully disabled, otherwise the error code
*
*/
int SwiftRngApi::disable_post_processing() {

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	if (m_device_version_double < 1.2) {
		swrng_printErrorMessage(c_cannot_disable_pp_msg);
		return -1;
	}

	m_post_processing_enabled = false;
	return SWRNG_SUCCESS;
}

/**
* Check to see if raw data post processing is enabled for device.
*
* @param post_processing_status - pointer to store the post processing status (1 - enabled or 0 - otherwise)
* @return int - 0 when post processing flag successfully retrieved, otherwise the error code
*
*/
int SwiftRngApi::get_post_processing_status(int *post_processing_status) {
	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	*post_processing_status = m_post_processing_enabled;
	return SWRNG_SUCCESS;
}

/**
* Retrieve post processing method
*
* @param post_processing_method_id - pointer to store the post processing method id
* @return int - 0 when post processing method successfully retrieved, otherwise the error code
*
*/
int SwiftRngApi::get_post_processing_method(int *post_processing_method_id) {
	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}
	*post_processing_method_id = m_post_processing_method_id;
	return SWRNG_SUCCESS;
}

/**
* Retrieve device embedded correction method
*
* @param dev_emb_corr_method_id - pointer to store the device built-in correction method id:
* 			0 - none, 1 - Linear correction (P. Lacharme)
* @return int - 0 embedded correction method successfully retrieved, otherwise the error code
*
*/
int SwiftRngApi::get_embedded_correction_method(int *dev_emb_corr_method_id) {
	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}
	*dev_emb_corr_method_id = m_dev_embedded_corr_method_id;
	return SWRNG_SUCCESS;
}

/**
* Enable post processing method.
*
* @param post_processing_method_id - 0 for SHA256 (default), 1 - xorshift64 (devices with versions 1.2 and up), 2 - for SHA512
*
* @return int - 0 when post processing successfully enabled, otherwise the error code
*
*/
int SwiftRngApi::enable_post_processing(int post_processing_method_id) {

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	switch(post_processing_method_id) {
	case 0:
		m_post_processing_method_id = c_sha256_pp_method_id;
		break;
	case 1:
		if (m_device_version_double < 1.2) {
			swrng_printErrorMessage(c_pp_op_not_supported_msg);
			return -1;
		}
		m_post_processing_method_id = c_xorshift64_pp_method_id;
		break;
	case 2:
		m_post_processing_method_id = c_sha512_pp_method_id;
		break;
	default:
		swrng_printErrorMessage(c_pp_method_not_supported_msg);
		return -1;
	}
	m_post_processing_enabled = true;
	return SWRNG_SUCCESS;
}

/**
* Enable statistical tests for raw random data stream.
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*
*/
int SwiftRngApi::enable_statistical_tests() {

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	m_stat_tests_enabled = true;
	return SWRNG_SUCCESS;
}

/**
 * Generate and retrieve device statistics
 * @return a pointer to DeviceStatistics structure or nullptr if the call failed
 */
DeviceStatistics* SwiftRngApi::generate_device_statistics() {

	if (is_context_initialized() == false) {
		return nullptr;
	}

	time(&m_device_stats.endTime); // Stop performance timer
	// Calculate performance
	m_device_stats.totalTime = m_device_stats.endTime - m_device_stats.beginTime;
	if (m_device_stats.totalTime == 0) {
		m_device_stats.totalTime = 1;
	}
	m_device_stats.downloadSpeedKBsec = (int) (m_device_stats.numGenBytes / (int64_t) 1024 / m_device_stats.totalTime);
	return &m_device_stats;
}

/**
* Retrieve the last error message.
* The caller should make a copy of the error message returned immediately after calling this function.
* @return - pointer to the error message
*/
const char* SwiftRngApi::get_last_error_message() {

	if (is_context_initialized() == false) {
		clear_last_error_msg();
		swrng_printErrorMessage("Context not initialized");
	}

	return m_last_error_log_char;
}

/**
* Call this function to enable printing error messages to the error stream
*/
void SwiftRngApi::enable_printing_error_messages() {
	if (is_context_initialized() == false) {
		return;
	}
	m_print_error_messages = true;
}

/**
* Retrieve Max number of adaptive proportion test failures encountered per data block
*
* @param max_apt_failures_per_block pointer to the max number of failures
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*/
int SwiftRngApi::get_max_apt_failures_per_block(uint16_t *max_apt_failures_per_block) const {
	if (is_context_initialized() == false) {
		return -1;
	}
	*max_apt_failures_per_block = m_max_apt_failures_per_block;
	return 0;
}

/**
* Retrieve Max number of repetition count test failures encountered per data block
*
* @param max_rct_failures_per_block pointer to the max number of failures
*
* @return int - 0 when statistical tests successfully enabled, otherwise the error code
*/
int SwiftRngApi::get_max_rct_failures_per_block(uint16_t *max_rct_failures_per_block) const {
	if (is_context_initialized() == false) {
		return -1;
	}
	*max_rct_failures_per_block = m_max_rct_failures_per_block;
	return 0;
}

/**
* Check to see if statistical tests are enabled on raw data stream for device.
*
* @param statistical_tests_enabled_status - pointer to store statistical tests status (1 - enabled or 0 - otherwise)
* @return int - 0 when statistical tests flag successfully retrieved, otherwise the error code
*
*/
int SwiftRngApi::get_statistical_tests_status(int *statistical_tests_enabled_status) {
	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	*statistical_tests_enabled_status = m_stat_tests_enabled;
	return SWRNG_SUCCESS;
}

/**
* Disable statistical tests for raw random data stream.
* Statistical tests are initially enabled for all devices.
*
* To disable statistical tests, call this function immediately after device is successfully open.
*
* @return int - 0 when statistical tests successfully disabled, otherwise the error code
*
*/
int SwiftRngApi::disable_statistical_tests() {

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	m_stat_tests_enabled = false;
	return SWRNG_SUCCESS;

}

/**
* Run device diagnostics
*
* @return int - 0 when diagnostics ran successfully, otherwise the error code
*
*/
int SwiftRngApi::run_device_diagnostics() {
	char command;

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}


	if (m_device_version_double < 2.0) {
		swrng_printErrorMessage(c_diag_op_not_supported_msg);
		return -1;
	}
	command = 'd';

	int resp = swrng_snd_rcv_usb_data(&command, 1, &command, 0, c_usb_read_timeout_secs);
	if (resp != 0) {
		swrng_printErrorMessage("Device diagnostics failed");
	}
	return resp;
}


/**
 * Reset statistics for the SWRNG device
 *
 */
void SwiftRngApi::reset_statistics() {

	if (is_context_initialized() == false) {
		return;
	}
	time(&m_device_stats.beginTime); // Start performance timer
	m_device_stats.downloadSpeedKBsec = 0;
	m_device_stats.numGenBytes = 0;
	m_device_stats.totalRetries = 0;
	m_device_stats.endTime = 0;
	m_device_stats.totalTime = 0;
}

/**
 * Retrieve SwiftRNG device model number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param DeviceModel* model - pointer to a structure for holding SwiftRNG device model number
 * @return int 0 - when model retrieved successfully
 *
 */
int SwiftRngApi::get_model(DeviceModel *model) {
	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data("m", 1, model->value,	sizeof(DeviceModel) - 1, c_usb_read_timeout_secs);
	// Replace status byte with \0 for ASCIIZ
	model->value[sizeof(DeviceModel) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage("Could not retrieve device model");
	}
	return resp;
}

/**
* Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
* currently connected or when the device is already in use.
*
* @param double* version - pointer to a double for holding SwiftRNG device version
* @return int - 0 when version retrieved successfully
*
*/
int SwiftRngApi::get_version_number(double *version) {
	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	*version = m_device_version_double;
	return SWRNG_SUCCESS;
}

/**
* A function to retrieve up to 100000 random bytes
*
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive, max value is 100000
* @return 0 - successful operation, otherwise the error code
*
*/
int SwiftRngApi::get_entropy(unsigned char *buffer, long length) {
	int retval = SWRNG_SUCCESS;
	long act;
	long total;

	if (is_context_initialized() == false) {
		return -1;
	}

	if (length > c_max_request_size_bytes || length < 0) {
		retval = -EPERM;
	} else if (!m_device_open) {
		retval = -ENODEV;
	} else {
		total = 0;
		do {
			if (m_cur_rng_out_idx >= c_rnd_out_buff_size) {
				retval = swrng_getEntropyBytes();
			}
			if (retval == SWRNG_SUCCESS) {
				act = c_rnd_out_buff_size - m_cur_rng_out_idx;
				if (act > (length - total)) {
					act = (length - total);
				}
				memcpy(buffer + total, m_buff_rnd_out + m_cur_rng_out_idx, act);
				m_cur_rng_out_idx += act;
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

	}
	return retval;
}

/**
 * Clear potential random bytes from the receiver buffer if any
 */
void SwiftRngApi::swrng_clearReceiverBuffer() {
	int transferred;
	int retval;
	for (int i = 0; i < 3; i++) {

		if (m_usb_serial_device->is_connected()) {
			retval = m_usb_serial_device->receive_data(m_bulk_in_buffer, c_rnd_in_buff_size + 1, &transferred);
		}
		else {
			retval = libusb_bulk_transfer(m_libusb_devh, c_bulk_ep_in, m_bulk_in_buffer, c_rnd_in_buff_size + 1, &transferred, c_usb_bulk_read_timeout_mlsecs);
		}

		if (retval || transferred == 0) {
			break;
		}
	}
}

/**
 * Send a SW device command and receive response
 *
 * @param const char *snd -  a pointer to the command
 * @param int size_snd - how many bytes in command
 * @param char *rcv - a pointer to the data receive buffer
 * @param int size_rcv - how many bytes expected to receive
 * @param int op_timeout_secs - device read time out value in seconds
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int SwiftRngApi::swrng_snd_rcv_usb_data(const char *snd, int sizeSnd, char *rcv, int sizeRcv,	int op_timeout_secs) {
	int retry;
	int actualc_cnt;
	int retval = SWRNG_SUCCESS;

	for (retry = 0; retry < c_usb_read_max_retry_count; retry++) {
		if (m_usb_serial_device->is_connected()) {
			retval = m_usb_serial_device->send_command((const unsigned char*)snd, sizeSnd, &actualc_cnt);
		}
		else {
			retval = libusb_bulk_transfer(m_libusb_devh, c_bulk_ep_out, (unsigned char*)snd, sizeSnd, &actualc_cnt, 100);
		}

		if (retval == SWRNG_SUCCESS && actualc_cnt == sizeSnd) {
			retval = swrng_chip_read_data(rcv, sizeRcv + 1, op_timeout_secs);
			if (retval == SWRNG_SUCCESS) {
				if (rcv[sizeRcv] != 0) {
					retval = -EFAULT;
				} else {
					m_device_stats.numGenBytes += sizeRcv;
					break;
				}
			}
		}
		#ifdef inDebugMode
			fprintf(stderr, "It was an error during data communication. Cleaning up the receiving queue and continue.\n");
		#endif
			m_device_stats.totalRetries++;
		swrng_chip_read_data(m_buff_rnd_in, c_rnd_in_buff_size + 1, op_timeout_secs);
	}
	if (retry >= c_usb_read_max_retry_count && retval == SWRNG_SUCCESS) {
		retval = -ETIMEDOUT;
	}
	return retval;
}

/**
 * A function to handle SW device receive command
 * @param char *buff - a pointer to the data receive buffer
 * @param int length - how many bytes expected to receive
 * @param int op_timeout_secs - device read time out value in seconds
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int SwiftRngApi::swrng_chip_read_data(char *buff, int length, int op_timeout_secs) {
	long secs_waited;
	int transferred;
	time_t start;
	time_t end;
	int cnt;
	int retval;

	start = time(nullptr);

	cnt = 0;
	do {
		if (m_usb_serial_device->is_connected()) {
			retval = m_usb_serial_device->receive_data(m_bulk_in_buffer, length, &transferred);
		}
		else {
			retval = libusb_bulk_transfer(m_libusb_devh, c_bulk_ep_in, m_bulk_in_buffer, length, &transferred, c_usb_bulk_read_timeout_mlsecs);
		}

#ifdef inDebugMode
		fprintf(stderr, "chip_read_data retval %d transferred %d, length %d\n", retval, transferred, length);
#endif
		if (retval) {
			return retval;
		}

		if (transferred > length) {
			swrng_printErrorMessage("Received unexpected bytes when processing USB device request");
			return -EFAULT;
		}

		end = time(nullptr);
		secs_waited = end - start;
		if (transferred > 0) {
			for (int i = 0; i < transferred; i++) {
				buff[cnt++] = m_bulk_in_buffer[i];
			}
		}
	} while (cnt < length && secs_waited < op_timeout_secs);

	if (cnt != length) {
#ifdef inDebugMode
		fprintf(stderr, "timeout received, cnt %d\n", cnt);
#endif
		return -ETIMEDOUT;
	}

	return SWRNG_SUCCESS;
}

/**
 * Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param DeviceVersion* version - pointer to a structure for holding SwiftRNG device version
 * @return int - 0 when version retrieved successfully
 *
 */
int SwiftRngApi::get_version(DeviceVersion *version) {

	if (is_context_initialized() == false) {
		return -1;
	}

	if (m_device_open == false) {
		swrng_printErrorMessage(c_dev_not_open_msg);
		return -1;
	}

	int resp = swrng_snd_rcv_usb_data("v", 1, version->value, sizeof(DeviceVersion) - 1, c_usb_read_timeout_secs);
	// Replace status byte with \0 for ASCIIZ
	version->value[sizeof(DeviceVersion) - 1] = '\0';
	if (resp != 0) {
		swrng_printErrorMessage("Could not retrieve device version");
	}
	return resp;
}

int SwiftRngApi::swrng_handleDeviceVersion() {
	int status;

	// Clear the receive buffer
	swrng_clearReceiverBuffer();
	m_device_open = true;

	// Retrieve device version number
	status = get_version(&m_cur_device_version);
	if (status != SWRNG_SUCCESS) {
		close_USB_lib();
		return status;
	}
	m_device_version_double = atof((char*)&m_cur_device_version + 1);

	if (m_device_version_double >= 2.0) {
		// By default, disable post processing for devices with versions 2.0+
		m_post_processing_enabled = false;
		if (m_device_version_double >= 3.0) {
			// SwiftRNG Z devices use built-in 'P. Lacharme' Linear Correction method
			m_dev_embedded_corr_method_id = c_emb_corr_method_linear_id;
		}
	}
	else {
		// Adjust APT and RCT tests to count for BIAS with older devices.
		// All tests are performed now before applying post-processing to comply
		// with NIST SP 800-90B (second draft)
		if (m_device_version_double == 1.1) {
			m_num_failures_threshold = 6;
		}
		else if (m_device_version_double == 1.0) {
			m_num_failures_threshold = 9;
		}
	}

	return SWRNG_SUCCESS;
}


/**
 * Initialize the SHA256 data
 *
 */
void SwiftRngApi::sha256_initialize() {
	// Initialize H0, H1, H2, H3, H4, H5, H6 and H7
	m_sha256_ctxt.h0 = 0x6a09e667;
	m_sha256_ctxt.h1 = 0xbb67ae85;
	m_sha256_ctxt.h2 = 0x3c6ef372;
	m_sha256_ctxt.h3 = 0xa54ff53a;
	m_sha256_ctxt.h4 = 0x510e527f;
	m_sha256_ctxt.h5 = 0x9b05688c;
	m_sha256_ctxt.h6 = 0x1f83d9ab;
	m_sha256_ctxt.h7 = 0x5be0cd19;
}

/**
 * Initialize the SHA512 data
 *
 */
void SwiftRngApi::sha512_initialize() {
	// Initialize H0, H1, H2, H3, H4, H5, H6 and H7
	m_sha512_ctxt.h0 = 0x6a09e667f3bcc908;
	m_sha512_ctxt.h1 = 0xbb67ae8584caa73b;
	m_sha512_ctxt.h2 = 0x3c6ef372fe94f82b;
	m_sha512_ctxt.h3 = 0xa54ff53a5f1d36f1;
	m_sha512_ctxt.h4 = 0x510e527fade682d1;
	m_sha512_ctxt.h5 = 0x9b05688c2b3e6c1f;
	m_sha512_ctxt.h6 = 0x1f83d9abfb41bd6b;
	m_sha512_ctxt.h7 = 0x5be0cd19137e2179;

	for (int i = 0; i < 15; i++) {
		m_sha512_ctxt.w[i] = 0;
	}
}

/**
 * Stamp a new serial number for the input data block into the last word
 *
 * @param void* input_block pointer to the input hashing block
 *
 */
void SwiftRngApi::sha256_stampSerialNumber(uint32_t *input_block) {
	input_block[c_min_input_num_words] = m_sha256_ctxt.blockSerialNumber++;
}

/**
 * Initialize the serial number for hashing
 *
 * @param init_value - a startup random number for generating serial number for hashing
 *
 */
void SwiftRngApi::sha256_initializeSerialNumber(uint32_t init_value) {
	m_sha256_ctxt.blockSerialNumber = init_value;
}

/**
 * Generate SHA512 value.
 *
 * @param uint64_t* src - pointer to an array of 64 bit words used as hash input
 * @param uint64_t* dst - pointer to an array of 8 X 64 bit words used as hash output
 * @param int16_t len - number of 64 bit words available in array pointed by 'src'
 *
 * @return int 0 for successful operation, -1 for invalid parameters
 *
 */
int SwiftRngApi::sha512_generateHash(const uint64_t *src, int16_t len, uint64_t *dst) {

	if (len <= 0 || len > 14) {
		return -1;
	}

	sha512_initialize();

	int i = 0;
	for (; i < len; i++) {
		m_sha512_ctxt.w[i] = src[i];
	}
	m_sha512_ctxt.w[i] = 0x8000000000000000;
	m_sha512_ctxt.w[15] = len * 64;


	sha512_hashCurrentBlock();

	// Save the results
	dst[0] = m_sha512_ctxt.h0;
	dst[1] = m_sha512_ctxt.h1;
	dst[2] = m_sha512_ctxt.h2;
	dst[3] = m_sha512_ctxt.h3;
	dst[4] = m_sha512_ctxt.h4;
	dst[5] = m_sha512_ctxt.h5;
	dst[6] = m_sha512_ctxt.h6;
	dst[7] = m_sha512_ctxt.h7;

	return 0;
}

/**
 * Generate SHA256 value.
 *
 * @param uint32_t* src - pointer to an array of 32 bit words used as hash input
 * @param uint32_t* dst - pointer to an array of 8 X 32 bit words used as hash output
 * @param int16_t len - number of 32 bit words available in array pointed by 'src'
 *
 * @return int 0 for successful operation, -1 for invalid parameters
 *
 */
int SwiftRngApi::sha256_generateHash(const uint32_t *src, int16_t len, uint32_t *dst) {

	uint16_t blockNum;
	uint8_t ui8;
	int32_t initialMessageSize;
	uint16_t numCompleteDataBlocks;
	uint16_t reminder;
	uint16_t srcOffset;
	uint8_t needAdditionalBlock;
	uint8_t needToAddOneMarker;

	if (len <= 0) {
		return -1;
	}

	sha256_initialize();

	initialMessageSize = len * 8 * 4;
	numCompleteDataBlocks = len / m_max_data_block_size_words;
	reminder = len % m_max_data_block_size_words;

	// Process complete blocks
	for (blockNum = 0; blockNum < numCompleteDataBlocks; blockNum++) {
		srcOffset = blockNum * m_max_data_block_size_words;
		for (ui8 = 0; ui8 < m_max_data_block_size_words; ui8++) {
			m_sha256_ctxt.w[ui8] = src[ui8 + srcOffset];
		}
		// Hash the current block
		sha256_hashCurrentBlock();
	}

	srcOffset = numCompleteDataBlocks * m_max_data_block_size_words;
	needAdditionalBlock = 1;
	needToAddOneMarker = 1;
	if (reminder > 0) {
		// Process the last data block if any
		ui8 = 0;
		for (; ui8 < reminder; ui8++) {
			m_sha256_ctxt.w[ui8] = src[ui8 + srcOffset];
		}
		// Append '1' to the message
		m_sha256_ctxt.w[ui8++] = 0x80000000;
		needToAddOneMarker = 0;
		if (ui8 < m_max_data_block_size_words - 1) {
			for (; ui8 < m_max_data_block_size_words - 2; ui8++) {
				// Fill with zeros
				m_sha256_ctxt.w[ui8] = 0x0;
			}
			// add the message size to the current block
			m_sha256_ctxt.w[ui8++] = 0x0;
			m_sha256_ctxt.w[ui8] = initialMessageSize;
			sha256_hashCurrentBlock();
			needAdditionalBlock = 0;
		} else {
			// Fill the rest with '0'
			// Will need to create another block
			m_sha256_ctxt.w[ui8] = 0x0;
			sha256_hashCurrentBlock();
		}
	}

	if (needAdditionalBlock) {
		ui8 = 0;
		if (needToAddOneMarker) {
			m_sha256_ctxt.w[ui8++] = 0x80000000;
		}
		for (; ui8 < m_max_data_block_size_words - 2; ui8++) {
			m_sha256_ctxt.w[ui8] = 0x0;
		}
		m_sha256_ctxt.w[ui8++] = 0x0;
		m_sha256_ctxt.w[ui8] = initialMessageSize;
		sha256_hashCurrentBlock();
	}

	// Save the results
	dst[0] = m_sha256_ctxt.h0;
	dst[1] = m_sha256_ctxt.h1;
	dst[2] = m_sha256_ctxt.h2;
	dst[3] = m_sha256_ctxt.h3;
	dst[4] = m_sha256_ctxt.h4;
	dst[5] = m_sha256_ctxt.h5;
	dst[6] = m_sha256_ctxt.h6;
	dst[7] = m_sha256_ctxt.h7;

	return 0;
}

/**
 * Hash current block
 *
 */
void SwiftRngApi::sha512_hashCurrentBlock() {

	// Process elements 16...79
	for (uint8_t t = 16; t <= 79; t++) {
		m_sha512_ctxt.w[t] = sha512_sigma1(&m_sha512_ctxt.w[t-2]) + m_sha512_ctxt.w[t-7] + sha512_sigma0(&m_sha512_ctxt.w[t-15]) + m_sha512_ctxt.w[t-16];
	}

	// Initialize variables
	m_sha512_ctxt.a = m_sha512_ctxt.h0;
	m_sha512_ctxt.b = m_sha512_ctxt.h1;
	m_sha512_ctxt.c = m_sha512_ctxt.h2;
	m_sha512_ctxt.d = m_sha512_ctxt.h3;
	m_sha512_ctxt.e = m_sha512_ctxt.h4;
	m_sha512_ctxt.f = m_sha512_ctxt.h5;
	m_sha512_ctxt.g = m_sha512_ctxt.h6;
	m_sha512_ctxt.h = m_sha512_ctxt.h7;

	// Process elements 0...79
	for (uint8_t t = 0; t <= 79; t++) {
		m_sha512_ctxt.tmp1 = m_sha512_ctxt.h + sha512_sum1(&m_sha512_ctxt.e) + sha512_ch(&m_sha512_ctxt.e, &m_sha512_ctxt.f, &m_sha512_ctxt.g) + c_sha512_k[t] + m_sha512_ctxt.w[t];
		m_sha512_ctxt.tmp2 = sha512_sum0(&m_sha512_ctxt.a) + sha512_maj(&m_sha512_ctxt.a, &m_sha512_ctxt.b, &m_sha512_ctxt.c);
		m_sha512_ctxt.h = m_sha512_ctxt.g;
		m_sha512_ctxt.g = m_sha512_ctxt.f;
		m_sha512_ctxt.f = m_sha512_ctxt.e;
		m_sha512_ctxt.e = m_sha512_ctxt.d + m_sha512_ctxt.tmp1;
		m_sha512_ctxt.d = m_sha512_ctxt.c;
		m_sha512_ctxt.c = m_sha512_ctxt.b;
		m_sha512_ctxt.b = m_sha512_ctxt.a;
		m_sha512_ctxt.a = m_sha512_ctxt.tmp1 + m_sha512_ctxt.tmp2;
	}

	// Calculate the final hash for the block
	m_sha512_ctxt.h0 += m_sha512_ctxt.a;
	m_sha512_ctxt.h1 += m_sha512_ctxt.b;
	m_sha512_ctxt.h2 += m_sha512_ctxt.c;
	m_sha512_ctxt.h3 += m_sha512_ctxt.d;
	m_sha512_ctxt.h4 += m_sha512_ctxt.e;
	m_sha512_ctxt.h5 += m_sha512_ctxt.f;
	m_sha512_ctxt.h6 += m_sha512_ctxt.g;
	m_sha512_ctxt.h7 += m_sha512_ctxt.h;
}

/**
 * Hash current block
 *
 */
void SwiftRngApi::sha256_hashCurrentBlock() {
	// Process elements 16...63
	for (uint8_t t = 16; t <= 63; t++) {
		m_sha256_ctxt.w[t] = sha256_sigma1(&m_sha256_ctxt.w[t - 2]) + m_sha256_ctxt.w[t - 7] + sha256_sigma0(
				&m_sha256_ctxt.w[t - 15]) + m_sha256_ctxt.w[t - 16];
	}

	// Initialize variables
	m_sha256_ctxt.a = m_sha256_ctxt.h0;
	m_sha256_ctxt.b = m_sha256_ctxt.h1;
	m_sha256_ctxt.c = m_sha256_ctxt.h2;
	m_sha256_ctxt.d = m_sha256_ctxt.h3;
	m_sha256_ctxt.e = m_sha256_ctxt.h4;
	m_sha256_ctxt.f = m_sha256_ctxt.h5;
	m_sha256_ctxt.g = m_sha256_ctxt.h6;
	m_sha256_ctxt.h = m_sha256_ctxt.h7;

	// Process elements 0...63
	for (uint8_t t = 0; t <= 63; t++) {
		m_sha256_ctxt.tmp1 = m_sha256_ctxt.h + sha256_sum1(&m_sha256_ctxt.e) + sha256_ch(&m_sha256_ctxt.e, &m_sha256_ctxt.f, &m_sha256_ctxt.g)
				+ c_sha256_k[t] + m_sha256_ctxt.w[t];
		m_sha256_ctxt.tmp2 = sha256_sum0(&m_sha256_ctxt.a) + sha256_maj(&m_sha256_ctxt.a, &m_sha256_ctxt.b, &m_sha256_ctxt.c);
		m_sha256_ctxt.h = m_sha256_ctxt.g;
		m_sha256_ctxt.g = m_sha256_ctxt.f;
		m_sha256_ctxt.f = m_sha256_ctxt.e;
		m_sha256_ctxt.e = m_sha256_ctxt.d + m_sha256_ctxt.tmp1;
		m_sha256_ctxt.d = m_sha256_ctxt.c;
		m_sha256_ctxt.c = m_sha256_ctxt.b;
		m_sha256_ctxt.b = m_sha256_ctxt.a;
		m_sha256_ctxt.a = m_sha256_ctxt.tmp1 + m_sha256_ctxt.tmp2;
	}

	// Calculate the final hash for the block
	m_sha256_ctxt.h0 += m_sha256_ctxt.a;
	m_sha256_ctxt.h1 += m_sha256_ctxt.b;
	m_sha256_ctxt.h2 += m_sha256_ctxt.c;
	m_sha256_ctxt.h3 += m_sha256_ctxt.d;
	m_sha256_ctxt.h4 += m_sha256_ctxt.e;
	m_sha256_ctxt.h5 += m_sha256_ctxt.f;
	m_sha256_ctxt.h6 += m_sha256_ctxt.g;
	m_sha256_ctxt.h7 += m_sha256_ctxt.h;
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.2)
 *
 * @param uint32_t* x pointer to variable x
 * @param uint32_t* y pointer to variable y
 * @param uint32_t* z pointer to variable z
 * $return uint32_t Ch value
 *
 */
uint32_t SwiftRngApi::sha256_ch(uint32_t *x, uint32_t *y, uint32_t *z) {
	return ((*x) & (*y)) ^ (~(*x) & (*z));
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.3)
 *
 * @param uint32_t* x pointer to variable x
 * @param uint32_t* y pointer to variable y
 * @param uint32_t* z pointer to variable z
 * $return uint32_t Maj value
 *
 */
uint32_t SwiftRngApi::sha256_maj(const uint32_t *x, const uint32_t *y, const uint32_t *z) {
	return ((*x) & (*y)) ^ ((*x) & (*z)) ^ ((*y) & (*z));
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.4)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t Sum0 value
 *
 */
uint32_t SwiftRngApi::sha256_sum0(const uint32_t *x) {
	return rotr32(2, *x) ^ rotr32(13, *x) ^ rotr32(22, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.5)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t Sum1 value
 *
 */
uint32_t SwiftRngApi::sha256_sum1(const uint32_t *x) {
	return rotr32(6, *x) ^ rotr32(11, *x) ^ rotr32(25, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.6)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t sigma0 value
 *
 */
uint32_t SwiftRngApi::sha256_sigma0(const uint32_t *x) {
	return rotr32(7, *x) ^ rotr32(18, *x) ^ ((*x) >> 3);
}

/**
 * FIPS PUB 180-4 section 4.1.2 formula (4.7)
 *
 * @param uint32_t* x pointer to variable x
 * $return uint32_t sigma1 value
 *
 */
uint32_t SwiftRngApi::sha256_sigma1(const uint32_t *x) {
	return rotr32(17, *x) ^ rotr32(19, *x) ^ ((*x) >> 10);
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.8)
 *
 * @param uint64_t* x pointer to variable x
 * @param uint64_t* y pointer to variable y
 * @param uint64_t* z pointer to variable z
 * $return uint64_t ch value
 *
 */
uint64_t SwiftRngApi::sha512_ch(const uint64_t *x, const uint64_t *y, const uint64_t *z) {
	return  ((*x) & (*y)) ^ (~(*x) & (*z));
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.9)
 *
 * @param uint64_t* x pointer to variable x
 * @param uint64_t* y pointer to variable y
 * @param uint64_t* z pointer to variable z
 * $return uint64_t maj value
 *
 */
uint64_t SwiftRngApi::sha512_maj(const uint64_t *x, const uint64_t *y, const uint64_t *z) {
	return ((*x) & (*y)) ^ ((*x) & (*z)) ^ ((*y) & (*z));
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.10)
 *
 * @param uint64_t* x pointer to variable x
 * $return uint64_t sum0 value
 *
 */
uint64_t SwiftRngApi::sha512_sum0(const uint64_t *x) {
	return rotr64(28, *x) ^ rotr64(34, *x) ^ rotr64(39, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.11)
 *
 * @param uint64_t* x pointer to variable x
 * $return uint64_t sum1 value
 *
 */
uint64_t SwiftRngApi::sha512_sum1(const uint64_t *x) {
	return rotr64(14, *x) ^ rotr64(18, *x) ^ rotr64(41, *x);
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.12)
 *
 * @param uint64_t* x pointer to variable x
 * $return uint64_t sigma0 value
 *
 */
uint64_t SwiftRngApi::sha512_sigma0(const uint64_t *x) {
	return rotr64(1, *x) ^ rotr64(8, *x) ^ ((*x) >> 7);
}

/**
 * FIPS PUB 180-4 section 4.1.3 formula (4.13)
 *
 * @param uint64_t* x pointer to variable x
 * $return uint64_t sigma1 value
 *
 */
uint64_t SwiftRngApi::sha512_sigma1(const uint64_t *x) {
	return rotr64(19, *x) ^ rotr64(61, *x) ^ ((*x) >> 6);
}


/*
 * A function for running the self test for the SHA256 post processing method
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int SwiftRngApi::sha256_selfTest() {
	uint32_t results[8];
	int retVal;

	retVal = sha256_generateHash(c_sha256_test_seq_1, (uint16_t) 11, (uint32_t*) results);
	if (retVal == 0) {
		// Compare the expected with actual results
		retVal = memcmp(results, c_sha256_expt_hash_seq_1, 8);
	}
	return retVal;
}

/*
 * A function for running the self test for the SHA512 post processing method
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int SwiftRngApi::sha512_selfTest() {
	uint64_t results[8];
	int retVal;
	if (is_context_initialized() == false) {
		return -1;
	}

	retVal = sha512_generateHash((const uint64_t *)"8765432187654321876543218765432187654321876543218765432187654321",
			(uint16_t) 8, (uint64_t*) results);
	if (retVal == 0) {
		// Compare the expected with actual results
		retVal = memcmp(results, c_sha512_expt_hash_seq1, 8);
	}
	return retVal;
}

/*
 * A function for running the self test for the xorshift64 post processing method
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int SwiftRngApi::xorshift64_selfTest() {
	uint64_t rawWord = 0x1212121212121212;
	uint64_t testWord = 0x2322d6d77d8b7b55;
	xorshift64_postProcess((uint8_t*)&rawWord, 8);

	if (rawWord == testWord) {
		return SWRNG_SUCCESS;
	} else {
		return -1;
	}
}

/**
* Apply Xorshift64 (Marsaglia's PPRNG method) to the raw word
* @param raw_word - word to post process
*/
uint64_t SwiftRngApi::xorshift64_postProcessWord(uint64_t raw_word) const {
	uint64_t trueWord = raw_word;

	trueWord ^= trueWord >> 12;
	trueWord ^= trueWord << 25;
	trueWord ^= trueWord >> 27;
	return trueWord * UINT64_C(2685821657736338717);
}

/**
*
* @param buffer - pointer to input data buffer
* @param num_elements - number of elements in the input buffer
*/
void SwiftRngApi::xorshift64_postProcessWords(uint64_t *buffer, int num_elements) {
	for(int i = 0; i < num_elements; i++) {
		buffer[i] = xorshift64_postProcessWord(buffer[i]);
	}
}

/**
* @param buffer - pointer to input data buffer
* @param num_elements - number of elements in the input buffer
*/
void SwiftRngApi::xorshift64_postProcess(uint8_t *buffer, int num_elements) {
	xorshift64_postProcessWords((uint64_t *)buffer, num_elements / 8);
}

/**
 * A function to initialize the repetition count test
 *
 *
 */
void SwiftRngApi::rct_initialize() {
	memset(&m_rct, 0x00, sizeof(m_rct));
	m_rct.statusByte = 0;
	m_rct.signature = 1;
	m_rct.maxRepetitions = 5;
	rct_restart();
}

/**
 * A function to restart the repetition count test
 *
 */
void SwiftRngApi::rct_restart() {
	m_rct.isInitialized = false;
	m_rct.curRepetitions = 1;
	m_rct.failureCount = 0;
}

/**
 * A function to initialize the adaptive proportion test
 *
 *
 */
void SwiftRngApi::apt_initialize() {
	memset(&m_apt, 0x00, sizeof(m_apt));
	m_apt.statusByte = 0;
	m_apt.signature = 2;
	m_apt.windowSize = 64;
	m_apt.cutoffValue = 5;
	apt_restart();
}

/**
 * A function to restart the adaptive proportion test
 *
 *
 */
void SwiftRngApi::apt_restart() {
	m_apt.isInitialized = false;
	m_apt.cycleFailures = 0;
}

void SwiftRngApi::clear_last_error_msg() {
	m_last_error_log_oss.str("");
	m_last_error_log_oss.clear();
	m_last_error_log_char[0] = '\0';
}

SwiftRngApi::~SwiftRngApi() {
	if (c_sha256_k != nullptr) {
		delete [] c_sha256_k;
	}

	if (c_sha256_test_seq_1 != nullptr) {
		delete [] c_sha256_test_seq_1;
	}

	if (c_sha256_expt_hash_seq_1 != nullptr) {
		delete [] c_sha256_expt_hash_seq_1;
	}


	if (c_sha512_k != nullptr) {
		delete [] c_sha512_k;
	}

	if (c_sha512_expt_hash_seq1 != nullptr) {
		delete [] c_sha512_expt_hash_seq1;
	}

	if (m_usb_serial_device != nullptr) {
		delete m_usb_serial_device;
	}

	if (m_buff_rnd_out != nullptr) {
		delete [] m_buff_rnd_out;
	}

	if (m_bulk_in_buffer != nullptr) {
		delete [] m_bulk_in_buffer;
	}

	if (m_buff_rnd_in != nullptr) {
		delete [] m_buff_rnd_in;
	}

	if (m_src_to_hash_32 != nullptr) {
		delete [] m_src_to_hash_32;
	}

	if (m_src_to_hash_64 != nullptr) {
		delete [] m_src_to_hash_64;
	}

	if (m_last_error_log_char != nullptr) {
		delete [] m_last_error_log_char;
	}

}


const std::string SwiftRngApi::c_dev_not_open_msg = "Device not open";
const std::string SwiftRngApi::c_freq_table_invalid_msg = "Invalid FrequencyTables pointer argument";
const std::string SwiftRngApi::c_cannot_disable_pp_msg = "Post processing cannot be disabled for device";
const std::string SwiftRngApi::c_pp_op_not_supported_msg = "Post processing method not supported for device";
const std::string SwiftRngApi::c_diag_op_not_supported_msg = "Diagnostics not supported for device";
const std::string SwiftRngApi::c_pp_method_not_supported_msg = "Post processing method not supported";
const std::string SwiftRngApi::c_cannot_get_freq_table_for_device_msg = "Frequency tables can only be retrieved for devices with version 1.2 and up";
const std::string SwiftRngApi::c_too_many_devices_msg = "Cannot have more than 127 USB devices";
const std::string SwiftRngApi::c_cannot_read_device_descriptor_msg = "Failed to retrieve USB device descriptor";
const std::string SwiftRngApi::c_libusb_init_failure_msg = "Failed to initialize libusb";
#ifdef _WIN32
const std::wstring SwiftRngApi::c_hardware_id = L"USB\\VID_1FC9&PID_8111";
#endif


} /* namespace swiftrng */
