# #############################################################################
# Copyright (C) 2016 - 2022 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# #############################################################################

find_package( Boost COMPONENTS program_options REQUIRED)
set( Boost_USE_STATIC_LIBS OFF )

find_package( FFTW 3.0 REQUIRED MODULE COMPONENTS FLOAT DOUBLE )

set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

set( hipfft-test_source
  gtest_main.cpp
  hipfft_accuracy_test.cpp
  simple_test.cpp
  accuracy_test_1D.cpp
  accuracy_test_2D.cpp
  accuracy_test_3D.cpp
  accuracy_test_callback.cpp
  )

set( hipfft-test_includes
  ../rocFFT/clients/tests/fftw_transform.h
  ../rocFFT/clients/tests/rocfft_against_fftw.h
  ../rocFFT/clients/tests/misc/include/test_exception.h
  )

add_executable( hipfft-test ${hipfft-test_source} ${hipfft-test_includes} )

target_include_directories(
  hipfft-test
  PRIVATE
  $<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${FFTW_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${hip_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../library/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../rocFFT/library/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../rocFFT/clients/tests>
  )


if( GTEST_FOUND )
  target_include_directories( hipfft-test PRIVATE $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}> )
  target_link_libraries( hipfft-test PRIVATE ${GTEST_LIBRARIES} )
else()
  add_dependencies( hipfft-test gtest )
  target_include_directories( hipfft-test PRIVATE hipfft-test_include_dirs ${GTEST_INCLUDE_DIRS} )
  target_link_libraries( hipfft-test PRIVATE ${GTEST_LIBRARIES} )
endif()

target_compile_options( hipfft-test PRIVATE ${WARNING_FLAGS} )

if( NOT CMAKE_CXX_COMPILER MATCHES ".*/hipcc$" )
  if( NOT BUILD_WITH_LIB STREQUAL "CUDA" )
    if( WIN32 )
      find_package( HIP CONFIG REQUIRED )
    else()
      find_package( HIP MODULE REQUIRED )
    endif()
    target_link_libraries( hipfft-test PRIVATE hip::host )
  else()
    target_compile_definitions( hipfft-test PRIVATE __HIP_PLATFORM_NVIDIA__)
    target_include_directories( hipfft-test PRIVATE ${HIP_INCLUDE_DIRS})
  endif()
else()
  if( BUILD_WITH_LIB STREQUAL "CUDA" AND DEFINED ${boost_program_options_VERSION} )
    # NVCC doesn't like linking with files that don't end in .so, so
    # we add a hack to remove the version number as the suffix.
    string(REGEX REPLACE
      \.${boost_program_options_VERSION} ""
      Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE
      ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE})
  endif()
endif()

if ( BUILD_WITH_LIB STREQUAL "CUDA" )
  target_link_libraries( hipfft-test  PRIVATE ${CUDA_LIBRARIES} )
  target_compile_definitions( hipfft-test PUBLIC _CUFFT_BACKEND )
endif()

if(FFTW_MULTITHREAD)
  target_compile_options( hipfft-test PRIVATE -DFFTW_MULTITHREAD )
endif( )

set_target_properties( hipfft-test PROPERTIES
  DEBUG_POSTFIX "-d"
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  )

target_link_libraries( hipfft-test
  PRIVATE
  ${FFTW_LIBRARIES}
   ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE} 
  Threads::Threads
  )

target_link_libraries( hipfft-test  PRIVATE hip::hipfft )

set_target_properties( hipfft-test
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

rocm_install(TARGETS hipfft-test COMPONENT tests)

if (WIN32)

  # Ensure tests run with HIP DLLs and not anything the driver owns
  # in system32.  Libraries like amdhip64.dll are also in the HIP
  # runtime, and we need run with those.  But the only way to make a
  # same-named DLL override something in system32 is to have it next
  # to the executable.  So copy them in.
  file( GLOB third_party_dlls
    LIST_DIRECTORIES OFF
    CONFIGURE_DEPENDS
    ${HIP_DIR}/bin/*.dll
  )
  foreach( file_i ${third_party_dlls})
    add_custom_command( TARGET hipfft-test POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy ${file_i} ${PROJECT_BINARY_DIR}/staging )
  endforeach( file_i )
endif()
