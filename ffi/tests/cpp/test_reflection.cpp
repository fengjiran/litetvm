//
// Created by 赵丹 on 25-6-6.
//
#include "ffi/object.h"
#include "ffi/reflection/reflection.h"
#include "testing_object.h"

#include <gtest/gtest.h>

namespace {

using namespace litetvm::ffi;
using namespace litetvm::ffi::testing;

struct A : public Object {
    int64_t x;
    int64_t y;
};

TEST(Reflection, GetFieldByteOffset) {
    EXPECT_EQ(details::GetFieldByteOffsetToObject(&A::x), sizeof(TVMFFIObject));
    EXPECT_EQ(details::GetFieldByteOffsetToObject(&A::y), 8 + sizeof(TVMFFIObject));
    EXPECT_EQ(details::GetFieldByteOffsetToObject(&TIntObj::value), sizeof(TVMFFIObject));
}

TEST(Reflection, FieldGetter) {
    ObjectRef a = TInt(10);
    details::ReflectionFieldGetter getter(details::GetReflectionFieldInfo("test.Int", "value"));
    EXPECT_EQ(getter(a).cast<int>(), 10);

    ObjectRef b = TFloat(10.0);
    details::ReflectionFieldGetter getter_float(
            details::GetReflectionFieldInfo("test.Float", "value"));
    EXPECT_EQ(getter_float(b).cast<double>(), 10.0);
}

}// namespace