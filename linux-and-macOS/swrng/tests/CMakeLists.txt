# Each test source file becomes its own executable and its own CTest test.
# None of the tests opens a SwiftRNG device, so they are safe to run with a device plugged in.
set(SWRNG_TESTS sha xorshift health usb_serial api_no_device)

foreach(name ${SWRNG_TESTS})
  add_executable(test_${name} test_${name}.cpp)
  target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(test_${name} PRIVATE swiftrng)
  add_test(NAME ${name} COMMAND test_${name})
endforeach()

# The cluster API test also needs the cluster library (and threads)
add_executable(test_cl_api test_cl_api.cpp)
target_include_directories(test_cl_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_cl_api PRIVATE swrng-cl-api)
add_test(NAME cl_api COMMAND test_cl_api)

# Tests of the OpenSSL provider. They load a stand-in build of the provider (a repeatable byte stream instead of a
# device), so they need no hardware. Each test program has its own OpenSSL configuration, so each is separate.
if(SWRNG_BUILD_PROVIDER)
  add_library(swiftrng_provider_sim MODULE
    ${CMAKE_CURRENT_SOURCE_DIR}/../provider/swiftrng_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rng_backend_sim.cpp)
  set_target_properties(swiftrng_provider_sim PROPERTIES PREFIX "" OUTPUT_NAME swiftrng_sim SUFFIX "${SWRNG_PROVIDER_SUFFIX}")
  target_include_directories(swiftrng_provider_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../provider)
  target_link_libraries(swiftrng_provider_sim PRIVATE OpenSSL::Crypto)

  set(SWRNG_PROVIDER_TESTS provider_config provider_threads provider_first_call provider_api provider_unset provider_read_fail provider_open_fail)
  foreach(name ${SWRNG_PROVIDER_TESTS})
    add_executable(test_${name} test_${name}.cpp)
    target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_${name} PRIVATE OpenSSL::Crypto Threads::Threads)
    target_compile_definitions(test_${name} PRIVATE SWRNG_SIM_PROVIDER_PATH="$<TARGET_FILE:swiftrng_provider_sim>")
    add_dependencies(test_${name} swiftrng_provider_sim)
    add_test(NAME ${name} COMMAND test_${name})
  endforeach()
endif()
