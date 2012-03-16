cmake_minimum_required(VERSION 2.8)

project(tthread)

file(GLOB TTHREAD_HEADERS *.h)
file(GLOB TTHREAD_SOURCES *.cpp)

add_library(tthread STATIC ${TTHREAD_HEADERS} ${TTHREAD_SOURCES})

set(BUILD_TESTS TRUE CACHE BOOL "Build unit tests")

if(BUILD_TESTS)
  include_directories(.)

  add_executable(test    tests/test.cpp)
  target_link_libraries(test tthread)

  add_executable(hello   tests/hello.cpp)
  target_link_libraries(hello tthread)

  add_executable(fractal tests/fractal.cpp)
  target_link_libraries(fractal tthread)

  add_executable(future tests/future.cpp)
  target_link_libraries(future tthread)
endif()
