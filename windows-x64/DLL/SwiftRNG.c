/*
 * SwiftRNG.cpp
 * ver. 2.2
 *
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program is used for interacting with the hardware random data generator device SwiftRNG for the purpose of
 downloading true random bytes.

 This program requires the libusb-1.0 library when communicating with the SwiftRNG device. Please read the provided
 documentation for libusb-1.0 installation details.

 This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
 Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

 This program may only be used in conjunction with the SwiftRNG device.

 This DLL was designed to be shared between multiple applications concurrently. To get a secured and dedicated 
 connection to SwiftRNG device use SwiftRNG API library instead.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include <SwiftRNG.h>
#include <swrngapi.h>
#include <windows.h>

//
// SwiftRNG API related data
//

static CRITICAL_SECTION cs;				// For thread synchronization
static HANDLE sharedGlobalMutex;		// For process synchronization

#define MAX_BUFF_REQUEST_SIZE 100000	// Must not be modified
#define NUM_REQUESTS 12					// Should be between 1 and 50
#define CACHE_BUFF_SIZE_BYTES (MAX_BUFF_REQUEST_SIZE) * (NUM_REQUESTS)

static unsigned char cacheBuffer[CACHE_BUFF_SIZE_BYTES + 1];
static uint32_t idx;
static BOOL enablePostProcessing;
static BOOL enableStatisticalTests;

//
// Entropy server API related data
//

typedef struct
{
	DWORD cmd;
	DWORD cbReqData;
} REQCMD;

#define MAX_ES_BUFF_REQUEST_SIZE 100000	// Must not be modified
#define CMD_ENTROPY_RETRIEVE_ID 0
#define PIPENAME_MAX_CHARS 128
static wchar_t pipeEndPoint[PIPENAME_MAX_CHARS + 1];
static char defaultPipeEndpoint[] = "\\\\.\\pipe\\SwiftRNG";
static unsigned char cacheESBuffer[MAX_ES_BUFF_REQUEST_SIZE];
static uint32_t idxEs;
static BOOL fSuccess;


/**
* A process-safe and thread-safe function to set the pipe endpoint to the SwiftRNG entropy server.
* When used, it should be called immediately after loading the DLL in order to override the default endpoint
* and before any other call to the DLL is made.
* @param char *pipeEndpoint - a pointer to an ASCIIZ pipe endpoint
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftSetEntropyServerPipeEndpointSynchronized(char *pipeEndpoint) {
	int status = -1;
	EnterCriticalSection(&cs);
	if (pipeEndpoint != NULL && strlen(pipeEndpoint) <= PIPENAME_MAX_CHARS) {
		mbstowcs(pipeEndPoint, pipeEndpoint, strlen(pipeEndpoint) + 1);
		status = 0;
	}
	LeaveCriticalSection(&cs);
	return status;
}

/**
* Initialize local data
*
*/
static void initializeDLL() {
	idx = CACHE_BUFF_SIZE_BYTES;
	idxEs = MAX_ES_BUFF_REQUEST_SIZE;
	enablePostProcessing = TRUE;
	enableStatisticalTests = TRUE;
	mbstowcs(pipeEndPoint, defaultPipeEndpoint, strlen(defaultPipeEndpoint) + 1);
	fSuccess = FALSE;
}

/**
* Lock the global mutex for library access synchronization
* @return TRUE - when mutex locked successfully, FALSE otherwise
*/
static BOOL lockGlobalMutext() {
	DWORD dwWaitResult = WaitForSingleObject(sharedGlobalMutex, INFINITE);
	switch (dwWaitResult)
	{
		// The thread got Global ownership of the mutex
	case WAIT_OBJECT_0:
		return TRUE;
	case WAIT_ABANDONED:
		return FALSE;
	}
	return FALSE;
}

/**
* Unlock the global mutex
* @return TRUE - when mutex unlocked successfully, FALSE otherwise
*/
static BOOL unlockGlobalMutext() {
	// Release ownership of the mutex object
	if (!ReleaseMutex(sharedGlobalMutex))
	{
		return FALSE;
	}
	return TRUE;
}

/**
* Retrieve random bytes from the first SwiftRNG device and fill up the buffer
* @return 0 - successful operation, otherwise the error code
*/
static int fillEntropyBuffer() {
	int status = -1;
	SwrngContext ctxt;
	swrngInitializeContext(&ctxt);

	if (lockGlobalMutext() == TRUE)
	{
		__try {
			status = swrngOpen(&ctxt, 0);
			if (status == 0) {

				if (enablePostProcessing == FALSE) {
					swrngDisablePostProcessing(&ctxt); // Ignore any error
				}
				if (enableStatisticalTests == TRUE)
					swrngEnableStatisticalTests(&ctxt);
				else
					swrngDisableStatisticalTests(&ctxt);

				for (int i = 0; i < NUM_REQUESTS; i++) {
					unsigned char *p = cacheBuffer + (i * MAX_BUFF_REQUEST_SIZE);
					status = swrngGetEntropy(&ctxt, p, MAX_BUFF_REQUEST_SIZE);
					if (status != SWRNG_SUCCESS) {
						break;
					}
				}
				if (status == SWRNG_SUCCESS) {
					idx = 0;
				}
			}
			swrngDestroyContext(&ctxt);
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/*
* Open named pipe to the entropy server
* @param *hPipe - pipe handle
* @return 0 - success
*/
static int openNamedPipe(HANDLE *hPipe) {
	DWORD dwMode;
	while (1) {
		*hPipe = CreateFile(
			pipeEndPoint,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		if (*hPipe != INVALID_HANDLE_VALUE) {
			break;
		}

		if (GetLastError() != ERROR_PIPE_BUSY) {
			return -1;
		}

		if (!WaitNamedPipe(pipeEndPoint, 25000)) {
			return -1;
		}
	}

	dwMode = PIPE_READMODE_BYTE;
	fSuccess = SetNamedPipeHandleState(
		*hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if (!fSuccess) {
		return -1;
	}
	return 0;
}

/*
* Close named pipe to the entropy server
* @param *hPipe - pipe handle
* @return 0 - success
*/
static int closeNamedPipe(HANDLE *hPipe) {
	CloseHandle(*hPipe);
	return 0;
}

/*
* Read entropy data from named pipe
* @param commandId - command to send to the entropy server
* @param rcvBuff buffer for incoming data
* @param len - number of bytes to receive
* @param *hPipe - pipe handle
* @return 0 - success
*/
static int readEntropyFromPipe(DWORD commandId, unsigned char *rcvBuff, DWORD len, HANDLE *hPipe) {
	REQCMD reqCmd;

	DWORD  cbRead, cbToWrite, cbWritten;
	cbToWrite = sizeof(REQCMD);
	reqCmd.cmd = commandId;
	reqCmd.cbReqData = len;

	fSuccess = WriteFile(
		*hPipe,                  // pipe handle 
		&reqCmd,				// bytes 
		cbToWrite,              // bytes length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess) {
		return -1;
	}
	do {
		fSuccess = ReadFile(
			*hPipe,    // pipe handle 
			rcvBuff,    // buffer to receive reply 
			len,  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 

		if (!fSuccess && GetLastError() != ERROR_MORE_DATA) {
			break;
		}
	} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

	if (!fSuccess) {
		return -1;
	}
	return 0;
}

/**
* Retrieve random bytes from entropy server and fill up the buffer
* @return 0 - successful operation, otherwise the error code
*/
static int fillESEntropyBuffer() {
	int status = -1;
	int openStatus = -1;
	HANDLE hPipe;
	status = openNamedPipe(&hPipe);
	openStatus = status;
	if (status == 0) {
		status = readEntropyFromPipe(CMD_ENTROPY_RETRIEVE_ID, cacheESBuffer, MAX_ES_BUFF_REQUEST_SIZE, &hPipe);
	}
	if (openStatus == 0) {
		closeNamedPipe(&hPipe);
	}
	if (status == 0) {
		idxEs = 0;
	}
	return status;
}

/**
* A process-safe and thread-safe function to retrieve random bytes from SwiftRNG entropy server.
* There should be an entropy server running to successfully call the function.
* @param unsigned char *buffer - a pointer to the data receive buffer
* @param long length - how many bytes expected to receive (must not be greater than 100,000)
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swftGetEntropyFromEntropyServer(unsigned char *buffer, long length) {
	int status = SWRNG_SUCCESS;
	int openStatus = -1;
	HANDLE hPipe;

	if (length > 100000 || length <= 0) {
		return -1;
	}

	status = openNamedPipe(&hPipe);
	openStatus = status;
	if (status == 0) {
		status = readEntropyFromPipe(CMD_ENTROPY_RETRIEVE_ID, buffer, length, &hPipe);
	}
	if (openStatus == 0) {
		closeNamedPipe(&hPipe);
	}
	return status;
}

/**
*
* A process-safe and thread-safe function to retrieve a random byte from wiftRNG entropy server.
* There should be an entropy server running to successfully call the function.
* @return random byte value between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
__declspec(dllexport) int swftGetByteFromEntropyServerSynchronized() {
	int retVal = SWRNG_SUCCESS;
	EnterCriticalSection(&cs);

	if (idxEs >= MAX_ES_BUFF_REQUEST_SIZE) {
		retVal = fillESEntropyBuffer();
	}
	if (retVal == 0) {
		retVal = cacheESBuffer[idxEs++];
	}
	else {
		retVal = 256;
	}
	LeaveCriticalSection(&cs);
	return retVal;
}

/**
*
* A process-safe and thread-safe function to retrieve a random byte from the SwiftRNG device.
* @return random byte value between 0 and 255 (a value of 256 or greater will indicate an error)
*
*/
int swftGetEntropyByteSynchronized() {
	int retVal = SWRNG_SUCCESS;
	EnterCriticalSection(&cs);

	if (idx >= CACHE_BUFF_SIZE_BYTES) {
		retVal = fillEntropyBuffer();
	}
	if (retVal == SWRNG_SUCCESS) {
		retVal = cacheBuffer[idx++];
	}
	else {
		retVal = 256;
	}
	LeaveCriticalSection(&cs);
	return retVal;
}

/**
* DLL main entry routine
*
* @param HINSTANCE hinstDLL - handle to DLL module
* @param DWORD fdwReason - reason for calling function
* @param LPVOID lpReserved - reserved
*/
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, PVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		initializeDLL();
		InitializeCriticalSection(&cs);
		sharedGlobalMutex = CreateMutex(NULL, FALSE, TEXT("Global\SwiftRNG"));
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup when detaching the process.
		CloseHandle(sharedGlobalMutex);
		DeleteCriticalSection(&cs);
		break;
	}
	return TRUE;
}

/**
* A process-safe and thread-safe function to retrieve random bytes from the SwiftRNG device
* @param unsigned char *buffer - a pointer to the data receive buffer (must hold length + 1 bytes)
* @param long length - how many bytes expected to receive (must not be greater than 10,000,000)
* @return 0 - successful operation, otherwise the error code
*
*/
int swftGetEntropySynchronized(unsigned char *buffer, long length) {
	int status = SWRNG_SUCCESS;

	if (length > 10000000) {
		return -1;
	}

	for (long l = 0; l < length; l++) {
		int retVal = swftGetEntropyByteSynchronized();
		if (retVal > 255) {
			status = -1;
			break;
		}
		buffer[l] = retVal;
	}
	return status;
}

/**
* A process-safe and thread-safe function for setting device power profile
* @param int ppNum - power profile number (between 0 and 9)
* @return 0 - successful operation, otherwise the error code
*
*/
int swftSetPowerProfileSynchronized(int ppNum) {
	int status = -1;
	SwrngContext ctxt;
	swrngInitializeContext(&ctxt);


	if (lockGlobalMutext() == TRUE)
	{
		__try {
			status = swrngOpen(&ctxt, 0);
			if (status == 0) {
				status = swrngSetPowerProfile(&ctxt, ppNum);
			}
			swrngDestroyContext(&ctxt);
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/**
* A process-safe and thread-safe function to retrieve SwiftRNG device model number.
* @param char model[9] - pointer to a 9 bytes char array for holding SwiftRNG device model number
* @return 0 - successful operation, otherwise the error code
*
*/
int swftGetModelSynchronized(char model[9]) {
	int status = -1;
	SwrngContext ctxt;
	swrngInitializeContext(&ctxt);

	DeviceModel deviceModel;
	if (lockGlobalMutext() == TRUE)
	{
		__try {
			status = swrngOpen(&ctxt, 0);
			if (status == 0) {
				status = swrngGetModel(&ctxt, &deviceModel);
				if (status == 0) {
					strcpy(model, deviceModel.value);
				}
			}
			swrngDestroyContext(&ctxt);
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/**
* A process-safe and thread-safe function to retrieve SwiftRNG device version.
* @param char version[5] - pointer to a 5 bytes array for holding SwiftRNG device version
* @return 0 - successful operation, otherwise the error code
*
*/
int swftGetVersionSynchronized(char version[5]) {
	int status = -1;
	SwrngContext ctxt;
	swrngInitializeContext(&ctxt);

	DeviceVersion deviceVersion;
	if (lockGlobalMutext() == TRUE)
	{
		__try {
			status = swrngOpen(&ctxt, 0);
			if (status == 0) {
				status = swrngGetVersion(&ctxt, &deviceVersion);
				if (status == 0) {
					strcpy(version, deviceVersion.value);
				}
			}
			swrngDestroyContext(&ctxt);
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/**
* A process-safe and thread-safe function to retrieve SwiftRNG device serial number.
* @param char serialNumber[16] - pointer to a 16 bytes array for holding SwiftRNG device S/N
* @return 0 - successful operation, otherwise the error code
*
*/
int swftGetSerialNumberSynchronized(char serialNumber[16]) {
	int status = -1;
	SwrngContext ctxt;
	swrngInitializeContext(&ctxt);

	DeviceSerialNumber deviceSerNum;
	if (lockGlobalMutext() == TRUE)
	{
		__try {
			status = swrngOpen(&ctxt, 0);
			if (status == 0) {
				status = swrngGetSerialNumber(&ctxt, &deviceSerNum);
				if (status == 0) {
					strcpy(serialNumber, deviceSerNum.value);
				}
			}
			swrngDestroyContext(&ctxt);
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/**
* A process-safe and thread-safe function to retrieve 16,000 of RAW random bytes from a noise source of the SwiftRNG device.
* No data alteration, verification or quality tests will be performed when calling this function.
* It will retrieve 16,000 raw bytes of the sampled random data from one of the noise sources.
* It can be used for inspecting the quality of the noise sources and data acquisition components.
*
* @param unsigned char rawBytes[16000] - pointer to a 16,000 bytes array for holding RAW random bytes
* @param int noiseSourceNum - noise source number (0 - first noise source, 1 - second one)
* @return 0 - successful operation, otherwise the error code
*
*/
__declspec(dllexport) int swrngGetRawDataBlockSynchronized(unsigned char rawBytes[16000], int noiseSourceNum) {
	int status = -1;
	SwrngContext ctxt;
	swrngInitializeContext(&ctxt);

	NoiseSourceRawData noiseSourceRawData;
	if (lockGlobalMutext() == TRUE)
	{
		__try {
			status = swrngOpen(&ctxt, 0);
			if (status == 0) {
				status = swrngGetRawDataBlock(&ctxt, &noiseSourceRawData, noiseSourceNum);
				if (status == 0) {
					memcpy(rawBytes, noiseSourceRawData.value, 16000);
				}
			}
			swrngDestroyContext(&ctxt);
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/**
* Enable or disable post processing, Applies only to devices V1.2 and up.
* @param postProcessingFlag - true to enable post processing, false to disable
* @return 0 - successful operation, otherwise the error code*
*
*/
int swrngSetPostProcessing(BOOL postProcessingFlag) {
	int status = -1;
	if (lockGlobalMutext() == TRUE)
	{
		__try {
			enablePostProcessing = postProcessingFlag;
			status = 0;
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/**
* Enable or disable statistical tests.
* @param statisticalTestsFlag - true to enable statistical tests, false to disable
* @return 0 - successful operation, otherwise the error code*
*
*/
int swrngSetStatisticalTestsStatus(BOOL statisticalTestsFlag) {
	int status = -1;
	if (lockGlobalMutext() == TRUE)
	{
		__try {
			enableStatisticalTests = statisticalTestsFlag;
			status = 0;
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/**
* A process-safe and thread-safe function for enabling post processing of raw random data regardless of the device version.
* For devices with versions 1.2 and up the post processing can be disabled after device is open.
*
* To enable post processing, call this function any time
*
* @return int - 0 when post processing was successfully enabled, otherwise the error code
*
*/
__declspec(dllexport) int swrngEnableDataPostProcessing() {
	return swrngSetPostProcessing(TRUE);
}

/**
* A process-safe and thread-safe function for disabling post processing of raw random data.
* It takes effect only for devices with versions 1.2 and up.
*
* To disable post processing, call this function any time
*
* @return int - 0 when post processing was successfully disabled, otherwise the error code
*
*/
__declspec(dllexport) int swrngDisableDataPostProcessing() {
	return swrngSetPostProcessing(FALSE);
}

/**
* Check to see if raw data post processing is enabled for device.
*
* @return int - 1 when post processing is enabled, 0 if disabled, negative number if error
*/
__declspec(dllexport) int swrngGetDataPostProcessingStatus() {
	int status = -1;
	if (lockGlobalMutext() == TRUE)
	{
		__try {
			status = enablePostProcessing == TRUE ? 1 : 0;
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}

/**
* A process-safe and thread-safe function for disabling statistical tests for raw random data.
*
* @return int - 0 when statistical tests were successfully disabled, otherwise the error code
*
*/
__declspec(dllexport) int swrngDisableDataStatisticalTests() {
	return swrngSetStatisticalTestsStatus(FALSE);
}

/**
* A process-safe and thread-safe function for enabling statistical tests for raw random data.
*
* @return int - 0 when statistical tests were successfully enabled, otherwise the error code
*
*/
__declspec(dllexport) int swrngEnableDataStatisticalTests() {
	return swrngSetStatisticalTestsStatus(TRUE);
}

/**
* Check to see if statistical tests are enabled for device.
*
* @return int - 1 when statistical tests are enabled, 0 if disabled, negative number if error
*/
__declspec(dllexport) int swrngGetDataStatisticalTestsStatus() {
	int status = -1;
	if (lockGlobalMutext() == TRUE)
	{
		__try {
			status = enableStatisticalTests == TRUE ? 1 : 0;
		}
		__finally {
			// Release ownership of the mutex object
			if (unlockGlobalMutext() == FALSE)
			{
				status = -1;
			}
		}
	}
	return status;
}




