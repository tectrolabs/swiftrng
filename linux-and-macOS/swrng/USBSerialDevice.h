/*
 * USBSerialDevice.h
 * Ver 1.0
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

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

#define MAX_DEVICE_COUNT (25)
#define MAX_SIZE_DEVICE_NAME (128)

class USBSerialDevice {
private:
	int fd;
	int lock;
	char devNames[MAX_DEVICE_COUNT][MAX_SIZE_DEVICE_NAME];
	int activeDevCount;
	bool deviceConnected;
	char lastError[512];
	void setErrMsg(const char *errMessage);
	void clearErrMsg();
	void purgeComm();
public:
	USBSerialDevice();
	virtual ~USBSerialDevice();
	void initialize();
	bool isConnected();
	bool connect(const char *devicePath);
	bool disconnect();
	const char* getLastErrMsg();
	int sendCommand(unsigned char *snd, int sizeSnd, int *bytesSent);
	int receiveDeviceData(unsigned char *rcv, int sizeRcv, int *bytesReceived);
	int getConnectedDeviceCount();
	void scanForConnectedDevices();
	bool retrieveConnectedDevice(char *devName, int devNum);

};

#endif /* USBSERIALDEVICE_H_ */
