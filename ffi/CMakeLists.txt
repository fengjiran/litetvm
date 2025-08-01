# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

cmake_minimum_required(VERSION 3.14)

project(
        tvm_ffi
        VERSION 1.0
        DESCRIPTION "TVM's FFI system"
        LANGUAGES CXX C
)

option(TVM_FFI_BUILD_TESTS "Adding test targets." ON)
option(TVM_FFI_USE_LIBBACKTRACE "Enable libbacktrace" ON)
option(TVM_FFI_USE_EXTRA_CXX_API "Enable extra CXX API in shared lib" ON)
option(TVM_FFI_BACKTRACE_ON_SEGFAULT "Set signal handler to print traceback on segfault" ON)

include(cmake/Utils/CxxWarning.cmake)
include(cmake/Utils/Sanitizer.cmake)
include(cmake/Utils/Library.cmake)
if (TVM_FFI_USE_LIBBACKTRACE)
    include(cmake/Utils/AddLibbacktrace.cmake)
endif ()

########## Target: `dlpack_header` ##########

add_library(dlpack_header INTERFACE)
target_include_directories(dlpack_header INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dlpack/include")

########## Target: `tvm_ffi_header` ##########

add_library(tvm_ffi_header INTERFACE)
target_include_directories(tvm_ffi_header INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tvm_ffi_header INTERFACE dlpack_header)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

if (TVM_FFI_USE_EXTRA_CXX_API)
    list(APPEND tvm_ffi_objs_sources
            "${CMAKE_CURRENT_SOURCE_DIR}/src/ffi/extra/structural_equal.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/ffi/extra/structural_hash.cpp"
    )
endif()

########## Target: `tvm_ffi` ##########
add_library(tvm_ffi_objs OBJECT ${SRC_FILES})
set_target_properties(
        tvm_ffi_objs PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        PREFIX "lib"
)
add_cxx_warning(tvm_ffi_objs)
target_link_libraries(tvm_ffi_objs PRIVATE dlpack_header)
target_include_directories(tvm_ffi_objs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (TVM_FFI_USE_LIBBACKTRACE)
    message(STATUS "Setting C++ macro TVM_FFI_USE_LIBBACKTRACE - 1")
    target_compile_definitions(tvm_ffi_objs PRIVATE TVM_FFI_USE_LIBBACKTRACE=1)
else ()
    message(STATUS "Setting C++ macro TVM_FFI_USE_LIBBACKTRACE - 0")
    target_compile_definitions(tvm_ffi_objs PRIVATE TVM_FFI_USE_LIBBACKTRACE=0)
endif ()

if (TVM_FFI_BACKTRACE_ON_SEGFAULT)
    message(STATUS "Setting C++ macro TVM_FFI_BACKTRACE_ON_SEGFAULT - 1")
    target_compile_definitions(tvm_ffi_objs PRIVATE TVM_FFI_BACKTRACE_ON_SEGFAULT=1)
else ()
    message(STATUS "Setting C++ macro TVM_FFI_BACKTRACE_ON_SEGFAULT - 0")
    target_compile_definitions(tvm_ffi_objs PRIVATE TVM_FFI_BACKTRACE_ON_SEGFAULT=0)
endif ()

add_target_from_obj(tvm_ffi tvm_ffi_objs)

if (TARGET libbacktrace)
    target_link_libraries(tvm_ffi_objs PRIVATE libbacktrace)
    target_link_libraries(tvm_ffi_shared PRIVATE libbacktrace)
    target_link_libraries(tvm_ffi_static PRIVATE libbacktrace)
endif ()

if (MSVC)
    target_link_libraries(tvm_ffi_objs PRIVATE DbgHelp.lib)
    target_link_libraries(tvm_ffi_shared PRIVATE DbgHelp.lib)
    target_link_libraries(tvm_ffi_static PRIVATE DbgHelp.lib)
endif ()

target_link_libraries(tvm_ffi_objs PUBLIC tvm_ffi_header)
target_link_libraries(tvm_ffi_shared PUBLIC tvm_ffi_header)
target_link_libraries(tvm_ffi_static PUBLIC tvm_ffi_header)

install(TARGETS tvm_ffi_static DESTINATION lib${LIB_SUFFIX})
install(TARGETS tvm_ffi_shared DESTINATION lib${LIB_SUFFIX})

add_msvc_flags(tvm_ffi_objs)

########## Adding tests ##########
if (TVM_FFI_BUILD_TESTS)
    enable_testing()
    message(STATUS "Enable Testing")
    include(cmake/Utils/AddGoogleTest.cmake)
    add_subdirectory(tests/cpp/)
endif ()
