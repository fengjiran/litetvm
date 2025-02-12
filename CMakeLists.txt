cmake_minimum_required(VERSION 3.22)
project(litetvm)

set(CMAKE_CXX_STANDARD 20)

# Set custom Alloc Alignment for device allocated memory ndarray points to
set(USE_KALLOC_ALIGNMENT 64)

if(USE_KALLOC_ALIGNMENT)
    message(STATUS "Build Alloc alignment set to ${USE_KALLOC_ALIGNMENT}")
    add_definitions(-DTVM_KALLOC_ALIGNMENT=${USE_KALLOC_ALIGNMENT})
endif(USE_KALLOC_ALIGNMENT)

find_package(glog REQUIRED)

set(DMLC_PATH "3rdparty/dmlc-core/include")
set(BACKTRACE_PATH "3rdparty/libbacktrace")

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/src/*.cc
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${DMLC_PATH}
        ${BACKTRACE_PATH}
)

target_link_libraries(${PROJECT_NAME}
        glog::glog
        pthread
)

add_subdirectory(test)
