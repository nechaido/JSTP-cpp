cmake_minimum_required(VERSION 3.4)
project(JSTP_cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES jsrs.cc jsrs.h deps.h jsrm.cc jsrm.h)

add_library (jsrs STATIC ${SOURCE_FILES})

add_subdirectory(lib/g-test-1.7.0)

include_directories(${gtest_SOURCE_DIR}/include)

add_executable(tests jsrs_test.cc)

target_link_libraries(tests gtest gtest_main)
target_link_libraries(tests jsrs)