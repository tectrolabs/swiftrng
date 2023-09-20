/*
 * USBSerialDevice.cpp
 * Ver 1.4
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2023 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class implements access to SwiftRNG device over CDC USB interface on Linux and macOS platforms.

 This class may only be used in conjunction with TectroLabs devices.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "USBSerialDevice.h"

USBSerialDevice::USBSerialDevice() {
	clear_error_log();
}

void USBSerialDevice::clear_error_log() {
	m_error_log_oss.str("");
	m_error_log_oss.clear();
}

void USBSerialDevice::initialize() {
	disconnect();
}

bool USBSerialDevice::is_connected() const {
	return m_device_connected;
}


bool USBSerialDevice::connect(const char *devicePath) {
	if (is_connected()) {
		return false;
	}

	clear_error_log();

	m_fd = open(devicePath, O_RDWR | O_NOCTTY);
	if (m_fd == -1) {
		m_error_log_oss << "Could not open serial device: " << devicePath << ". ";
		return false;
	}

	// Lock the device
	m_lock = flock(m_fd, LOCK_EX | LOCK_NB);
	if (m_lock != 0) {
		m_error_log_oss << "Could not lock device: " << devicePath << ". ";
		close(m_fd);
		return false;
	}

	purge_comm_data();

	struct termios opts;
	int retVal = tcgetattr(m_fd, &opts);
	if (retVal) {
		m_error_log_oss << "Could not retrieve configuration from serial device: " << devicePath << ". ";
		close(m_fd);
		return false;
	}

	opts.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	opts.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	opts.c_oflag &= ~(ONLCR | OCRNL);

	// Set time out to 100 milliseconds for read serial device operations
	opts.c_cc[VTIME] = 1;
	opts.c_cc[VMIN] = 0;

	retVal = tcsetattr(m_fd, TCSANOW, &opts);
	if (retVal) {
		m_error_log_oss << "Could not set configuration for serial device: " << devicePath << ". ";
		close(m_fd);
		return false;
	}

	m_device_connected = true;
	return true;
}

void USBSerialDevice::set_error_message(const char *error_message) {
	m_error_log_oss << error_message;
}

bool USBSerialDevice::disconnect() {
	if (!is_connected()) {
		return false;
	}
	flock(m_fd, LOCK_UN);
	close(m_fd);
	m_device_connected = false;
	clear_error_log();
	return true;
}

int USBSerialDevice::send_command(const unsigned char *snd, int sizeSnd, int *bytesSent) {
	if (!is_connected()) {
		return -1;
	}
	ssize_t retVal = write(m_fd, snd, sizeSnd);
	if (retVal != (ssize_t)sizeSnd) {
		m_error_log_oss << "Could not send command to serial device.";
		return -1;
	}
	*bytesSent = (int)retVal;
	return 0;
}

std::string USBSerialDevice::get_error_log() const {
	return m_error_log_oss.str();
}

void USBSerialDevice::purge_comm_data() const {
	if (m_fd != -1) {
		tcflush(m_fd, TCIOFLUSH);
	}
}

int USBSerialDevice::receive_data(unsigned char *rcv, int sizeRcv, int *bytesReceived) {
	int returnStatus = 0;
	if (!is_connected()) {
		return -1;
	}

	size_t actualReceivedBytes = 0;
	bool inLoop = true;
	while (actualReceivedBytes < (size_t)sizeRcv && inLoop) {
		ssize_t receivedCount = read(m_fd, rcv + actualReceivedBytes, sizeRcv - actualReceivedBytes);
		if (receivedCount < 0) {
			m_error_log_oss << "Could not receive data from serial device.";
			returnStatus = -1;
			inLoop = false;
		} else if (receivedCount == 0) {
			// Operation timed out
			returnStatus = -7;
			inLoop = false;
		} else {
			actualReceivedBytes += receivedCount;
		}
	  }
	*bytesReceived = (int)actualReceivedBytes;
	return returnStatus;
}

USBSerialDevice::~USBSerialDevice() {
	if (is_connected()) {
		disconnect();
	}
}


#ifndef __FreeBSD__
void USBSerialDevice::scan_available_devices() {
	m_active_device_count = 0;
#ifdef __linux__
	char command[] = "/bin/ls -1l /dev/serial/by-id 2>&1 | grep -i \"TectroLabs_SwiftRNG\"";
#else
	char command[] = "/bin/ls -1a /dev/cu.usbmodemSWRNG* /dev/cu.usbmodemFD* 2>&1";
#endif
	FILE *pf = popen(command,"r");
	if (pf == nullptr) {
		return;
	}

	char line[512];
	while (fgets(line, sizeof(line), pf) && m_active_device_count < c_max_devices) {
#ifdef __linux__
		char *tty = strstr(line, "ttyACM");
		if (tty == NULL) {
			continue;
		}
#else
		int cmp1  = strncmp(line, "/dev/cu.usbmodemSWRNG", 21);
		int cmp2  = strncmp(line, "/dev/cu.usbmodemFD", 18);
		if (cmp1 != 0 && cmp2 != 0 ) {
			continue;
		}
		char *tty = line;
#endif
		auto sizeTty = (int)strlen(tty);
		for (int i = 0; i < sizeTty; i++) {
			if(tty[i] < 33 || tty[i] > 125) {
				tty[i] = 0;
			}
		}
#ifdef __linux__
		strcpy(c_device_names[m_active_device_count], "/dev/");
		strcat(c_device_names[m_active_device_count], tty);
#else
		strcpy(c_device_names[m_active_device_count], tty);
#endif
		m_active_device_count++;
	}
	pclose(pf);
}
#endif

#ifdef __FreeBSD__
void USBSerialDevice::scan_available_devices() {
	m_active_device_count = 0;
	int device_candidate = false;
	char command[] = "usbconfig show_ifdrv | grep -E \"TectroLabs SwiftRNG|VCOM\" | grep -vi \"(tectrolabs)\"";
	FILE *pf = popen(command,"r");
	if (pf == nullptr) {
		return;
	}

	char line[512];
	while (fgets(line, sizeof(line), pf) && m_active_device_count < c_max_devices) {
		if (device_candidate == false && strstr(line, "SwiftRNG") != nullptr) {
			device_candidate = true;
			continue;
		}
		if (device_candidate) {
			if (strstr(line, "VCOM") != nullptr && strstr(line, "umodem") != nullptr) {
				// Found VCOM description. Extract 'cuaU' number.
				char *p = strstr(line, "umodem");
				if (p == nullptr) {
					continue;
				}
				char *tkn = strtok(p + strlen("umodem"), ":");
				if (tkn != nullptr) {
					strcpy(c_device_names[m_active_device_count], "/dev/cuaU");
					strcat(c_device_names[m_active_device_count], tkn);
					m_active_device_count++;
					device_candidate = false;
				}
			} else {
				device_candidate = false;
			}
		}
	}
	pclose(pf);
}
#endif

int USBSerialDevice::get_device_count() const {
	return m_active_device_count;
}

bool USBSerialDevice::retrieve_device_path(char *devName, int devNum) const {
	if (devNum >= m_active_device_count) {
		return false;
	}
	strcpy(devName, c_device_names[devNum]);
	return true;
}
