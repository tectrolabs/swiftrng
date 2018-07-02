/*
 * SwiftRNG.h
 * ver. 1.0
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2016 TectroLabs, http://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for interacting with the hardware random data generator device SwiftRNG for the purpose of
 downloading true random bytes.

 This program requires the libusb-1.0 library when communicating with the SwiftRNG device. Please read the provided
 documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with the SwiftRNG device.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/



#ifndef SWIFTRNG_H_
#define SWIFTRNG_H_

#include <stdio.h>

#ifdef __cplusplus

extern "C" __declspec(dllexport) int swftOpen(int deviceNum);
extern "C" __declspec(dllexport) int swftClose();
extern "C" __declspec(dllexport) int swftGetEntropy(unsigned char *buffer, long length);
extern "C" __declspec(dllexport) int swftGetEntropyByte();
extern "C" __declspec(dllexport) int swftSetPowerProfile(int ppNum);
extern "C" __declspec(dllexport) const char* swftGetLastErrorMessage();
extern "C" __declspec(dllexport) int swftGetModel(char model[9]);
extern "C" __declspec(dllexport) int swftGetVersion(char version[5]);
extern "C" __declspec(dllexport) int swftGetSerialNumber(char serialNumber[16]);

#else

extern __declspec(dllexport) int swftOpen(int deviceNum);
extern __declspec(dllexport) int swftClose();
extern __declspec(dllexport) int swftGetEntropy(unsigned char *buffer, long length);
extern __declspec(dllexport) int swftGetEntropyByte();
extern __declspec(dllexport) int swftSetPowerProfile(int ppNum);
extern __declspec(dllexport) const char* swftGetLastErrorMessage();
extern __declspec(dllexport) int swftGetModel(char model[9]);
extern __declspec(dllexport) int swftGetVersion(char version[5]);
extern __declspec(dllexport) int swftGetSerialNumber(char serialNumber[16]);

#endif

#endif /* SWIFTRNG_H_ */ 