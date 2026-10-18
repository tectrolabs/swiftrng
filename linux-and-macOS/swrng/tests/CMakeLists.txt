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
