//
// Created by richard on 5/24/25.
//
#include "ffi/cast.h"
#include "ffi/container/map.h"
#include "ffi/memory.h"
#include "ffi/object.h"
#include "testing_object.h"

#include <gtest/gtest.h>

using namespace litetvm::ffi;
using namespace litetvm::ffi::testing;

TEST(Cast, GetRef) {
    auto a = make_object<TIntObj>(10);
    auto ref_a = GetRef<TInt>(a.get());
    EXPECT_EQ(ref_a.use_count(), 2);
}

TEST(Cast, GetObjectPtr) {
    auto a = make_object<TIntObj>(10);
    auto p = GetObjectPtr<TNumberObj>(a.get());
    EXPECT_EQ(a.use_count(), 2);
}