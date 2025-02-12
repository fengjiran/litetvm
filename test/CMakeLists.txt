cmake_minimum_required(VERSION 3.22)
project(litetvm_test)
set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)
find_package(glog REQUIRED)

file(GLOB_RECURSE TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${TEST_SRC_FILES})

target_link_libraries(${PROJECT_NAME}
        litetvm
        GTest::gtest_main
        glog::glog
)
