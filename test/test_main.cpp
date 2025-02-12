//
// Created by richard on 4/29/24.
//
#include "glog/logging.h"
#include "gtest/gtest.h"

int main() {
    testing::InitGoogleTest();
    google::InitGoogleLogging("openXAE_test");
    FLAGS_logtostderr = true;
    return RUN_ALL_TESTS();
}