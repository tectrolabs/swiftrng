#include "stdafx.h"
/*
* USBComPort.cpp
* Ver 1.0
*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (C) 2014-2020 TectroLabs, https://tectrolabs.com

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

This class implements access to a SwiftRNG device over CDC USB interface on Windows platform.

This class may only be used in conjunction with TectroLabs devices.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include "USBComPort.h"

USBComPort::USBComPort()
{
	this->deviceConnected = false;
	clearErrMsg();
}

void USBComPort::initialize() {
	disconnect();
}

/**
* Clear the last error message
*/
void USBComPort::clearErrMsg() {
	setErrMsg("");
}

/**
* Check to see if device is open
*
* @param  true if there is an active connection
*
*/
bool USBComPort::isConnected() {
	return this->deviceConnected;
}

/**
* Connect to SwiftRNG device through COM port name provided
*
* @param WCHAR *comPort - pointer to a COM port pathname
* @return true - successful operation
*
*/
bool USBComPort::connect(WCHAR *comPort) {
	if (this->isConnected()) {
		return false;
	}

	clearErrMsg();

	this->cdcUsbDevHandle = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (this->cdcUsbDevHandle == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			setErrMsg("COM port not found");
		}
		else {
			setErrMsg("Could not open COM port");
		}
		return false;
	}

	COMMTIMEOUTS timeOut = { 0 };
	timeOut.ReadIntervalTimeout = 0;
	timeOut.ReadTotalTimeoutConstant = 100;
	timeOut.ReadTotalTimeoutMultiplier = 0;
	timeOut.WriteTotalTimeoutConstant = 50;
	timeOut.WriteTotalTimeoutMultiplier = 0;
	SetCommTimeouts(cdcUsbDevHandle, &timeOut);

	purgeComm();
	this->deviceConnected = true;
	return true;
}

/**
* Save error message so it can be retrieved at later time
*
* @param const char *errMessage - pointer to error ASCIIZ text
* 
*/
void USBComPort::setErrMsg(const char *errMessage) {
	strcpy(this->lastError, errMessage);
}

/**
* Disconnect from SwiftRNG device 
*
* @return true - successful operation
*
*/
bool USBComPort::disconnect() {
	if (!this->isConnected()) {
		return false;
	}
	CloseHandle(this->cdcUsbDevHandle);
	this->deviceConnected = false;
	clearErrMsg();
	return true;
}

/**
* Send specified command to SwiftRNG device
* 
* @param unsigned char *snd - pointer to a command to send
* @param int sizeSnd - the size of command in bytes
* @return 0 - successful operation, otherwise the error code
*
*/
int USBComPort::sendCommand(unsigned char *snd, int sizeSnd, int *bytesSend) {
	DWORD actualBytesSent;
	int retStatus = -1;
	if (!this->isConnected()) {
		return retStatus;
	}

	BOOL status = WriteFile(this->cdcUsbDevHandle, (void *)snd, sizeSnd, &actualBytesSent, 0);
	*bytesSend = (int)actualBytesSent;
	if (!status || *bytesSend != sizeSnd)
	{
		if (status && *bytesSend != sizeSnd) {
			retStatus = -7; // Time out
			setErrMsg("Got timeout while sending data to device");
		}
		else {
			setErrMsg("Could not send data to device");
		}

		clearCommErr();
		purgeComm();
		return retStatus;
	}
	return 0;
}

/**
* Receive data from SwiftRNG device
*
* @param unsigned char *rcv - a pointer to receive buffer
* @param int sizeRcv - how many bytes to receive
* @return 0 - successful operation, otherwise the error code
*
*/
int USBComPort::receiveDeviceData(unsigned char *rcv, int sizeRcv, int *bytesReveived) {
	DWORD actualBytesReceived;
	int retStatus = -1;
	if (!this->isConnected()) {
		return retStatus;
	}

	BOOL status = ReadFile(this->cdcUsbDevHandle, (void *)rcv, sizeRcv, &actualBytesReceived, NULL);
	*bytesReveived = (int)actualBytesReceived;
	if (!status || *bytesReveived != sizeRcv) {
		if (status && *bytesReveived != sizeRcv) {
			retStatus = -7; // Time out
			setErrMsg("Got timeout while receiving data from the device");
		} else {
			setErrMsg("Could not receive data from the device");
		}

		clearCommErr();
		purgeComm();
		return retStatus;
	}
	return 0;
}

/**
* Retrieve the last error message
*
* @return pointer to the last error message
*
*/
const char * USBComPort::getLastErrMsg() {
	return this->lastError;
}

/**
* Send a command to SwiftRNG and receive data as a result
* 
* @param unsigned char *snd - pointer to a command to send
* @param int sizeSnd - the size of command in bytes
* @param unsigned char *rcv - a pointer to receive buffer
* @param int sizeRcv - how many bytes to receive
* @return 0 - successful operation, otherwise the error code
*
*/
int USBComPort::executeDeviceCommand(unsigned char *snd, int sizeSnd, unsigned char *rcv, int sizeRcv) {
	int actualBytesSent;
	int status = sendCommand(snd, sizeSnd, &actualBytesSent);
	if (status) {
		return status;
	}
	if (sizeRcv == 0) {
		return status;
	}
	int actualBytesReceived;
	return receiveDeviceData(rcv, sizeRcv, &actualBytesReceived);
}

/**
* Clear COM port receive and transmit buffers
*
* @param unsigned char *rcv - a pointer to receive buffer
* @param int sizeRcv - how many bytes to receive
* @return 0 - successful operation, otherwise the error code
*
*/

void USBComPort::purgeComm() {
	if (!this->isConnected()) {
		return;
	}
	PurgeComm(this->cdcUsbDevHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

void USBComPort::clearCommErr() {
	if (!this->isConnected()) {
		return;
	}
	ClearCommError(this->cdcUsbDevHandle, &this->commError, &this->commStatus);
}

/**
* Scan registry and discover all the SwiftRNG devices attached to COM ports
*
* @param int ports[] - an array of integers that represent all of the SwiftRNG COM ports found 
* @param int maxPorts - the maximum number of SwiftRNG devices to discover
* @param actualCount - a pointer to receive buffer
* @param  int *actualCount - a pointer to the actual number of SwiftRNG devices found
* @param  WCHAR *hardwareId - a pointer to SwiftRNG device hardware ID 
* @return 0 - successful operation, otherwise the error code
*
*/
void USBComPort::getConnectedPorts(int ports[], int maxPorts, int *actualCount, WCHAR *hardwareId) {

	DWORD devIdx = 0;
	int foundPortIndex = 0;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA devInfoData;
	BYTE curHardwareId[1024] = { 0 };
	DEVPROPTYPE devPropType;
	DWORD dwSize = 0;
	DWORD error = 0;

	hDevInfo = SetupDiGetClassDevs(
		NULL,
		L"USB",
		NULL,
		DIGCF_ALLCLASSES | DIGCF_PRESENT);

	if (hDevInfo == INVALID_HANDLE_VALUE)
		return;


	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	while (SetupDiEnumDeviceInfo(
		hDevInfo,
		devIdx,
		&devInfoData) && foundPortIndex < maxPorts)
	{
		devIdx++;

		if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID, &devPropType, (BYTE*)curHardwareId, sizeof(curHardwareId), &dwSize))
		{
			HKEY hDeviceRegistryKey;
			hDeviceRegistryKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
			if (hDeviceRegistryKey == INVALID_HANDLE_VALUE)
			{
				error = GetLastError();
				break;
			}
			else
			{
				wchar_t curPortName[80];
				DWORD dwSize = sizeof(curPortName);
				DWORD dwType = 0;

				if ((RegQueryValueEx(hDeviceRegistryKey, L"PortName", NULL, &dwType, (LPBYTE)curPortName, &dwSize) == ERROR_SUCCESS) && (dwType == REG_SZ))
				{
					if (_tcsnicmp(curPortName, _T("COM"), 3) == 0)
					{
						TCHAR* src = (TCHAR*)curHardwareId;
						int size = _tcsnlen(hardwareId, 80);

						if (_tcsnicmp(hardwareId, (TCHAR*)curHardwareId, _tcsnlen(hardwareId, 80)) == 0) {
							int nPortNr = _ttoi(curPortName + 3);
							if (nPortNr != 0)
							{
								ports[foundPortIndex++] = nPortNr;
							}
						}

					}
				}
				RegCloseKey(hDeviceRegistryKey);
			}
		}

	}

	if (hDevInfo)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
	*actualCount = foundPortIndex;
}

/**
* Convert a port number to a fully qualified COM port pathname
*
* @param int portNum - COM port number as integer
* @paramW CHAR* portName - a pointer to converted fully qualified COM port pathname
* @param actualCount - a pointer to receive buffer
* @param  int portNameSize - the max number of characters available for portName
*
*/
void USBComPort::toPortName(int portNum, WCHAR* portName, int portNameSize) {
	swprintf_s(portName, portNameSize, L"\\\\.\\COM%d", portNum);
}

USBComPort::~USBComPort()
{
	if (this->isConnected()) {
		disconnect();
	}
}
