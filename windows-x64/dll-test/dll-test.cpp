#include "stdafx.h"

/*
* dll-test.cpp
* ver. 1.6
*
*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (C) 2014-2020 TectroLabs, http://tectrolabs.com

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

#include "stdafx.h"

#include "SwiftRNG.h"
#include <windows.h>
#include <process.h>
#include <stdint.h>

#define RNG_BUFF_SIZE_BYTES 10000
#define EAW_RNG_BUFF_SIZE_BYTES 16000
#define NUM_TEST_THREADS 50
char errBuff[256];
char model[9];
char version[5];
char sn[16];
HANDLE threads[NUM_TEST_THREADS];

unsigned char entropyBytes[RNG_BUFF_SIZE_BYTES + 1];
unsigned char rawNoiseSourceBytes[EAW_RNG_BUFF_SIZE_BYTES];

HINSTANCE hDLL; // Handle to the DLL

typedef int (CALLBACK* LPGEBS1)();
typedef int (CALLBACK* LPGEBS2)(unsigned char *, long);
typedef int (CALLBACK* LPSPP)(int);
typedef int (CALLBACK* LPGETMODEL)(char *);
typedef int (CALLBACK* LPGETVER)(char *);
typedef int (CALLBACK* LPGETSN)(char *);
typedef int (CALLBACK* LPGETRAW)(unsigned char *, int);
typedef int (CALLBACK* LPENABLEDPP)();
typedef int (CALLBACK* LPDISABLEDPP)();
typedef int (CALLBACK* LPGETPPSTATUS)();
typedef int (CALLBACK* LPENABLEST)();
typedef int (CALLBACK* LPDISABLEST)();
typedef int (CALLBACK* LPGETSTSTATUS)();


LPGEBS1 lpGetEntropyByteSynchronized;
LPGEBS2 lpSwftGetEntropySynchronized;
LPSPP lpSwftSetPowerProfileSynchronized;
LPGETMODEL lpSwftGetModelSynchronized;
LPGETVER lpSwftGetVersionSynchronized;
LPGETSN lpSwftGetSerialNumberSynchronized;
LPGETRAW lpSwftGetRawDataBlockSynchronized;
LPENABLEDPP lpSwrngEnableDataPostProcessing;
LPDISABLEDPP lpSwrngDisableDataPostProcessing;
LPGETPPSTATUS lpSwrngGetDataPostProcessingStatus;
LPENABLEST lpSwrngEnableDataStatisticalTests;
LPDISABLEST lpSwrngDisableDataStatisticalTests;
LPGETSTSTATUS lpSwrngGetDataStatisticalTestsStatus;


int status;
volatile BOOL isMultiThreadTestStatusSuccess = TRUE;

//
// Testing thread 
//
unsigned int __stdcall testingThread(void*) {
	LPGEBS2 lpFunc;
	int testStatus;
	unsigned char testEntropyBytes[RNG_BUFF_SIZE_BYTES + 1];
	
	HINSTANCE tDLL = LoadLibrary(L"SwiftRNG.dll");
	if (tDLL == NULL)
	{
		isMultiThreadTestStatusSuccess = FALSE;
		return 0;
	}

	lpFunc = (LPGEBS2)GetProcAddress(tDLL, "swftGetEntropySynchronized");
	if (!lpFunc)
	{
		isMultiThreadTestStatusSuccess = FALSE;
		FreeLibrary(tDLL);
		return 0;
	}
	for (int i = 0; i < 5; i++) {
		testStatus = lpFunc(testEntropyBytes, (long)RNG_BUFF_SIZE_BYTES);
		if (testStatus != 0) {
			isMultiThreadTestStatusSuccess = FALSE;
			FreeLibrary(tDLL);
			return 0;
		}
	}
	FreeLibrary(tDLL);
	return 0;
}

//
// Main entry
//
int main() {
	printf("--------------------------------------------------------------------------\n");
	printf("--- A program utility for testing the SwiftRNG.dll with SwiftRNG device --\n");
	printf("----- Make sure no other process is using SwiftRNG device when running ---\n");
	printf("--------------------------------------------------------------------------\n");

	printf("\n");

	//
	// Step 1
	//
	printf("Loading SwiftRNG.dll ............................................. ");
	hDLL = LoadLibrary(L"SwiftRNG.dll");
	if (hDLL == NULL)
	{
		printf(" failed\n");
		return -1;
	}
	else {
		printf("SUCCESS\n");
	}

	lpSwrngEnableDataPostProcessing = (LPENABLEDPP)GetProcAddress(hDLL, "swrngEnableDataPostProcessing");
	if (!lpSwrngEnableDataPostProcessing)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}

	lpSwrngDisableDataPostProcessing = (LPDISABLEDPP)GetProcAddress(hDLL, "swrngDisableDataPostProcessing");
	if (!lpSwrngDisableDataPostProcessing)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}

	lpSwrngGetDataPostProcessingStatus = (LPGETPPSTATUS)GetProcAddress(hDLL, "swrngGetDataPostProcessingStatus");
	if (!lpSwrngGetDataPostProcessingStatus)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}

	int case1 = lpSwrngGetDataPostProcessingStatus();
	lpSwrngDisableDataPostProcessing();
	int case2 = lpSwrngGetDataPostProcessingStatus();
	lpSwrngEnableDataPostProcessing();
	int case3 = lpSwrngGetDataPostProcessingStatus();

	if (case1 != 1 || case2 != 0 || case3 != 1) {
		printf("post processing status failed\n");
		FreeLibrary(hDLL);
		return -1;
	}

	//
	// Step 2
	//
	printf("Testing swftGetEntropyByteSynchronized() ......................... ");
	lpGetEntropyByteSynchronized = (LPGEBS1)GetProcAddress(hDLL, "swftGetEntropyByteSynchronized");
	if (!lpGetEntropyByteSynchronized)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	for (uint64_t i = 0; i < 10000000; i++) {
		status = lpGetEntropyByteSynchronized();
		if (status > 255) {
			printf("FAILED");
			FreeLibrary(hDLL);
			return -1;
		}
	}
	printf("SUCCESS\n");

	//
	// Step 3
	//
	printf("Testing swftGetEntropySynchronized() ............................. ");
	lpSwftGetEntropySynchronized = (LPGEBS2)GetProcAddress(hDLL, "swftGetEntropySynchronized");
	if (!lpSwftGetEntropySynchronized)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	for (int i = 0; i < 10; i++) {
		status = lpSwftGetEntropySynchronized(entropyBytes, (long)RNG_BUFF_SIZE_BYTES);
		if (status != 0) {
			printf("FAILED");
			FreeLibrary(hDLL);
			return -1;
		}
	}
	printf("SUCCESS\n");

	//
	// Step 4
	//
	printf("Testing swftSetPowerProfileSynchronized() ........................ ");
	lpSwftSetPowerProfileSynchronized = (LPSPP)GetProcAddress(hDLL, "swftSetPowerProfileSynchronized");
	if (!lpSwftSetPowerProfileSynchronized)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}

	status = lpSwftSetPowerProfileSynchronized(10);
	if (status == 0) {
		printf("FAILED");
		FreeLibrary(hDLL);
		return -1;
	}
	printf("SUCCESS\n");


	//
	// Step 5
	//
	printf("Testing swftGetModelSynchronized() ............................... ");
	lpSwftGetModelSynchronized = (LPGETMODEL)GetProcAddress(hDLL, "swftGetModelSynchronized");
	if (!lpSwftGetModelSynchronized)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	status = lpSwftGetModelSynchronized(model);
	if (status != 0) {
		printf("FAILED");
		FreeLibrary(hDLL);
		return -1;
	}
	printf("SUCCESS (%s)\n", model);

	//
	// Step 6
	//
	printf("Testing swftGetVersionSynchronized() ............................. ");
	lpSwftGetVersionSynchronized = (LPGETMODEL)GetProcAddress(hDLL, "swftGetVersionSynchronized");
	if (!lpSwftGetVersionSynchronized)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	status = lpSwftGetVersionSynchronized(version);
	if (status != 0) {
		printf("FAILED");
		FreeLibrary(hDLL);
		return -1;
	}
	printf("SUCCESS (%s)\n", version);

	//
	// Step 7
	//
	printf("Testing swftGetSerialNumberSynchronized() ........................ ");
	lpSwftGetSerialNumberSynchronized = (LPGETSN)GetProcAddress(hDLL, "swftGetSerialNumberSynchronized");
	if (!lpSwftGetSerialNumberSynchronized)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	status = lpSwftGetSerialNumberSynchronized(sn);
	if (status != 0) {
		printf("FAILED");
		FreeLibrary(hDLL);
		return -1;
	}
	printf("SUCCESS (%s)\n", sn);

	//
	// Step 8
	//
	printf("Testing swrngGetRawDataBlockSynchronized (V1.2 only) ............. ");
	lpSwftGetRawDataBlockSynchronized = (LPGETRAW)GetProcAddress(hDLL, "swrngGetRawDataBlockSynchronized");
	if (!lpSwftGetRawDataBlockSynchronized)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	status = lpSwftGetRawDataBlockSynchronized(rawNoiseSourceBytes, 0);
	if (status != 0) {
		printf("FAILED\n");
	}
	else {
		status = lpSwftGetRawDataBlockSynchronized(rawNoiseSourceBytes, 1);
		if (status != 0) {
			printf("FAILED");
			FreeLibrary(hDLL);
			return -1;
		}
		printf("SUCCESS\n");
	}

	//
	// Step 9
	//
	printf("Testing lpSwrngEnableDataStatisticalTests ........................ ");
	lpSwrngEnableDataStatisticalTests = (LPENABLEST)GetProcAddress(hDLL, "swrngEnableDataStatisticalTests");
	if (!lpSwrngEnableDataStatisticalTests)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	status = lpSwrngEnableDataStatisticalTests();
	if (status != 0) {
		printf("FAILED");
		FreeLibrary(hDLL);
		return -1;
	}
	printf("SUCCESS\n");

	//
	// Step 10
	//
	printf("Testing lpSwrngDisableDataStatisticatTests ....................... ");
	lpSwrngDisableDataStatisticalTests = (LPENABLEST)GetProcAddress(hDLL, "swrngDisableDataStatisticalTests");
	if (!lpSwrngDisableDataStatisticalTests)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	status = lpSwrngDisableDataStatisticalTests();
	if (status != 0) {
		printf("FAILED");
		FreeLibrary(hDLL);
		return -1;
	}
	printf("SUCCESS\n");

	//
	// Step 11
	// 
	printf("Testing lpSwrngGetDataStatisticalTestsStatus ..................... ");
	lpSwrngGetDataStatisticalTestsStatus = (LPENABLEST)GetProcAddress(hDLL, "swrngGetDataStatisticalTestsStatus");
	if (!lpSwrngGetDataStatisticalTestsStatus)
	{
		printf("failed to get address\n");
		FreeLibrary(hDLL);
		return -1;
	}
	status = lpSwrngGetDataStatisticalTestsStatus();
	if (status != 0) {
		printf("FAILED");
		FreeLibrary(hDLL);
		return -1;
	}
	printf("SUCCESS\n");

	//
	// Step 12
	//
	printf("Testing swftGetEntropySynchronized() %3d threads ................. ", NUM_TEST_THREADS);

	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		threads[i] = (HANDLE)_beginthreadex(0, 0, &testingThread, (void*)0, 0, 0);
	}

	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		WaitForSingleObject(threads[i], INFINITE);
	}

	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		CloseHandle(threads[i]);
	}

	if (isMultiThreadTestStatusSuccess == TRUE) {
		printf("SUCCESS\n");
	}
	else {
		printf("FAILED\n");
	}

	FreeLibrary(hDLL);
	return (0);

}

