cmake_minimum_required(VERSION 3.10)
project(swrng LANGUAGES C CXX)

# Match the Makefile: Release builds use plain -O2, with no debug info and no -DNDEBUG.
# Replace CMake's default Release flags (-O3 -DNDEBUG), but leave any value the user set.
if(NOT WIN32)
  foreach(lang C CXX)
    if(CMAKE_${lang}_FLAGS_RELEASE STREQUAL "-O3 -DNDEBUG")
      set(CMAKE_${lang}_FLAGS_RELEASE "-O2" CACHE STRING "Flags used by the ${lang} compiler for Release builds" FORCE)
    endif()
  endforeach()
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SWRNG_BUILD_ENGINE "Also build the SwiftRNG OpenSSL engine (eng_swiftrng), for OpenSSL 3.4 and earlier" OFF)
option(SWRNG_BUILD_PROVIDER "Also build the SwiftRNG OpenSSL provider (swiftrng.so), for OpenSSL 4.0 and 3.5.x with provider random bytes support" OFF)

# libusb: <libusb-1.0/libusb.h> on Linux and macOS, <libusb.h> on FreeBSD (part of the base system).
# Homebrew's libusb is keg-only on macOS, so probe its usual locations.
find_path(LIBUSB_INCLUDE_DIR NAMES libusb-1.0/libusb.h libusb.h
  HINTS /opt/homebrew/opt/libusb/include /usr/local/opt/libusb/include /usr/local/include)
find_library(LIBUSB_LIBRARY NAMES usb-1.0 usb
  HINTS /opt/homebrew/opt/libusb/lib /usr/local/opt/libusb/lib /usr/local/lib)
if(NOT LIBUSB_INCLUDE_DIR OR NOT LIBUSB_LIBRARY)
  message(FATAL_ERROR "libusb not found. Install it (libusb-1.0-0-dev, brew install libusb, or the FreeBSD base system).")
endif()

find_package(Threads REQUIRED)

# Core library shared by all utilities
add_library(swiftrng STATIC
  api-src/RandomSeqGenerator.cpp
  api-src/SwiftRngApi.cpp
  api-src/SwiftRngApiCWrapper.cpp
  api-src/USBSerialDevice.cpp
)
set_target_properties(swiftrng PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(swiftrng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/api-inc ${LIBUSB_INCLUDE_DIR})
target_link_libraries(swiftrng PUBLIC ${LIBUSB_LIBRARY})
if(MSVC)
  target_compile_options(swiftrng PRIVATE /W3)
else()
  target_compile_options(swiftrng PUBLIC -Wall -Wextra)
endif()

# Client library used by the *-cl utilities
add_library(swrng-cl-api STATIC api-src/swrng-cl-api.c)
target_link_libraries(swrng-cl-api PUBLIC swiftrng Threads::Threads)

# C utilities that talk to the device directly
set(SWRNG_C_TOOLS swdiag swperftest bitcount swrng swrawrandom)
# C utilities that use the client library
set(SWRNG_CL_TOOLS swdiag-cl swperf-cl-test bitcount-cl swrng-cl)
# C++ utilities
set(SWRNG_CPP_TOOLS swrngseqgen)
# Samples (built, but not installed, as with the Makefile)
set(SWRNG_C_SAMPLES sample)
set(SWRNG_CL_SAMPLES sample-cl)

foreach(tool ${SWRNG_C_TOOLS} ${SWRNG_C_SAMPLES})
  add_executable(${tool} ${tool}.c)
  target_link_libraries(${tool} PRIVATE swiftrng)
  set_target_properties(${tool} PROPERTIES LINKER_LANGUAGE CXX)
endforeach()

foreach(tool ${SWRNG_CL_TOOLS} ${SWRNG_CL_SAMPLES})
  add_executable(${tool} ${tool}.c)
  target_link_libraries(${tool} PRIVATE swrng-cl-api)
  set_target_properties(${tool} PROPERTIES LINKER_LANGUAGE CXX)
endforeach()

foreach(tool ${SWRNG_CPP_TOOLS})
  add_executable(${tool} ${tool}.cpp)
  target_link_libraries(${tool} PRIVATE swiftrng)
endforeach()
add_executable(sample++ sample++.cpp)
target_link_libraries(sample++ PRIVATE swiftrng)

# Optional OpenSSL engine
if(SWRNG_BUILD_ENGINE)
  find_package(OpenSSL REQUIRED)
  add_library(eng_swiftrng MODULE eng_swiftrng.cpp)
  set_target_properties(eng_swiftrng PROPERTIES PREFIX "" SUFFIX ".so")
  target_link_libraries(eng_swiftrng PRIVATE swiftrng OpenSSL::Crypto)
endif()

# Optional OpenSSL provider. OpenSSL 4.0 removed engines; this provider makes RAND_bytes() return bytes
# from the SwiftRNG device when it is named as the random provider in the OpenSSL config file.
if(SWRNG_BUILD_PROVIDER)
  find_package(OpenSSL REQUIRED)
  include(CheckSymbolExists)
  set(CMAKE_REQUIRED_INCLUDES ${OPENSSL_INCLUDE_DIR})
  check_symbol_exists(OSSL_FUNC_PROVIDER_RANDOM_BYTES openssl/core_dispatch.h SWRNG_OPENSSL_HAS_RANDOM_BYTES_HOOK)
  unset(CMAKE_REQUIRED_INCLUDES)
  if(NOT SWRNG_OPENSSL_HAS_RANDOM_BYTES_HOOK)
    message(FATAL_ERROR "The OpenSSL found (${OPENSSL_VERSION}) has no provider random bytes support. "
            "The provider needs OpenSSL 4.0 or a 3.5.x release that has OSSL_FUNC_PROVIDER_RANDOM_BYTES. "
            "Use -DOPENSSL_ROOT_DIR=<path> to choose another OpenSSL, or the eng_swiftrng engine with older OpenSSL.")
  endif()
  # OpenSSL looks for provider modules with the platform suffix when they are loaded by name
  if(APPLE)
    set(SWRNG_PROVIDER_SUFFIX ".dylib")
  else()
    set(SWRNG_PROVIDER_SUFFIX ".so")
  endif()
  add_library(swiftrng_provider MODULE provider/swiftrng_provider.cpp provider/rng_backend_device.cpp)
  set_target_properties(swiftrng_provider PROPERTIES PREFIX "" OUTPUT_NAME swiftrng SUFFIX "${SWRNG_PROVIDER_SUFFIX}")
  target_link_libraries(swiftrng_provider PRIVATE swiftrng OpenSSL::Crypto)
endif()

# Install / uninstall (same set as the Makefile)
include(GNUInstallDirs)
install(TARGETS ${SWRNG_C_TOOLS} ${SWRNG_CL_TOOLS} ${SWRNG_CPP_TOOLS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(NOT TARGET uninstall)
  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -DMANIFEST=${CMAKE_BINARY_DIR}/install_manifest.txt
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake)
endif()

# Unit tests, opt-in: cmake -S . -B build -DSWRNG_BUILD_TESTS=ON && cmake --build build && (cd build && ctest --output-on-failure)
option(SWRNG_BUILD_TESTS "Build the unit tests" OFF)
if(SWRNG_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
