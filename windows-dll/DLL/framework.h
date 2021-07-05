/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This DLL may only be used in conjunction with TectroLabs devices.

 This DLL is used for interacting with the entropy server using named pipes.

 */

 /**
  *    @file framework.h
  *    @date 07/04/2021
  *    @Author: Andrian Belinski
  *    @version 1.0
  *
  *    @brief DLL definition for interacting with the entropy server using named pipes.
  */
#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <EntropyServerConnector.h>

using namespace entropy::server::api;

#ifdef __cplusplus
extern "C" {
#endif


	/*
	* A function for retrieving entropy bytes from the entropy server.
	* There should be an entropy server running to successfully call the function.
	*
	* @param[out] buffer a pointer to the data receive buffer
	* @param[in] length how many bytes expected to receive (must not be greater than 100,000)
	* @return 0 successful operation, otherwise the error code
	*
	*/
	__declspec(dllexport) int getEntropy(unsigned char* buffer, long length);

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
	__declspec(dllexport) int getNoise(unsigned char* buffer, long length, int noise_source);

	/*
	*
	* A function for retrieving an entropy byte from the entropy server.
	* There should be an entropy server running to successfully call the function.
	*
	* @return random byte value between 0 and 255 (a value of 256 or greater will indicate an error)
	*
	*/
	__declspec(dllexport) int getEntropyAsByte();

	/*
	*
	* A function for retrieving the device identifier from the entropy server.
	* There should be an entropy server running to successfully call the function.
	*
	* @param[out] identifier a pointer to an array of 15 chars for device identifier (a.k.a. serial number)
	* @return 0 successful operation, otherwise the error code
	*
	*/
	__declspec(dllexport) int getDeviceIdentifier(char* identifier);

	/*
	*
	* A function for retrieving the device model from the entropy server.
	* There should be an entropy server running to successfully call the function.
	*
	* @param[out] model a pointer to an array of 15 chars for device model
	* @return 0 successful operation, otherwise the error code
	*
	*/
	__declspec(dllexport) int getDeviceModel(char* model);

	/*
	*
	* A function for retrieving the device major version number from the entropy server.
	* There should be an entropy server running to successfully call the function.
	*
	* @return major version number between 0 and 255 (a value of 256 or greater will indicate an error)
	*
	*/
	__declspec(dllexport) int getDeviceMajorVersion();

	/*
	*
	* A function for retrieving the device minor version number from the entropy server.
	* There should be an entropy server running to successfully call the function.
	*
	* @return minor version number between 0 and 255 (a value of 256 or greater will indicate an error)
	*
	*/
	__declspec(dllexport) int getDeviceMinorVersion();

	/*
	*
	* A function for retrieving the major version number of the entropy server
	* There should be an entropy server running to successfully call the function.
	*
	* @return major version number between 0 and 255 (a value of 256 or greater will indicate an error)
	*
	*/
	__declspec(dllexport) int getServerMajorVersion();

	/*
	*
	* A function for retrieving the minor version number of the entropy server
	* There should be an entropy server running to successfully call the function.
	*
	* @return major version number between 0 and 255 (a value of 256 or greater will indicate an error)
	*
	*/
	__declspec(dllexport) int getServerMinorVersion();

#ifdef __cplusplus
}
#endif