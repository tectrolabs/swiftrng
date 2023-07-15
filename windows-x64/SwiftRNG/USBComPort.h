/*
* USBComPort.h
* Ver 1.2
*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

This class implements access to a SwiftRNG device over CDC USB interface on Windows platform.

This class may only be used in conjunction with TectroLabs devices.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef SWRNGUSBCOMPORT_H_
#define SWRNGUSBCOMPORT_H_


#include <initguid.h>
#include <windows.h>
#include <Setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <combaseapi.h>

class USBComPort
{
public:
	USBComPort();
	~USBComPort();
	void initialize();
	bool isConnected();
	bool connect(WCHAR *comPort);
	bool disconnect();
	int executeDeviceCommand(unsigned char *snd, int sizeSnd, unsigned char *rcv, int sizeRcv);
	void getConnectedPorts(int ports[], int maxPorts, int* actualCount, WCHAR *hardwareId);
	void toPortName(int portNum, WCHAR* portName, int portNameSize);
	const char* getLastErrMsg();
	int sendCommand(unsigned char *snd, int sizeSnd, int *bytesSend);
	int receiveDeviceData(unsigned char *rcv, int sizeRcv, int *bytesReveived);

private:
	HANDLE cdcUsbDevHandle;
	bool deviceConnected;
	char lastError[512];
	COMSTAT commStatus;
	DWORD commError;
	void setErrMsg(const char* errMessage);
	void clearErrMsg();
	void purgeComm();
	void clearCommErr();

};

#endif /* SWRNGUSBCOMPORT_H_ */