#ifndef ENTROPY_CLIENT_TEST_H_
#define ENTROPY_CLIENT_TEST_H_

#include "SwiftRNG.h"
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <process.h>


#define ENTROPY_BUFFER_SIZE 100000
#define NUM_BLOCKS 1000
#define CMD_ENTROPY_RETRIEVE_ID 0
#define CMD_DIAG_ID 1
#define NUM_TEST_THREADS 50
#define NUM_PIPE_TEST_THREADS 50
#define PIPENAME_MAX_CHARS 128

typedef struct
{
	DWORD cmd;
	DWORD cbReqData;
} REQCMD;

REQCMD reqCmd;
HANDLE hPipe;
DWORD  cbRead, cbToWrite, cbWritten, dwMode;
unsigned char entropyBuffer[ENTROPY_BUFFER_SIZE];
HANDLE threads[NUM_TEST_THREADS];
HANDLE pipeThreads[NUM_PIPE_TEST_THREADS];
time_t start, end;
HINSTANCE hDLL; // Handle to the DLL

typedef int (CALLBACK* LPGETENTROPYBYTESER)();
typedef int (CALLBACK* LPGETENTROPY)(unsigned char *buffer, long length);
typedef int (CALLBACK* LPSETPIPEEP)(char *pipeEndpoint);

LPGETENTROPYBYTESER lpSwrngGetByteFromEntropyServerSynchronized;
LPGETENTROPY  lpSwrngGetEntropyFromEntropyServer;
LPSETPIPEEP  lpSwftSetEntropyServerPipeEndpointSynchronized;

volatile BOOL isMultiThreadTestStatusSuccess = TRUE;
volatile BOOL isPipeMultiThreadTestStatusSuccess = TRUE;
wchar_t pipeEndPoint[PIPENAME_MAX_CHARS + 1];
char pipeEndPointASCII[PIPENAME_MAX_CHARS + 1];
char defaultPipeEndpoint[] = "\\\\.\\pipe\\SwiftRNG";
size_t numCharConverted;

//
// Local functions
//

/*
* Open named pipe to the entropy server
*
* @return 0 - success
*/
int openNamedPipe();

/*
* Close named pipe to the entropy server
*
* @return 0 - success
*/
int closeNamedPipe();

/*
* Read entropy data from named pipe
* @param commandId - command to send to the entropy server
* @param rcvBuff buffer for incoming data
* @param len - number of bytes to receive
* @param hPipe - pipe handle
* @return 0 - success
*/
int readEntropyFromPipe(DWORD commandId, unsigned char *rcvBuff, DWORD len, HANDLE hPipe);

/*
* Retrieve entropy data from the entropy server
* @param commandId - command to send to the entropy server
* @param rcvBuff buffer for incoming data
* @param len - number of bytes to receive
* @return 0 - success
*/
int retrieveEntropyFromServer(DWORD commandId, unsigned char *rcvBuff, DWORD len);

#endif /* ENTROPY_CLIENT_TEST_H_ */

