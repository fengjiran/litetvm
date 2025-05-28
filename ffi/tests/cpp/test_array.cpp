//
// Created by richard on 5/28/25.
//
#include "ffi/container/array.h"
#include "testing_object.h"

#include <gtest/gtest.h>

namespace {
using namespace litetvm::ffi;
using namespace litetvm::ffi::testing;

TEST(Array, basic) {
    Array<TInt> arr = {TInt(11), TInt(12)};
    EXPECT_EQ(arr.capacity(), 2);
    EXPECT_EQ(arr.size(), 2);
    TInt v1 = arr[0];
    EXPECT_EQ(v1->value, 11);
}

}// namespace