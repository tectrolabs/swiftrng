/*
* USBComPort.h
* Ver 1.4
*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

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
#include <cfgmgr32.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <combaseapi.h>
#include <sstream>

class USBComPort
{
public:
	USBComPort();
	~USBComPort();
	void initialize();
	bool is_connected();
	bool connect(WCHAR *comPort);
	bool disconnect();
	std::string get_error_log() const;
	void clear_error_log();
	int execute_device_cmd(const unsigned char *snd, int sizeSnd, unsigned char *rcv, int sizeRcv);
	void get_connected_ports(int ports[], int maxPorts, int* actualCount, WCHAR *hardwareId, WCHAR* serialId);
	void toPortName(int portNum, WCHAR* portName, int portNameSize);
	int send_command(const unsigned char *snd, int sizeSnd, int *bytesSend);
	int receive_data(unsigned char *rcv, int sizeRcv, int *bytesReveived);

private:
	void set_error_message(const char* error_message);
	void purge_comm_data();
	void clear_comm_err();

private:
	HANDLE m_cdc_usb_dev_handle;
	bool m_device_connected{ false };
	COMSTAT m_comm_status;
	DWORD m_comm_error;
	std::ostringstream m_error_log_oss;
};

#endif /* SWRNGUSBCOMPORT_H_ */