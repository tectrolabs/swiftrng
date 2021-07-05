/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for interacting with the entropy server using named pipes.

 */

 /**
  *    @file EntropyServerConnector.h
  *    @date 07/04/2021
  *    @Author: Andrian Belinski
  *    @version 1.1
  *
  *    @brief A pipe service client for downloading true random bytes from the entropy server.
  */

#ifndef ENTROPY_SERVER_CLIENT_H_
#define ENTROPY_SERVER_CLIENT_H_

#include <windows.h>
#include <string>
#include <sstream>

using namespace std;

namespace entropy {
	namespace server {
		namespace api {


			enum class EntropyServerCommand {
				getEntropy = 0,
				getTestData = 1,
				getDeviceSerialNumber = 2,
				getDeviceModel = 3,
				getDeviceMinorVersion = 4,
				getDeviceMajorVersion = 5,
				getServerMinorVersion = 6,
				getServerMajorVersion = 7,
				getNoiseSourceOne = 8,
				getNoiseSourceTwo = 9
			};

			class EntropyServerConnector
			{
			public:
				EntropyServerConnector() : m_pipe_endpoint(L"\\\\.\\pipe\\AlphaRNG") {}
				EntropyServerConnector(const string& pipe_endpoint);
				EntropyServerConnector(const wstring& pipe_endpoint) : m_pipe_endpoint(pipe_endpoint) {}
				bool is_connected() { return m_is_connected; }
				bool open_named_pipe();
				void close_named_pipe();
				string get_last_error() { return m_error_log_oss.str(); }
				bool get_entropy(unsigned char* rcv_buffer, DWORD byte_count);
				bool get_noise_source_1(unsigned char* rcv_buffer, DWORD byte_count);
				bool get_noise_source_2(unsigned char* rcv_buffer, DWORD byte_count);
				bool get_test_bytes(unsigned char* rcv_buffer, DWORD byte_count);
				bool get_device_serial_number(string& device_serial_number);
				bool get_device_model(string& device_model);
				bool get_device_minor_version(int& device_minor_version);
				bool get_device_major_version(int& device_major_version);
				bool get_server_minor_version(int& server_minor_version);
				bool get_server_major_version(int& server_major_version);
				wstring get_pipe_endpoint() { return m_pipe_endpoint; }
				virtual ~EntropyServerConnector() { close_named_pipe(); }

			private:
				bool get_bytes(EntropyServerCommand cmd, unsigned char* rcv_buffer, DWORD byte_count);
				void clear_error_log();

			private:
				wstring m_pipe_endpoint;
				HANDLE m_pipe_handle = NULL;
				bool m_is_connected = false;
				ostringstream m_error_log_oss;

#pragma pack (1)
				typedef struct
				{
					DWORD cmd;
					DWORD num_bytes;
				} REQCMD;
#pragma pack ()

			};
		}
	}
} /* namespace entropy::server::api */

#endif /* ENTROPY_SERVER_CLIENT_H_ */
