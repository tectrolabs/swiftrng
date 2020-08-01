/*
* entropy-server.h
* Ver. 2.0
*
*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (C) 2014-2020 TectroLabs, http://tectrolabs.com

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

This program is used for interacting with the hardware random data generator device SwiftRNG for the purpose of
downloading and distributing true random bytes using named pipes.

This program requires the libusb-1.0 library and the DLL when communicating with any SwiftRNG device.
Please read the provided documentation for libusb-1.0 installation details.

This program uses libusb-1.0 (directly or indirectly) which is distributed under the terms of the GNU Lesser General
Public License as published by the Free Software Foundation. For more information, please visit: http://libusb.info

This program may only be used in conjunction with the SwiftRNG device.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef ENTROPY_SERVER_H_
#define ENTROPY_SERVER_H_

#include "swrngapi.h"
#include <windows.h> 
#include <stdio.h>
#include <tchar.h>

#include <strsafe.h>

#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 
#define DEFAULT_PIPE_INSTANCES 10
#define MAX_PIPE_INSTANCES 64
#define PIPE_TIMEOUT 5000
#define WRITE_BUFSIZE 100000
#define CMD_ENTROPY_RETRIEVE_ID 0
#define CMD_DIAG_ID 1
#define PIPENAME_MAX_CHARS 128


typedef struct
{
	DWORD cmd;
	DWORD cbReqData;
} READCMD;

typedef struct
{
	OVERLAPPED oOverlap;
	HANDLE hPipeInst;
	READCMD chRequest;
	DWORD cbRead;
	unsigned char chReply[WRITE_BUFSIZE];
	DWORD dwState;
	BOOL fPendingIO;
} PIPEINST;

//
// Local variables
//

PIPEINST Pipe[MAX_PIPE_INSTANCES];
HANDLE hEvents[MAX_PIPE_INSTANCES];
int deviceNum; // USB device number starting with 0 (a command line argument)
DWORD pipeInstances = DEFAULT_PIPE_INSTANCES;
int ppNum = 9; // Power profile number, between 0 and 9
char *postProcessingMethod = NULL; // Post processing method or NULL if not specified
int postProcessingMethodId = 0; // Post processing method id, 0 - SHA256, 1 - xorshift64, 2 - SHA512
char postProcessingMethodName[64];
SwrngContext ctxt;
swrngBool postProcessingEnabled = SWRNG_TRUE;
swrngBool statisticalTestsEnabled = SWRNG_TRUE;
DWORD i, dwWait, cbRet, dwErr;
BOOL fSuccess;
BOOL isDevieNumSpecified = FALSE;
DeviceSerialNumber dsn;
DeviceModel dm;
DeviceVersion dv;
static wchar_t pipeEndPoint[PIPENAME_MAX_CHARS + 1];
static char defaultPipeEndpoint[] = "\\\\.\\pipe\\SwiftRNG";
size_t numCharConverted;
char embeddedCorrectionMethodStr[32];
int embeddedCorrectionMethodId;


/**
* Function Declarations
*/

void reConnect(DWORD);
BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED);
int displayDevices();
void displayUsage();
int process(int argc, char **argv);
int processArguments(int argc, char **argv);
int validateArgumentCount(int curIdx, int actualArgumentCount);
int parseDeviceNum(int idx, int argc, char **argv);
int parsePipeInstances(int idx, int argc, char **argv);
int parsePowerProfileNum(int idx, int argc, char **argv);
int processServer();
int fillEntropyForWrite(DWORD i);
int retrieveEntropy(DWORD i);

#endif /* ENTROPY_SERVER_H_ */
