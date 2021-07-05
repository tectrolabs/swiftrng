#include "stdafx.h"

/*
* entropy-cl-server.cpp
* Ver. 2.0
*
*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (C) 2014-2020 TectroLabs, http://tectrolabs.com

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

This program is used for interacting with a cluster of hardware random data generator device SwiftRNG for 
purpose of downloading and distributing true random bytes using named pipes.

This program requires the libusb-1.0 library and the DLL when communicating with any SwiftRNG device.
Please read the provided documentation for libusb-1.0 installation details.

This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

This program may only be used in conjunction with SwiftRNG devices.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "entropy-cl-server.h"


/**
* Display usage message
*
*/
void displayUsage() {
	printf("*********************************************************************************\n");
	printf("                   SwiftRNG entropy-cl-server Ver 2.0  \n");
	printf("*********************************************************************************\n");
	printf("NAME\n");
	printf("     entropy-cl-server - An application server for distributing random bytes \n");
	printf("              downloaded from a cluster of SwiftRNG devices \n");
	printf("SYNOPSIS\n");
	printf("     entropy-cl-server <options>\n");
	printf("\n");
	printf("DESCRIPTION\n");
	printf("     entropy-cl-server downloads random bytes from two more Hardware (True) \n");
	printf("     Random Number Generator SwiftRNG devices and distributes them to \n");
	printf("     consumer applications using a named pipe.\n");
	printf("\n");
	printf("OPTIONS\n");
	printf("     Operation modifiers:\n");
	printf("\n");
	printf("     -cs NUMBER, --cluster-size NUMBER\n");
	printf("           Preferred number (between 1 and 10) of devices in a cluster.\n");
	printf("           Default value is 2\n");
	printf("\n");
	printf("     -pi NUMBER, --pipe-instances NUMBER\n");
	printf("          How many pipe instances to create (default: %d)\n", DEFAULT_PIPE_INSTANCES);
	printf("          It also defines how many concurrent requests the server can handle\n");
	printf("          Valid values are integers from 1 to %d \n", MAX_PIPE_INSTANCES);
	printf("\n");
	printf("     -ppn NUMBER, --power-profile-number NUMBER\n");
	printf("           Device power profile NUMBER, 0 (lowest) to 9 (highest - default)\n");
	printf("\n");
	printf("     -ppm METHOD, --post-processing-method METHOD\n");
	printf("           SwiftRNG post processing method: SHA256, SHA512 or xorshift64\n");
	printf("           Skip this option for using default method\n");
	printf("\n");
	printf("     -dpp, --disable-post-processing\n");
	printf("           Disable post processing of random data for devices with version 1.2+\n");
	printf("\n");
	printf("     -dst, --disable-statistical-tests\n");
	printf("           Disable 'Repetition Count' and 'Adaptive Proportion' tests.\n");
	printf("\n");
	printf("     -npe, ENDPOINT, --named-pipe-endpoint ENDPOINT\n");
	printf("           Use custom named pipe endpoint (if different from the default endpoint)\n");
	printf("\n");
	printf("EXAMPLES:\n");
	printf("     To start the server using two SwiftRNG devices:\n");
	printf("           entropy-server -cs 2\n");
	printf("     To start the server with post processing disabled for distributing RAW device data:\n");
	printf("           entropy-server -cs 2 -dpp\n");
	printf("     To start the server using a custom pipe endpoint name:\n");
	printf("           entropy-server -cs 2 -npe \\\\.\\pipe\\mycustompipename \n");
	printf("\n");
}

/**
* Main entry
*
* @param int argc - number of parameters
* @param char ** argv - parameters
*
*/
int main(int argc, char **argv) {
	return process(argc, argv);
}

/**
* Process command line arguments
*
* @param int argc - number of parameters
* @param char ** argv - parameters
* @return int - 0 when run successfully
*/
int process(int argc, char **argv) {
	// Initialize SwiftRNG context
	int status = swrngInitializeCLContext(&ctxt);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Could not initialize context\n");
		return(status);
	}

	swrngEnableCLPrintingErrorMessages(&ctxt);
	if (argc == 1) {
		displayUsage();
		return -1;
	}
	mbstowcs_s(&numCharConverted, pipeEndPoint, defaultPipeEndpoint, strlen(defaultPipeEndpoint) + 1);
	return processArguments(argc, argv);
}

/**
* Parse command line parameters
*
* @param int argc
* @param char** argv
* @return int - 0 when run successfully
*/
int processArguments(int argc, char **argv) {
	int idx = 1;
	while (idx < argc) {
		if (strcmp("-dpp", argv[idx]) == 0 || strcmp("--disable-post-processing", argv[idx]) == 0) {
			idx++;
			postProcessingEnabled = SWRNG_FALSE;
		}
		else if (strcmp("-dst", argv[idx]) == 0 || strcmp("--disable-statistical-tests", argv[idx]) == 0) {
			idx++;
			statisticalTestsEnabled = SWRNG_FALSE;
		}
		else if (strcmp("-ppm", argv[idx]) == 0 || strcmp("--post-processing-method",
			argv[idx]) == 0) {
			if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
				return -1;
			}
			postProcessingMethod = argv[idx++];
			if (strcmp("SHA256", postProcessingMethod) == 0) {
				postProcessingMethodId = 0;
			}
			else if (strcmp("SHA512", postProcessingMethod) == 0) {
				postProcessingMethodId = 2;
			}
			else if (strcmp("xorshift64", postProcessingMethod) == 0) {
				postProcessingMethodId = 1;
			}
			else {
				fprintf(stderr, "Invalid post processing method: %s \n", postProcessingMethod);
				return -1;
			}
		}
		else if (strcmp("-npe", argv[idx]) == 0 || strcmp("--named-pipe-endpoint",
			argv[idx]) == 0) {
			if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
				return -1;
			}
			mbstowcs_s(&numCharConverted, pipeEndPoint, argv[idx], strlen(argv[idx]) + 1);
			idx++;
		}
		else if (parseClusterSize(idx, argc, argv) == -1) {
			return -1;
		}
		else if (parsePipeInstances(idx, argc, argv) == -1) {
			return -1;
		}
		else if (parsePowerProfileNum(idx, argc, argv) == -1) {
			return -1;
		}
		else {
			// Could not handle the argument, skip to the next one
			++idx;
		}
	}

	return processServer();
}

/**
* Parse power profile number if specified
*
* @param int idx - current parameter number
* @param int argc - number of parameters
* @param char ** argv - parameters
* @return int - 0 when successfully parsed
*/
int parsePowerProfileNum(int idx, int argc, char **argv) {
	if (idx < argc) {
		if (strcmp("-ppn", argv[idx]) == 0 || strcmp("--power-profile-number", argv[idx]) == 0) {
			if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
				return -1;
			}
			ppNum = atoi(argv[idx++]);
			if (ppNum < 0 || ppNum > 9) {
				fprintf(stderr, "Power profile number invalid, must be between 0 and 9\n");
				return -1;
			}
		}
	}
	return 0;
}

/**
* Validate command line argument count
*
* @param int curIdx
* @param int actualArgumentCount
* @return swrngBool - true if run successfully
*/
swrngBool validateArgumentCount(int curIdx, int actualArgumentCount) {
	if (curIdx >= actualArgumentCount) {
		fprintf(stderr, "\nMissing command line arguments\n\n");
		displayUsage();
		return SWRNG_FALSE;
	}
	return SWRNG_TRUE;
}

/**
* Parse cluster size if specified
*
* @param int idx - current parameter number
* @param int argc - number of parameters
* @param char ** argv - parameters
* @return int - 0 when successfully parsed
*/
int parseClusterSize(int idx, int argc, char **argv) {
	if (idx < argc) {
		if (strcmp("-cs", argv[idx]) == 0 || strcmp("--cluster-size",
			argv[idx]) == 0) {
			if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
				return -1;
			}
			clusterSize = atoi(argv[idx++]);
			if (clusterSize < 1 || clusterSize > 10) {
				fprintf(stderr, "Cluster size invalid value, must be between 1 and 10\n");
				return -1;
			}
		}
	}
	return 0;
}

/**
* Parse pipe instances if specified
*
* @param int idx - current parameter number
* @param int argc - number of parameters
* @param char ** argv - parameters
* @return int - 0 when successfully parsed
*/
int parsePipeInstances(int idx, int argc, char **argv) {
	if (idx < argc) {
		if (strcmp("-pi", argv[idx]) == 0 || strcmp("--pipe-instances",
			argv[idx]) == 0) {
			if (validateArgumentCount(++idx, argc) == SWRNG_FALSE) {
				return -1;
			}
			pipeInstances = atoi(argv[idx++]);
			if (pipeInstances < 1 || pipeInstances > MAX_PIPE_INSTANCES) {
				fprintf(stderr, "Pipe Instances parameter is invalid, must be an integer between 1 and %d\n", MAX_PIPE_INSTANCES);
				return -1;
			}
		}
	}
	return 0;
}

/**
* Open SwiftRNG device
*
* @return int - 0 when run successfully
*/
int openDevice() {
	int status = swrngCLOpen(&ctxt, clusterSize);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Cannot open device cluster , error code %d\n", status);
		swrngCLClose(&ctxt);
		return status;
	}

	if (statisticalTestsEnabled == SWRNG_FALSE) {
		status = swrngDisableCLStatisticalTests(&ctxt);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, "Cannot disable statistical tests, error code %d\n", status);
			swrngCLClose(&ctxt);
			return status;
		}
	}
	if (postProcessingEnabled == SWRNG_FALSE) {
		status = swrngDisableCLPostProcessing(&ctxt);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, "Cannot disable post processing, error code %d\n",
				status);
			swrngCLClose(&ctxt);
			return status;
		}
	}
	else if (postProcessingMethod != NULL) {
		status = swrngEnableCLPostProcessing(&ctxt, postProcessingMethodId);
		if (status != SWRNG_SUCCESS) {
			fprintf(stderr, "Cannot enable processing method, error code %d ... ",
				status);
			swrngCLClose(&ctxt);
			return status;
		}
	}

	status = swrngSetCLPowerProfile(&ctxt, ppNum);
	if (status != SWRNG_SUCCESS) {
		fprintf(stderr, "Cannot set cluster power profile, error code %d\n",
			status);
		swrngCLClose(&ctxt);
		return status;
	}

	return SWRNG_SUCCESS;
}

/**
* Create a pool of named pipe instances
*
* @return int - 0 when run successfully
*/
int createPipeInstances() {
	for (i = 0; i < pipeInstances; i++)
	{

		hEvents[i] = CreateEvent(
			NULL,
			TRUE,
			TRUE,
			NULL);

		if (hEvents[i] == NULL)
		{
			fprintf(stderr, "CreateEvent failed with %d error code.\n", GetLastError());
			return 0;
		}

		Pipe[i].oOverlap.hEvent = hEvents[i];

		Pipe[i].hPipeInst = CreateNamedPipe(
			pipeEndPoint,            // pipe name 
			PIPE_ACCESS_DUPLEX |     // read/write access 
			FILE_FLAG_OVERLAPPED,    // overlapped mode 
			PIPE_TYPE_BYTE |      // byte-type pipe 
			PIPE_READMODE_BYTE |  // byte-read mode 
			PIPE_WAIT,               // blocking mode 
			pipeInstances,               // number of instances 
			WRITE_BUFSIZE,		// output buffer size 
			sizeof(READCMD),    // input buffer size 
			PIPE_TIMEOUT,            // client time-out 
			NULL);                   // default security attributes 

		if (Pipe[i].hPipeInst == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "CreateNamedPipe failed with %d error code.\n", GetLastError());
			return -1;
		}

		Pipe[i].fPendingIO = ConnectToNewClient(
			Pipe[i].hPipeInst,
			&Pipe[i].oOverlap);

		Pipe[i].dwState = Pipe[i].fPendingIO ? CONNECTING_STATE : READING_STATE;
	}
	return SWRNG_SUCCESS;
}

/**
* Start the server
*
* @return int - 0 when run successfully
*/
int processServer() {

	int devStatus = openDevice();
	if (devStatus != SWRNG_SUCCESS) {
		return devStatus;
	}

	int pipesStatus = createPipeInstances();
	if (pipesStatus != SWRNG_SUCCESS) {
		return pipesStatus;
	}

	strcpy_s(postProcessingMethodName, "default");
	if (postProcessingEnabled == SWRNG_FALSE) {
		strcpy_s(postProcessingMethodName, "none");
	}
	else if (postProcessingMethod != NULL) {
		strcpy_s(postProcessingMethodName, postProcessingMethod);
	}

	printf("Entropy server started using a cluster of %d devices, post processing: '%s'", swrngGetCLSize(&ctxt), postProcessingMethodName);
	_tprintf(TEXT(", statistical tests "));
	if (statisticalTestsEnabled)
		_tprintf(TEXT("enabled"));
	else
		_tprintf(TEXT("disabled"));
	_tprintf(TEXT(", on named pipe: %s\n"), pipeEndPoint);

	while (1)
	{
		dwWait = WaitForMultipleObjects(
			pipeInstances,    // number of event objects 
			hEvents,      // array of event objects 
			FALSE,        // does not wait for all 
			INFINITE);    // waits indefinitely 

		i = dwWait - WAIT_OBJECT_0;  // determines which pipe 
		if (i < 0 || i >(pipeInstances - 1))
		{
			fprintf(stderr, "Index out of range.\n");
			return 0;
		}

		if (Pipe[i].fPendingIO)
		{
			fSuccess = GetOverlappedResult(
				Pipe[i].hPipeInst, // handle to pipe 
				&Pipe[i].oOverlap, // OVERLAPPED structure 
				&cbRet,            // bytes transferred 
				FALSE);            // do not wait 

			switch (Pipe[i].dwState)
			{
			case CONNECTING_STATE:
				if (!fSuccess)
				{
					fprintf(stderr, "Error %d.\n", GetLastError());
					return 0;
				}
				Pipe[i].dwState = READING_STATE;
				break;

			case READING_STATE:
				if (!fSuccess || cbRet == 0)
				{
					reConnect(i);
					continue;
				}
				Pipe[i].cbRead = cbRet;
				Pipe[i].dwState = WRITING_STATE;
				break;

			case WRITING_STATE:
				if (!fSuccess || cbRet != Pipe[i].chRequest.cbReqData)
				{
					reConnect(i);
					continue;
				}
				Pipe[i].dwState = READING_STATE;
				break;

			default:
			{
				fprintf(stderr, "Invalid pipe state.\n");
				return 0;
			}
			}
		}

		switch (Pipe[i].dwState)
		{
		case READING_STATE:
			fSuccess = ReadFile(
				Pipe[i].hPipeInst,
				&Pipe[i].chRequest,
				sizeof(READCMD),
				&Pipe[i].cbRead,
				&Pipe[i].oOverlap);

			if (fSuccess && Pipe[i].cbRead == sizeof(READCMD))
			{
				Pipe[i].fPendingIO = FALSE;
				Pipe[i].dwState = WRITING_STATE;
				continue;
			}
			dwErr = GetLastError();
			if (!fSuccess && (dwErr == ERROR_IO_PENDING))
			{
				Pipe[i].fPendingIO = TRUE;
				continue;
			}
			reConnect(i);
			break;

		case WRITING_STATE:

			devStatus = fillEntropyForWrite(i);

			if (devStatus != SWRNG_SUCCESS) {
				reConnect(i);
				break;
			}
			fSuccess = WriteFile(
				Pipe[i].hPipeInst,
				Pipe[i].chReply,
				Pipe[i].chRequest.cbReqData,
				&cbRet,
				&Pipe[i].oOverlap);

			if (fSuccess && cbRet == Pipe[i].chRequest.cbReqData)
			{
				Pipe[i].fPendingIO = FALSE;
				Pipe[i].dwState = READING_STATE;
				continue;
			}

			dwErr = GetLastError();
			if (!fSuccess && (dwErr == ERROR_IO_PENDING))
			{
				Pipe[i].fPendingIO = TRUE;
				continue;
			}
			reConnect(i);
			break;

		default:
		{
			fprintf(stderr, "Invalid pipe state.\n");
			return 0;
		}
		}
	}

	return 0;
}

/**
* Populate the entropy buffer
*
* @param i - pipe instance index
* @return int - 0 when run successfully
*/
int fillEntropyForWrite(DWORD i) {
	int devStatus = -1;
	unsigned char testCounter = 0;
	if (Pipe[i].chRequest.cbReqData <= 0 || Pipe[i].chRequest.cbReqData > WRITE_BUFSIZE) {
		return devStatus;
	}
	switch (Pipe[i].chRequest.cmd) {
	case CMD_ENTROPY_RETRIEVE_ID:
		devStatus = retrieveEntropy(i);
		if (devStatus != SWRNG_SUCCESS) {
			swrngCLClose(&ctxt);
			devStatus = openDevice();
			if (devStatus == SWRNG_SUCCESS) {
				devStatus = retrieveEntropy(i);
			}
		}
		break;
	case CMD_DIAG_ID:
		testCounter = 0;
		for (DWORD t = 0; t < Pipe[i].chRequest.cbReqData; t++) {
			Pipe[i].chReply[t] = testCounter++;
		}
		devStatus = 0;
		break;
	default:
		fprintf(stderr, "Invalid command received: %d \n", Pipe[i].chRequest.cmd);
		devStatus = -1;
	}
	return devStatus;
}

/**
* Populate the entropy buffer
*
* @param i - pipe instance index
* @return int - 0 when run successfully
*/
int retrieveEntropy(DWORD i) {
	return swrngGetCLEntropy(&ctxt, Pipe[i].chReply, Pipe[i].chRequest.cbReqData);
}

/**
* @param i - pipe instance index
*/
void reConnect(DWORD i)
{
	if (!DisconnectNamedPipe(Pipe[i].hPipeInst))
	{
		fprintf(stderr, "DisconnectNamedPipe failed with %d.\n", GetLastError());
	}

	Pipe[i].fPendingIO = ConnectToNewClient(
		Pipe[i].hPipeInst,
		&Pipe[i].oOverlap);

	Pipe[i].dwState = Pipe[i].fPendingIO ?
		CONNECTING_STATE : // still connecting 
		READING_STATE;     // ready to read 
}

/*
* This function is called to start an overlapped connect operation.
* connection has been completed.
* @param hPipe pipe handle
* @param lpo overlapped flag
* @return TRUE if an operation is pending or FALSE if the
*/

BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
	BOOL fConnected, fPendingIO = FALSE;
	fConnected = ConnectNamedPipe(hPipe, lpo);
	if (fConnected)
	{
		fprintf(stderr, "ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}

	switch (GetLastError())
	{
	case ERROR_IO_PENDING:
		fPendingIO = TRUE;
		break;
	case ERROR_PIPE_CONNECTED:
		if (SetEvent(lpo->hEvent))
			break;
	default:
	{
		fprintf(stderr, "ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}
	}
	return fPendingIO;
}
