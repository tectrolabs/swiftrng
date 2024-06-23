/*
 * USBSerialDevice.h
 * Ver 1.3
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class implements access to SwiftRNG device over CDC USB interface on Linux and macOS platforms.

 This class may only be used in conjunction with TectroLabs devices.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef USBSERIALDEVICE_H_
#define USBSERIALDEVICE_H_

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <ctype.h>
#include <sstream>

#ifdef __FreeBSD__
#include <sys/param.h>
#endif

class USBSerialDevice {
public:
	USBSerialDevice();
	virtual ~USBSerialDevice();
	void initialize();
	bool is_connected() const;
	bool connect(const char *devicePath);
	bool disconnect();
	void clear_error_log();
	std::string get_error_log() const;
	int send_command(const unsigned char *snd, int sizeSnd, int *bytesSent);
	int receive_data(unsigned char *rcv, int sizeRcv, int *bytesReceived);
	int get_device_count() const;
	void scan_available_devices();
	bool retrieve_device_path(char *devName, int devNum) const;

private:
	void set_error_message(const char *error_message);
	void purge_comm_data() const;

private:
	static const int c_max_devices = 25;
	static const int c_max_size_device_name = 128;
	int m_fd {-1};
	int m_lock;
	char c_device_names[c_max_devices][c_max_size_device_name];
	int m_active_device_count {0};
	bool m_device_connected {false};
	std::ostringstream m_error_log_oss;

};

#endif /* USBSERIALDEVICE_H_ */
