/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 A minimal unit test framework for the SwiftRNG software kit. It has no external dependencies.
 Each test source file is built into its own executable, which includes this header and runs every
 TEST() defined in that file. The process exit code is 0 when all tests pass.
 */

/**
 *    @file test_framework.h
 *
 *    @brief Minimal test registration, assertion macros and a main() function for the unit tests.
 */
#ifndef SWIFTRNG_TESTS_TEST_FRAMEWORK_H_
#define SWIFTRNG_TESTS_TEST_FRAMEWORK_H_

#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

namespace tf {

struct TestCase {
	const char *name;
	void (*run)();
	const char *known_bug;  // reason, if the test is expected to fail until a known bug is fixed
};

inline std::vector<TestCase> &registry() {
	static std::vector<TestCase> tests;
	return tests;
}

// Number of failed checks in the test that is currently running
inline int &current_failures() {
	static int failures = 0;
	return failures;
}

struct Registrar {
	Registrar(const char *name, void (*run)(), const char *known_bug) {
		registry().push_back({name, run, known_bug});
	}
};

// True while a test that is expected to fail (a known bug) runs, to keep its failures out of the output
inline bool &quiet() {
	static bool value = false;
	return value;
}

inline void report_failure(const char *file, int line, const std::string &message) {
	if (!quiet()) {
		std::fprintf(stderr, "    %s:%d: %s\n", file, line, message.c_str());
	}
	current_failures()++;
}

inline std::string to_hex(const unsigned char *data, size_t size) {
	static const char digits[] = "0123456789abcdef";
	std::string out;
	for (size_t i = 0; i < size; ++i) {
		out += digits[data[i] >> 4];
		out += digits[data[i] & 0x0f];
	}
	return out;
}

inline std::vector<unsigned char> from_hex(const std::string &hex) {
	std::vector<unsigned char> out;
	for (size_t i = 0; i + 1 < hex.size(); i += 2) {
		out.push_back((unsigned char)std::stoi(hex.substr(i, 2), nullptr, 16));
	}
	return out;
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, std::string>::type
describe(T value) {
	return std::to_string(value);
}

inline std::string describe(const std::string &value) {
	return "\"" + value + "\"";
}

inline std::string describe(const char *value) {
	return describe(std::string(value));
}

inline std::string describe(bool value) {
	return value ? "true" : "false";
}

inline int run_all() {
	int failed_tests = 0;
	int known_bugs = 0;
	for (const TestCase &test : registry()) {
		current_failures() = 0;
		quiet() = test.known_bug != nullptr;
		test.run();
		quiet() = false;
		bool failed = current_failures() != 0;
		if (test.known_bug == nullptr) {
			if (!failed) {
				std::printf("[ PASS  ] %s\n", test.name);
			} else {
				std::printf("[ FAIL  ] %s (%d failed check%s)\n", test.name, current_failures(), current_failures() == 1 ? "" : "s");
				failed_tests++;
			}
		} else if (failed) {
			// Expected: the test documents a defect that has not been fixed yet
			std::printf("[ XFAIL ] %s (known bug: %s)\n", test.name, test.known_bug);
			known_bugs++;
		} else {
			std::printf("[ XPASS ] %s passes now, remove its known bug marker (%s)\n", test.name, test.known_bug);
			failed_tests++;
		}
	}
	std::printf("%d tests: %d failed, %d known bug%s\n", (int)registry().size(), failed_tests, known_bugs, known_bugs == 1 ? "" : "s");
	return failed_tests == 0 ? 0 : 1;
}

} // namespace tf

#define TEST_IMPL(name, known_bug) \
	static void test_##name(); \
	static tf::Registrar registrar_##name(#name, test_##name, known_bug); \
	static void test_##name()

// Define a test: TEST(name) { ...checks... }
#define TEST(name) TEST_IMPL(name, nullptr)

// Define a test that documents a known, unfixed defect. It is reported as XFAIL while it fails (and
// does not fail the run), and as an error once it starts passing, so the marker gets removed.
#define TEST_KNOWN_BUG(name, reason) TEST_IMPL(name, reason)

// Record a failed check and continue the test
#define CHECK(condition) \
	do { if (!(condition)) tf::report_failure(__FILE__, __LINE__, "CHECK failed: " #condition); } while (0)

// Record a failed check and stop the test (the test function must return void)
#define REQUIRE(condition) \
	do { if (!(condition)) { tf::report_failure(__FILE__, __LINE__, "REQUIRE failed: " #condition); return; } } while (0)

#define CHECK_EQ(actual, expected) \
	do { \
		auto actual_value = (actual); \
		auto expected_value = (expected); \
		if (!(actual_value == expected_value)) { \
			tf::report_failure(__FILE__, __LINE__, std::string("CHECK_EQ failed: " #actual " is ") \
					+ tf::describe(actual_value) + ", expected " + tf::describe(expected_value)); \
		} \
	} while (0)

int main() {
	return tf::run_all();
}

#endif /* SWIFTRNG_TESTS_TEST_FRAMEWORK_H_ */
