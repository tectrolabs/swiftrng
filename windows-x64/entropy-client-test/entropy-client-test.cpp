#include "stdafx.h"
#include "entropy-client-test.h"

//
// Testing thread 
//
unsigned int __stdcall testingThread(void*) {
	LPGETENTROPYBYTESER lpFunc;
	LPSETPIPEEP lpFuncEP;

	int testStatus;

	HINSTANCE tDLL = LoadLibrary(L"SwiftRNG.dll");
	if (tDLL == NULL)
	{
		isMultiThreadTestStatusSuccess = FALSE;
		return 0;
	}

	lpFunc = (LPGETENTROPYBYTESER)GetProcAddress(hDLL, "swftGetByteFromEntropyServerSynchronized");
	if (!lpFunc)
	{
		isMultiThreadTestStatusSuccess = FALSE;
		FreeLibrary(tDLL);
		return 0;
	}
	lpFuncEP = (LPSETPIPEEP)GetProcAddress(hDLL, "swftSetEntropyServerPipeEndpointSynchronized");
	if (!lpFuncEP)
	{
		isMultiThreadTestStatusSuccess = FALSE;
		FreeLibrary(tDLL);
		return 0;
	}
	testStatus = lpFuncEP(pipeEndPointASCII);
	if (testStatus != 0)
	{
		isMultiThreadTestStatusSuccess = FALSE;
		FreeLibrary(tDLL);
		return 0;
	}

	for (int i = 0; i < 500000; i++) {
		testStatus = lpFunc();
		if (testStatus > 255) {
			isMultiThreadTestStatusSuccess = FALSE;
			FreeLibrary(tDLL);
			return 0;
		}
	}
	FreeLibrary(tDLL);
	return 0;
}


/**
* Main entry
*
* @param int argc - number of parameters
* @param char ** argv - parameters
*
*/
int main(int argc, char **argv) {
	int status = 0;
	printf("----------------------------------------------------------------------------\n");
	printf("-------------------------- entropy-client-test -----------------------------\n");
	printf("--- A program utility for testing the connectivity to the entropy server. --\n");
	printf("--- Usage: entropy-client-test [pipe endpoint] -----------------------------\n");
	printf("----------------------------------------------------------------------------\n");

	if (argc < 2) {
		mbstowcs_s(&numCharConverted, pipeEndPoint, defaultPipeEndpoint, strlen(defaultPipeEndpoint) + 1);
		strcpy_s(pipeEndPointASCII, defaultPipeEndpoint);
	} else {
		mbstowcs_s(&numCharConverted, pipeEndPoint, argv[1], strlen(argv[1]) + 1);
		strcpy_s(pipeEndPointASCII, argv[1]);
	}

	printf("\n");
	_tprintf(TEXT("Using named pipe:  %s\n"), pipeEndPoint);
	printf("\n");

	printf("-------- Testing connectivity to the entropy server using named pipes ------\n");

	//
	// Step 1
	//
	printf("Connecting to the entropy server pipe .............................. ");
	status = openNamedPipe();
	if (status == 0) {
		printf("SUCCESS\n");
	}
	else {
		printf(" failed\n");
		printf("is entropy server running?\n");
		closeNamedPipe();
		return status;
	}
	closeNamedPipe();

	//
	// Step 2
	//
	printf("Retrieving 100000 bytes from the entropy server .................... ");
	status = retrieveEntropyFromServer(CMD_ENTROPY_RETRIEVE_ID, entropyBuffer, ENTROPY_BUFFER_SIZE);
	if (status == 0) {
		printf("SUCCESS\n");
	}
	else {
		printf(" failed\n");
		return status;
	}

	//
	// Step 3
	//
	printf("Running pipe communication diagnostics ............................. ");
	status = retrieveEntropyFromServer(CMD_DIAG_ID, entropyBuffer, ENTROPY_BUFFER_SIZE);
	if (status == 0) {
		unsigned char testCounter = 0;
		for (DWORD t = 0; t < ENTROPY_BUFFER_SIZE; t++) {
			if (entropyBuffer[t] != testCounter) {
				return -1;
			}
			testCounter++;
		}
		printf("SUCCESS\n");
	}
	else {
		printf(" failed\n");
		return status;
	}

	//
	// Step 4
	//
	printf("Calculating entropy download speed ................................. ");
	start = time(NULL);
	for (int l = 0; l < NUM_BLOCKS; l++) {
		status = retrieveEntropyFromServer(CMD_ENTROPY_RETRIEVE_ID, entropyBuffer, ENTROPY_BUFFER_SIZE);
		if (status != 0) {
			printf(" failed\n");
			return status;
		}
	}
	end = time(NULL);

	int64_t totalTime = end - start;
	if (totalTime == 0) {
		totalTime = 1;
	}
	double downloadSpeed = (ENTROPY_BUFFER_SIZE * NUM_BLOCKS) / totalTime / 1000.0 / 1000.0 * 8.0;
	printf("%3.2f Mbps\n", downloadSpeed);

	printf("\n");
	printf("------------ Testing connectivity to the entropy server using DLL ----------\n");

	//
	// Step 5
	//
	printf("Loading SwiftRNG.dll ............................................... ");
	hDLL = LoadLibrary(L"SwiftRNG.dll");
	if (hDLL == NULL)
	{
		printf(" failed\n");
		return -1;
	}
	else {
		printf("SUCCESS\n");
	}

	//
	// Step 6.1
	//
	printf("Getting proc addr swftSetEntropyServerPipeEndpointSynchronized() ... ");
	lpSwftSetEntropyServerPipeEndpointSynchronized = (LPSETPIPEEP)GetProcAddress(hDLL, "swftSetEntropyServerPipeEndpointSynchronized");
	if (!lpSwftSetEntropyServerPipeEndpointSynchronized)
	{
		printf(" failed\n");
		FreeLibrary(hDLL);
		return -1;
	}
	else {
		printf("SUCCESS\n");
	}

	//
	// Step 6.2
	//
	printf("Calling swftSetEntropyServerPipeEndpointSynchronized() ............. ");
	status = lpSwftSetEntropyServerPipeEndpointSynchronized(pipeEndPointASCII);
	if (status != 0) {
		printf("FAILED");
		FreeLibrary(hDLL);
		return -1;
	}
	printf("SUCCESS\n");

	//
	// Step 6.3
	//
	printf("Getting proc address swftGetByteFromEntropyServerSynchronized() .... ");
	lpSwrngGetByteFromEntropyServerSynchronized = (LPGETENTROPYBYTESER)GetProcAddress(hDLL, "swftGetByteFromEntropyServerSynchronized");
	if (!lpSwrngGetByteFromEntropyServerSynchronized)
	{
		printf(" failed\n");
		FreeLibrary(hDLL);
		return -1;
	}
	else {
		printf("SUCCESS\n");
	}

	//
	// Step 7
	//
	printf("Testing swftGetByteFromEntropyServerSynchronized() ................. ");
	for (uint64_t i = 0; i < 10000000; i++) {
		status = lpSwrngGetByteFromEntropyServerSynchronized();
		if (status > 255) {
			printf("FAILED");
			FreeLibrary(hDLL);
			return -1;
		}
	}
	printf("SUCCESS\n");


	//
	// Step 8
	//
	printf("swftGetByteFromEntropyServerSynchronized() download speed .......... ");
	start = time(NULL);
	for (int l = 0; l < NUM_BLOCKS * ENTROPY_BUFFER_SIZE; l++) {
		status = lpSwrngGetByteFromEntropyServerSynchronized();
		if (status > 255) {
			printf(" failed\n");
			return status;
		}
	}
	end = time(NULL);

	totalTime = end - start;
	if (totalTime == 0) {
		totalTime = 1;
	}
	downloadSpeed = (ENTROPY_BUFFER_SIZE * NUM_BLOCKS) / totalTime / 1000.0 / 1000.0 * 8.0;
	printf("%3.2f Mbps\n", downloadSpeed);

	//
	// Step 9, skip it for WIN32
	//
#if _WIN64
	printf("Testing swftGetByteFromEntropyServerSynchronized() %3d threads ----- ", NUM_TEST_THREADS);
	

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
#endif
	//
	// Step 10
	//
	printf("Getting proc address swftGetEntropyFromEntropyServer() ............. ");
	lpSwrngGetEntropyFromEntropyServer = (LPGETENTROPY)GetProcAddress(hDLL, "swftGetEntropyFromEntropyServer");
	if (!lpSwrngGetEntropyFromEntropyServer)
	{
		printf(" failed\n");
		FreeLibrary(hDLL);
		return -1;
	}
	else {
		printf("SUCCESS\n");
	}

	//
	// Step 11
	//
	printf("swftGetEntropyFromEntropyServer() download speed ................... ");
	start = time(NULL);
	for (int l = 0; l < NUM_BLOCKS; l++) {
		status = lpSwrngGetEntropyFromEntropyServer(entropyBuffer, ENTROPY_BUFFER_SIZE);
		if (status != 0) {
			printf(" failed\n");
			return status;
		}
	}
	end = time(NULL);

	totalTime = end - start;
	if (totalTime == 0) {
		totalTime = 1;
	}
	downloadSpeed = (ENTROPY_BUFFER_SIZE * NUM_BLOCKS) / totalTime / 1000.0 / 1000.0 * 8.0;
	printf("%3.2f Mbps\n", downloadSpeed);


	FreeLibrary(hDLL);
    return 0;
}

/*
* Retrieve entropy data from the entropy server
* @param commandId - command to send to the entropy server
* @param rcvBuff buffer for incoming data
* @param len - number of bytes to receive
* @return 0 - success
*/
int retrieveEntropyFromServer(DWORD commandId, unsigned char *rcvBuff, DWORD len) {
	int status = openNamedPipe();
	if (status == 0) {
		status = readEntropyFromPipe(commandId, rcvBuff, len);
	}
	closeNamedPipe();
	return status;
}

/*
* Open named pipe to the entropy server
*
* @return 0 - success
*/
int openNamedPipe() {
	while (1) {
		hPipe = CreateFile(
			pipeEndPoint,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		if (hPipe != INVALID_HANDLE_VALUE) {
			break;
		}

		if (GetLastError() != ERROR_PIPE_BUSY) {
			return -1;
		}

		if (!WaitNamedPipe(pipeEndPoint, 20000)) {
			return -1;
		}
	}

	dwMode = PIPE_READMODE_BYTE;
	fSuccess = SetNamedPipeHandleState(
		hPipe,    // pipe handle 
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
*
* @return 0 - success
*/
int closeNamedPipe() {
	CloseHandle(hPipe);
	return 0;
}

/*
* Read entropy data from named pipe
* @param commandId - command to send to the entropy server
* @param rcvBuff buffer for incoming data
* @param len - number of bytes to receive
* @return 0 - success
*/
int readEntropyFromPipe(DWORD commandId, unsigned char *rcvBuff, DWORD len) {
	cbToWrite = sizeof(REQCMD);
	reqCmd.cmd = commandId;
	reqCmd.cbReqData = len;

	fSuccess = WriteFile(
		hPipe,                  // pipe handle 
		&reqCmd,             // bytes 
		cbToWrite,              // bytes length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess) {
		return -1;
	}
	do {
		fSuccess = ReadFile(
			hPipe,    // pipe handle 
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
