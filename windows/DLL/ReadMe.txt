DLL API definition
==================


/**
 * Open SwiftRNG USB specific device.
 *
 * @param int deviceNum - device number (0 - for the first device)
 * @return int 0 - when open successfully or error code
 */
int swftOpen(int deviceNum)

/**
 * Close device if open
 *
 * @return int - 0 when processed successfully
 */
int swftClose() 

/**
 * A function to retrieve random bytes
 * @param insigned char *buffer - a pointer to the data receive buffer
 * @param long length - how many bytes expected to receive
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int swftGetEntropy(unsigned char *buffer, long length)

/**
 * A function to retrieve a random byte
 * @return random byte value between 0 and 255 (a value of 256 or greater will indicate an error) 
 *
 */
int swftGetEntropyByte()

/**
 * A function to set power profile
 * @param int ppNum - power profile number (between 0 and 9)
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
int swftSetPowerProfile(int ppNum)

/**
 * Retrieve the last error messages
 * @return - pointer to the error message
 */
const char* swftGetLastErrorMessage() 

/**
 * Retrieve SwiftRNG device model number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param char model[9] - pointer to a 9 bytes char array for holding SwiftRNG device model number
 * @return int 0 - when model retrieved successfully
 *
 */
int swftGetModel(char model[9])

/**
 * Retrieve SwiftRNG device version. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param char version[5] - pointer to a 5 bytes array for holding SwiftRNG device version
 * @return int - 0 when version retrieved successfully
 *
 */
int swftGetVersion(char version[5])

/**
 * Retrieve SwiftRNG device serial number. This call will fail when there is no SwiftRNG device
 * currently connected or when the device is already in use.
 *
 * @param char serialNumber[16] - pointer to a 16 bytes array for holding SwiftRNG device S/N
 * @return int - 0 when serial number retrieved successfully
 *
 */
int swftGetSerialNumber(char serialNumber[16])