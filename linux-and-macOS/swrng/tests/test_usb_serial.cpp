/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the USBSerialDevice class. No device is needed: the tests create a pseudo terminal pair and
 play the device on the master side, while USBSerialDevice connects to the slave side exactly as it would
 to /dev/cu.usbmodem... or /dev/ttyACM...
 */

/**
 *    @file test_usb_serial.cpp
 *
 *    @brief Connection, locking, send and receive tests for USBSerialDevice, using a pseudo terminal.
 */
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <USBSerialDevice.h>
#include "test_framework.h"

// A pseudo terminal pair. The test is the device on the master side.
struct Pty {
	int master {-1};
	std::string slave;

	Pty() {
		master = posix_openpt(O_RDWR | O_NOCTTY);
		if (master >= 0 && grantpt(master) == 0 && unlockpt(master) == 0) {
			const char *name = ptsname(master);
			if (name != nullptr) {
				slave = name;
			}
		}
	}

	~Pty() {
		if (master >= 0) {
			close(master);
		}
	}

	bool ok() const { return master >= 0 && !slave.empty(); }

	bool write_bytes(const unsigned char *data, size_t size) const {
		return write(master, data, size) == (ssize_t)size;
	}

	// Read up to 'size' bytes that arrive within 'timeout_ms'
	size_t read_bytes(unsigned char *buffer, size_t size, int timeout_ms) const {
		size_t total = 0;
		while (total < size) {
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(master, &fds);
			struct timeval tv;
			tv.tv_sec = timeout_ms / 1000;
			tv.tv_usec = (timeout_ms % 1000) * 1000;
			if (select(master + 1, &fds, nullptr, nullptr, &tv) <= 0) {
				break;
			}
			ssize_t n = read(master, buffer + total, size - total);
			if (n <= 0) {
				break;
			}
			total += (size_t)n;
		}
		return total;
	}
};

// A unique temporary file that is removed when the object goes out of scope
struct TempFile {
	std::string path;
	TempFile() {
		char name[] = "/tmp/swrng_test_XXXXXX";
		int fd = mkstemp(name);
		if (fd >= 0) {
			close(fd);
			path = name;
		}
	}
	~TempFile() {
		if (!path.empty()) {
			std::remove(path.c_str());
		}
	}
};

// Bytes that a text-oriented terminal would be tempted to change: newline, carriage return, NUL, 0xFF
static std::vector<unsigned char> awkward_bytes(size_t size) {
	std::vector<unsigned char> data(size);
	for (size_t i = 0; i < size; ++i) {
		data[i] = (unsigned char)(i * 37 + 3);
	}
	data[0] = 0x0a;
	data[1] = 0x0d;
	data[2] = 0x00;
	data[3] = 0xff;
	return data;
}

TEST(serial_new_object_is_not_connected) {
	USBSerialDevice device;
	device.initialize();
	unsigned char byte = 'x';
	unsigned char buffer[8];
	int count = 0;
	CHECK(!device.is_connected());
	CHECK_EQ(device.send_command(&byte, 1, &count), -1);
	CHECK_EQ(device.receive_data(buffer, 8, &count), -1);
	CHECK(!device.disconnect());
}

TEST(serial_connecting_to_a_missing_port_fails) {
	USBSerialDevice device;
	device.initialize();
	CHECK(!device.connect("/nonexistent/swrng_no_such_port"));
	CHECK(!device.is_connected());
	CHECK(device.get_error_log().find("Could not open serial device") != std::string::npos);
}

TEST(serial_connecting_to_something_that_is_not_a_terminal_fails) {
	TempFile file;
	REQUIRE(!file.path.empty());
	USBSerialDevice device;
	device.initialize();
	CHECK(!device.connect(file.path.c_str()));
	CHECK(!device.is_connected());
	CHECK(device.get_error_log().find("Could not retrieve configuration") != std::string::npos);
}

TEST(serial_connects_to_a_terminal_and_disconnects) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice device;
	device.initialize();
	REQUIRE(device.connect(pty.slave.c_str()));
	CHECK(device.is_connected());
	CHECK(device.disconnect());
	CHECK(!device.is_connected());
}

TEST(serial_can_reconnect_after_disconnecting) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice device;
	device.initialize();
	REQUIRE(device.connect(pty.slave.c_str()));
	REQUIRE(device.disconnect());
	CHECK(device.connect(pty.slave.c_str()));
	CHECK(device.is_connected());
}

TEST(serial_refuses_a_second_connect_while_connected) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice device;
	device.initialize();
	REQUIRE(device.connect(pty.slave.c_str()));
	CHECK(!device.connect(pty.slave.c_str()));
	CHECK(device.is_connected());
}

TEST(serial_port_in_use_by_another_object_is_refused) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice first;
	USBSerialDevice second;
	first.initialize();
	second.initialize();
	REQUIRE(first.connect(pty.slave.c_str()));
	CHECK(!second.connect(pty.slave.c_str()));
	CHECK(!second.is_connected());
	CHECK(second.get_error_log().find("Could not lock device") != std::string::npos);
}

TEST(serial_port_is_free_again_after_the_other_object_disconnects) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice first;
	USBSerialDevice second;
	first.initialize();
	second.initialize();
	REQUIRE(first.connect(pty.slave.c_str()));
	REQUIRE(first.disconnect());
	CHECK(second.connect(pty.slave.c_str()));
}

TEST(serial_sent_bytes_reach_the_device_unchanged) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice device;
	device.initialize();
	REQUIRE(device.connect(pty.slave.c_str()));

	std::vector<unsigned char> command = awkward_bytes(64);
	int sent = 0;
	REQUIRE(device.send_command(command.data(), (int)command.size(), &sent) == 0);
	CHECK_EQ(sent, (int)command.size());

	std::vector<unsigned char> arrived(command.size());
	size_t count = pty.read_bytes(arrived.data(), arrived.size(), 1000);
	CHECK_EQ(count, command.size());
	CHECK(arrived == command);
}

TEST(serial_received_bytes_match_what_the_device_sent) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice device;
	device.initialize();
	REQUIRE(device.connect(pty.slave.c_str()));

	std::vector<unsigned char> data = awkward_bytes(400);
	REQUIRE(pty.write_bytes(data.data(), data.size()));

	std::vector<unsigned char> received(data.size());
	int count = 0;
	CHECK_EQ(device.receive_data(received.data(), (int)received.size(), &count), 0);
	CHECK_EQ(count, (int)data.size());
	CHECK(received == data);
}

TEST(serial_receive_times_out_when_nothing_arrives) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice device;
	device.initialize();
	REQUIRE(device.connect(pty.slave.c_str()));

	unsigned char buffer[16];
	int count = -1;
	auto start = std::chrono::steady_clock::now();
	CHECK_EQ(device.receive_data(buffer, 16, &count), -7);
	auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	CHECK_EQ(count, 0);
	// The port is set to wait 100-200 ms for data, so the call returns promptly but not instantly
	CHECK(elapsed_ms >= 50 && elapsed_ms < 3000);
}

TEST(serial_receive_reports_the_partial_data_on_timeout) {
	Pty pty;
	REQUIRE(pty.ok());
	USBSerialDevice device;
	device.initialize();
	REQUIRE(device.connect(pty.slave.c_str()));

	std::vector<unsigned char> data = awkward_bytes(40);
	REQUIRE(pty.write_bytes(data.data(), data.size()));

	unsigned char buffer[100];
	int count = -1;
	CHECK_EQ(device.receive_data(buffer, 100, &count), -7);
	CHECK_EQ(count, 40);
	CHECK(std::memcmp(buffer, data.data(), 40) == 0);
}
