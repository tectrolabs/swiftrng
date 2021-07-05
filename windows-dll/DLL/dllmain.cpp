/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This DLL may only be used in conjunction with TectroLabs devices.

 This DLL is used for interacting with the entropy server using named pipes.

 */

 /**
  *    @file dllmain.h
  *    @date 07/04/2021
  *    @Author: Andrian Belinski
  *    @version 1.0
  *
  *    @brief DLL implementation for interacting with the entropy server using named pipes.
  */

#include "pch.h"

#ifdef SWIFTRNG_ENTROPY_SERVER
static char defaultPipeEndpoint[] = "\\\\.\\pipe\\SwiftRNG";
#else
static char defaultPipeEndpoint[] = "\\\\.\\pipe\\AlphaRNG";
#endif

/*
* DLL main entry routine
*
* @param hModule - DLL module handle 
* @param ul_reason_for_call - calling reason 
* @param lpReserved - reserved
*/
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	return TRUE;
}

/*
* A function for retrieving entropy bytes from the entropy server.
* There should be an entropy server running to successfully call the function.
*
* @param[out] buffer a pointer to the data receive buffer
* @param[in] length how many bytes expected to receive (must not be greater than 100,000)
* @return 0 successful operation, otherwise the error code
*
*/
__declspec(dllexport) int getEntropy(unsigned char* buffer, long length) {
	EntropyServerConnector pipe(defaultPipeEndpoint);
	if (!pipe.open_named_pipe()) {
		return -1;
	}
	if (!pipe.get_entropy(buffer, length)) {
		return -1;
	}
	return 0;
}

/*
* A function for retrieving raw random bytes from the device noise source using entropy server.
* There should be an entropy server running to successfully call the function.
*
* @param[out] buffer a pointer to the data receive buffer
* @param[in] length how many bytes expected to receive (must not be greater than 100,000)
* @param[in] noise_source noise source: either 1 or 2
* @return 0 successful operation, otherwise the error code
*
*/
__declspec(dllexport) int getNoise(unsigned char* buffer, long length, int noise_source) {
	EntropyServerConnector pipe(defaultPipeEndpoint);
	if (!pipe.open_named_pipe()) {
		return -1;
	}

	if (noise_source != 1 && noise_source != 2) {
		return -1;
	}

	bool status = false;

	switch (noise_source) {
	case 1:
		status = pipe.get_noise_source_1(buffer, length);
		break;
	case 2:
		status = pipe.get_noise_source_2(buffer, length);
		break;
	}

	return status ? 0 : -1;
}

/*
*
* A function for retrieving an entropy byte from the entropy server.
* There should be an entropy server running to successfully call the function.
*
* @return random byte value between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
__declspec(dllexport) int getEntropyAsByte() {
	unsigned char value;
	EntropyServerConnector pipe(defaultPipeEndpoint);

	if (!pipe.open_named_pipe()) {
		return 256;
	}
	if (!pipe.get_entropy(&value, 1)) {
		return 256;
	}
	return value;
}

/*
*
* A function for retrieving the device identifier from the entropy server.
* There should be an entropy server running to successfully call the function.
*
* @param[out] identifier a pointer to an array of 15 chars for device identifier (a.k.a. serial number)
* @return 0 successful operation, otherwise the error code
*
*/
__declspec(dllexport) int getDeviceIdentifier(char* identifier) {
	EntropyServerConnector pipe(defaultPipeEndpoint);

	if (!pipe.open_named_pipe()) {
		return -1;
	}
	string serial_number;
	if (!pipe.get_device_serial_number(serial_number)) {
		return -1;
	}
	memcpy(identifier, serial_number.c_str(), serial_number.size());
	return 0;
}

/*
*
* A function for retrieving the device model from the entropy server.
* There should be an entropy server running to successfully call the function.
*
* @param[out] model a pointer to an array of 15 chars for device model
* @return 0 successful operation, otherwise the error code
*
*/
__declspec(dllexport) int getDeviceModel(char* model) {
	EntropyServerConnector pipe(defaultPipeEndpoint);

	if (!pipe.open_named_pipe()) {
		return -1;
	}
	string device_model;
	if (!pipe.get_device_model(device_model)) {
		return -1;
	}
	memcpy(model, device_model.c_str(), device_model.size());
	return 0;
}

/*
*
* A function for retrieving the device major version number from the entropy server.
* There should be an entropy server running to successfully call the function.
*
* @return major version number between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
__declspec(dllexport) int getDeviceMajorVersion() {
	EntropyServerConnector pipe(defaultPipeEndpoint);

	if (!pipe.open_named_pipe()) {
		return 256;
	}
	int device_major_version;
	if (!pipe.get_device_major_version(device_major_version)) {
		return 256;
	}
	return device_major_version;
}

/*
*
* A function for retrieving the device minor version number from the entropy server.
* There should be an entropy server running to successfully call the function.
*
* @return minor version number between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
__declspec(dllexport) int getDeviceMinorVersion() {
	EntropyServerConnector pipe(defaultPipeEndpoint);

	if (!pipe.open_named_pipe()) {
		return 256;
	}
	int device_minor_version;
	if (!pipe.get_device_minor_version(device_minor_version)) {
		return 256;
	}
	return device_minor_version;
}

/*
*
* A function for retrieving the major version number of the entropy server
* There should be an entropy server running to successfully call the function.
*
* @return major version number between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
__declspec(dllexport) int getServerMajorVersion() {
	EntropyServerConnector pipe(defaultPipeEndpoint);

	if (!pipe.open_named_pipe()) {
		return 256;
	}
	int server_major_version;
	if (!pipe.get_server_major_version(server_major_version)) {
		return 256;
	}
	return server_major_version;
}

/*
*
* A function for retrieving the minor version number of the entropy server
* There should be an entropy server running to successfully call the function.
*
* @return major version number between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
__declspec(dllexport) int getServerMinorVersion() {
	EntropyServerConnector pipe(defaultPipeEndpoint);

	if (!pipe.open_named_pipe()) {
		return 256;
	}
	int server_minor_version;
	if (!pipe.get_server_minor_version(server_minor_version)) {
		return 256;
	}
	return server_minor_version;
}
