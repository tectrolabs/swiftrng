/*
* USBComPort.cpp
* Ver 1.3
*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

This class implements access to a SwiftRNG device over CDC USB interface on Windows platform.

This class may only be used in conjunction with TectroLabs devices.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include <USBComPort.h>

USBComPort::USBComPort()
{
	clear_error_log();
}

void USBComPort::initialize() {
	disconnect();
}

/**
* Clear error message log
*/
void USBComPort::clear_error_log() {
	m_error_log_oss.str("");
	m_error_log_oss.clear();
}

/**
* Check to see if device is open
*
* @param  true if there is an active connection
*
*/
bool USBComPort::is_connected() {
	return m_device_connected;
}

/**
* Connect to SwiftRNG device through COM port name provided
*
* @param WCHAR *comPort - pointer to a COM port pathname
* 
* @return true - successful operation
*
*/
bool USBComPort::connect(WCHAR *comPort) {
	if (is_connected()) {
		return false;
	}

	clear_error_log();

	m_cdc_usb_dev_handle = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_cdc_usb_dev_handle == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			set_error_message("COM port not found");
		}
		else {
			set_error_message("Could not open COM port");
		}
		return false;
	}

	COMMTIMEOUTS timeOut = { 0 };
	timeOut.ReadIntervalTimeout = 0;
	timeOut.ReadTotalTimeoutConstant = 100;
	timeOut.ReadTotalTimeoutMultiplier = 0;
	timeOut.WriteTotalTimeoutConstant = 50;
	timeOut.WriteTotalTimeoutMultiplier = 0;
	SetCommTimeouts(m_cdc_usb_dev_handle, &timeOut);

	purge_comm_data();
	m_device_connected = true;
	return true;
}

/**
* Save error message so it can be retrieved at later time
*
* @param const char *error_message - pointer to error ASCIIZ text
* 
*/
void USBComPort::set_error_message(const char* error_message) {
	m_error_log_oss << error_message;
}

/**
* Disconnect from SwiftRNG device 
*
* @return true - successful operation
*
*/
bool USBComPort::disconnect() {
	if (!is_connected()) {
		return false;
	}
	CloseHandle(m_cdc_usb_dev_handle);
	m_device_connected = false;
	clear_error_log();
	return true;
}

/**
* Send specified command to SwiftRNG device
* 
* @param const unsigned char *snd - pointer to a command to send
* @param int sizeSnd - the size of command in bytes
* @param int *bytesSend - pointer to store the number of bytes actually sent
* 
* @return 0 - successful operation, otherwise the error code
*
*/
int USBComPort::send_command(const unsigned char *snd, int sizeSnd, int *bytesSend){
	DWORD actualBytesSent;
	int retStatus = -1;
	if (!is_connected()) {
		return retStatus;
	}

	BOOL status = WriteFile(m_cdc_usb_dev_handle, (void *)snd, sizeSnd, &actualBytesSent, 0);
	*bytesSend = (int)actualBytesSent;
	if (!status || *bytesSend != sizeSnd)
	{
		if (status && *bytesSend != sizeSnd) {
			retStatus = -7; // Time out
			set_error_message("Got timeout while sending data to device");
		}
		else {
			set_error_message("Could not send data to device");
		}

		clear_comm_err();
		purge_comm_data();
		return retStatus;
	}
	return 0;
}

/**
* Receive data from SwiftRNG device
*
* @param unsigned char *rcv - a pointer to receive buffer
* @param int sizeRcv - how many bytes to receive
* @param int *bytesReveived -pointer to store the number of bytes actually received
* 
* @return 0 - successful operation, otherwise the error code
*
*/
int USBComPort::receive_data(unsigned char *rcv, int sizeRcv, int *bytesReveived) {
	DWORD actualBytesReceived;
	int retStatus = -1;
	if (!is_connected()) {
		return retStatus;
	}

	BOOL status = ReadFile(m_cdc_usb_dev_handle, (void *)rcv, sizeRcv, &actualBytesReceived, NULL);
	*bytesReveived = (int)actualBytesReceived;
	if (!status || *bytesReveived != sizeRcv) {
		if (status && *bytesReveived != sizeRcv) {
			retStatus = -7; // Time out
			set_error_message("Got timeout while receiving data from the device");
		} else {
			set_error_message("Could not receive data from the device");
		}

		clear_comm_err();
		purge_comm_data();
		return retStatus;
	}
	return 0;
}

/**
* Retrieve the current error log
*
* @return error log as string
*
*/
std::string USBComPort::get_error_log() const {
	return m_error_log_oss.str();
}

/**
* Send a command to SwiftRNG and receive data as a result
* 
* @param const unsigned char *snd - pointer to a command to send
* @param int sizeSnd - the size of command in bytes
* @param unsigned char *rcv - a pointer to receive buffer
* @param int sizeRcv - how many bytes to receive
* @return 0 - successful operation, otherwise the error code
*
*/
int USBComPort::execute_device_cmd(const unsigned char *snd, int sizeSnd, unsigned char *rcv, int sizeRcv) {
	int actualBytesSent;
	int status = send_command(snd, sizeSnd, &actualBytesSent);
	if (status) {
		return status;
	}
	if (sizeRcv == 0) {
		return status;
	}
	int actualBytesReceived;
	return receive_data(rcv, sizeRcv, &actualBytesReceived);
}

/**
* Clear COM port receive and transmit buffers
*/
void USBComPort::purge_comm_data() {
	if (!is_connected()) {
		return;
	}
	PurgeComm(m_cdc_usb_dev_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

/**
* Clear COM port errors
*/
void USBComPort::clear_comm_err() {
	if (!is_connected()) {
		return;
	}
	ClearCommError(m_cdc_usb_dev_handle, &m_comm_error, &m_comm_status);
}

/**
* Scan registry and discover all the SwiftRNG devices attached to COM ports
*
* @param int ports[] - an array of integers that represent all of the SwiftRNG COM ports found 
* @param int maxPorts - the maximum number of SwiftRNG devices to discover
* @param  int *actualCount - a pointer to the actual number of SwiftRNG devices found
* @param  WCHAR *hardwareId - a pointer to SwiftRNG device hardware ID 
* @param WCHAR* serialId - a pointer to SwiftRNG device hardware serial ID
* 
* @return 0 - successful operation, otherwise the error code
*
*/
void USBComPort::get_connected_ports(int ports[], int maxPorts, int *actualCount, WCHAR *hardwareId, WCHAR* serialId) {

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
						if (_tcsnicmp(hardwareId, (TCHAR*)curHardwareId, _tcsnlen(hardwareId, 80)) == 0) {
							int port_nr = _ttoi(curPortName + 3);
							if (port_nr != 0)
							{
								DEVINST dev_instance_parent_id;
								TCHAR sz_dev_instance_id[MAX_DEVICE_ID_LEN];
								CONFIGRET status = CM_Get_Parent(&dev_instance_parent_id, devInfoData.DevInst, 0);
								if (status == CR_SUCCESS)
								{
									status = CM_Get_Device_ID(dev_instance_parent_id, sz_dev_instance_id, MAX_DEVICE_ID_LEN, 0);
									if (status == CR_SUCCESS) {
										if (std::wstring(sz_dev_instance_id).find(serialId) != std::string::npos) {
											ports[foundPortIndex++] = port_nr;
										}
									}
								}
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
* @param  int portNameSize - the max number of characters available for portName
*
*/
void USBComPort::toPortName(int portNum, WCHAR* portName, int portNameSize) {
	swprintf_s(portName, portNameSize, L"\\\\.\\COM%d", portNum);
}

USBComPort::~USBComPort()
{
	if (is_connected()) {
		disconnect();
	}
}
