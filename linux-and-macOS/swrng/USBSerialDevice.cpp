/*
 * USBSerialDevice.cpp
 * Ver 1.0
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class implements access to SwiftRNG device over CDC USB interface on Linux and macOS platforms.

 This class may only be used in conjunction with TectroLabs devices.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "USBSerialDevice.h"

USBSerialDevice::USBSerialDevice() {
	this-> deviceConnected = false;
	this->fd = -1;
	activeDevCount = 0;
	clearErrMsg();
}

void USBSerialDevice::initialize() {
	disconnect();
}

void USBSerialDevice::clearErrMsg() {
	setErrMsg("");
}

bool USBSerialDevice::isConnected() {
	return this->deviceConnected;
}


bool USBSerialDevice::connect(const char *devicePath) {
	if (isConnected()) {
		return false;
	}

	clearErrMsg();

	this->fd = open(devicePath, O_RDWR | O_NOCTTY);
	if (this->fd == -1) {
		sprintf(lastError, "Could not open serial device: %s", devicePath);
		return false;;
	}

	purgeComm();

	struct termios opts;
	int retVal = tcgetattr(fd, &opts);
	if (retVal) {
		sprintf(lastError, "Could not retrieve configuration from serial device: %s", devicePath);
		close(fd);
		return false;
	}

	opts.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	opts.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	opts.c_oflag &= ~(ONLCR | OCRNL);

	// Set time out to 100 milliseconds for read serial device operations
	opts.c_cc[VTIME] = 1;
	opts.c_cc[VMIN] = 0;

	retVal = tcsetattr(fd, TCSANOW, &opts);
	if (retVal) {
		sprintf(lastError, "Could not set configuration for serial device: %s", devicePath);
		close(fd);
		return false;
	}

	this->deviceConnected = true;
	return true;
}

void USBSerialDevice::setErrMsg(const char *errMessage) {
	strcpy(this->lastError, errMessage);
}

bool USBSerialDevice::disconnect() {
	if (!isConnected()) {
		return false;
	}
	close(fd);
	this->deviceConnected = false;
	clearErrMsg();
	return true;
}

int USBSerialDevice::sendCommand(unsigned char *snd, int sizeSnd, int *bytesSent) {
	if (!isConnected()) {
		return -1;
	}
	ssize_t retVal = write(this->fd, snd, sizeSnd);
	if (retVal != (ssize_t)sizeSnd) {
		setErrMsg("Could not send command to serial device");
		return -1;
	}
	*bytesSent = retVal;
	return 0;
}

const char* USBSerialDevice::getLastErrMsg() {
	return this->lastError;
}

void USBSerialDevice::purgeComm() {
	if (this->fd != -1) {
		tcflush(this->fd, TCIOFLUSH);
	}
}

int USBSerialDevice::receiveDeviceData(unsigned char *rcv, int sizeRcv, int *bytesReceived) {
	int returnStatus = 0;
	if (!isConnected()) {
		return -1;
	}

	size_t actualReceivedBytes = 0;
	while (actualReceivedBytes < (size_t)sizeRcv) {
		ssize_t receivedCount = read(this->fd, rcv + actualReceivedBytes, sizeRcv - actualReceivedBytes);
		if (receivedCount < 0) {
			setErrMsg("Could not receive data from serial device");
			returnStatus = -1;
			break;
		}
		if (receivedCount == 0) {
			// Operation timed out
			returnStatus = -7;
			break;
		}
		actualReceivedBytes += receivedCount;
	  }
	*bytesReceived = actualReceivedBytes;
	return returnStatus;
}

USBSerialDevice::~USBSerialDevice() {
	if (isConnected()) {
		disconnect();
	}
}

void USBSerialDevice::scanForConnectedDevices() {
	activeDevCount = 0;
#ifdef __linux__
	char command[] = "/bin/ls -1l /dev/serial/by-id 2>&1 | grep -i \"TectroLabs_SwiftRNG\"";
#else
	char command[] = "/bin/ls -1a /dev/cu.usbmodemSWRNG* 2>&1";
#endif
	FILE *pf = popen(command,"r");
	if (pf == NULL) {
		return;
	}

	char line[512];
	while (fgets(line, sizeof(line), pf) && activeDevCount < MAX_DEVICE_COUNT) {
#ifdef __linux__
		char *tty = strstr(line, "ttyACM");
#else
		char *tty = strstr(line, "/dev/cu.usbmodemSWRNG");
#endif
		if (tty == NULL) {
			continue;
		}
		int sizeTty = strlen(tty);
		for (int i = 0; i < sizeTty; i++) {
			if(tty[i] < 33 || tty[i] > 125) {
				tty[i] = 0;
			}
		}
#ifdef __linux__
		strcpy(devNames[activeDevCount], "/dev/");
		strcat(devNames[activeDevCount], tty);
#else
		strcpy(devNames[activeDevCount], tty);
#endif
		activeDevCount++;
	}
	pclose(pf);

}

int USBSerialDevice::getConnectedDeviceCount() {
	return activeDevCount;
}

bool USBSerialDevice::retrieveConnectedDevice(char *devName, int devNum) {
	if (devNum >= activeDevCount) {
		return false;
	}
	strcpy(devName, devNames[devNum]);
	return true;
}
