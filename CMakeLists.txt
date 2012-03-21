cmake_minimum_required(VERSION 2.8)

project(tthread)

if(MSVC10)
  set(tthread_SUFFIX _vc10)
elseif(MSVC11)
  set(tthread_SUFFIX _vc11)
else()
  set(tthread_SUFFIX)
endif()

if(CMAKE_SIZEOF_VOID_P MATCHES 4)
  set(tthread_SIZE_TYPE x86)
else()
  set(tthread_SIZE_TYPE x64)
endif()

set(tthread_BINARY_PATH ${CMAKE_HOME_DIRECTORY}/bin/${tthread_SIZE_TYPE}${tthread_SUFFIX})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${tthread_BINARY_PATH}
    CACHE PATH
    "Single Directory for all Executables.")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${tthread_BINARY_PATH}
    CACHE PATH
    "Single Directory for all Libraries")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${tthread_LIBRARY_PATH}
    CACHE PATH
    "Single Directory for all static libraries.")

SET(TREAT_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat compiler warnings as errors")

if(UNIX OR MINGW)
  set(STOP_ON_FIRST_ERROR TRUE CACHE BOOL "Stop compilation on first error")
  if(TREAT_WARNINGS_AS_ERRORS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  endif()
  if (STOP_ON_FIRST_ERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfatal-errors")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -std=c++0x -Wall -Wno-return-type")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-deprecated")
    add_definitions( "-DDEBUG" )
  elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer")
  elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithProfile")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -g -O3 -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg -fPIC")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-unused-variable -Wno-write-strings -fno-strict-aliasing  -Wno-parentheses -Wno-deprecated -fno-omit-frame-pointer")
  endif()
endif()

if(MSVC)
  SET(LINK_STATIC_RUNTIME OFF CACHE BOOL "Link statically against C++ runtime")
  if(TREAT_WARNINGS_AS_ERRORS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNOMINMAX /MP /W3 /EHa")
  if(MSVC10 OR MSVC11)
    SET(ITERATOR_DEBUG TRUE CACHE BOOL "Use iterator debug level 2")
    #add_definitions("-D_ITERATOR_DEBUG_LEVEL=0")
    if (ITERATOR_DEBUG)
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=2")
    else()
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=0")
    endif()
    #add_definitions("-D_ITERATOR_DEBUG_LEVEL=0")
  endif()
  # Extra speed optimisation options
  set(MSVC_EXTRA_OPTIMIZE ON CACHE BOOL "Use extra optimization flags in release builds")
  if(MSVC_EXTRA_OPTIMIZE)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oi /Ot /Oy /GL /GS- /fp:fast")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oi /Ot /Oy /GL /GS- /fp:fast")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /LTCG")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /LTCG")
  endif()
  # Even more speed optimisations
  set(MSVC_OPTIMIZE_SSE2 ON CACHE BOOL "Use SSE2 instruction set in release builds")
  if(MSVC_OPTIMIZE_SSE2)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /arch:SSE2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE2")
  endif()
  if(LINK_STATIC_RUNTIME AND NOT BUILD_SHARED_LIBS)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:msvcrt /NODEFAULTLIB:msvcrtd")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt /NODEFAULTLIB:msvcrtd")
  endif()
endif()


file(GLOB TTHREAD_HEADERS *.h)
file(GLOB TTHREAD_SOURCES *.cpp)

add_library(tthread STATIC ${TTHREAD_HEADERS} ${TTHREAD_SOURCES})
if(UNIX)
  target_link_libraries(tthread pthread)
  set(TTHREAD_LIBS tthread pthread)
else()
  set(TTHREAD_LIBS tthread)
endif()

set(BUILD_TESTS FALSE CACHE BOOL "Build unit tests")

if(BUILD_TESTS)
  include_directories(.)

  add_executable(test    tests/test.cpp)
  target_link_libraries(test ${TTHREAD_LIBS})

  add_executable(hello   tests/hello.cpp)
  target_link_libraries(hello ${TTHREAD_LIBS})

  add_executable(fractal tests/fractal.cpp)
  target_link_libraries(fractal ${TTHREAD_LIBS})

  add_executable(future tests/future.cpp)
  target_link_libraries(future ${TTHREAD_LIBS})

  add_executable(pool tests/pool.cpp)
  target_link_libraries(pool ${TTHREAD_LIBS})
endif()
