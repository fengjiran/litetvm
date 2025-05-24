//
// Created by 赵丹 on 25-5-23.
//
#include "ffi/memory.h"
#include "ffi/object.h"
#include "testing_object.h"

#include <gtest/gtest.h>

using namespace litetvm::ffi;
using namespace litetvm::ffi::testing;

TEST(Object, RefCounter) {
    auto a = make_object<TIntObj>(10);
    ObjectPtr<TIntObj> b = a;
    EXPECT_EQ(a->value, 10);
    EXPECT_EQ(a.use_count(), 2);

    auto aa = make_object<TIntObj>(11);
    EXPECT_EQ(aa.use_count(), 1);
    EXPECT_EQ(aa->value, 11);

    b.reset();
    EXPECT_EQ(a.use_count(), 1);
    EXPECT_TRUE(b == nullptr);
    EXPECT_EQ(b.use_count(), 0);

    ObjectPtr<TIntObj> c = std::move(a);
    EXPECT_EQ(c.use_count(), 1);
    EXPECT_TRUE(a == nullptr);
    EXPECT_EQ(c->value, 10);
}

TEST(Object, TypeInfo) {
    const TypeInfo* info = TVMFFIGetTypeInfo(TIntObj::RuntimeTypeIndex());
    EXPECT_TRUE(info != nullptr);
    EXPECT_EQ(info->type_index, TIntObj::RuntimeTypeIndex());
    EXPECT_EQ(info->type_depth, 2);
    EXPECT_EQ(info->type_acenstors[0], Object::_type_index);
    EXPECT_EQ(info->type_acenstors[1], TNumberObj::_type_index);
    EXPECT_GE(info->type_index, TypeIndex::kTVMFFIDynObjectBegin);
}

TEST(Object, InstanceCheck) {
    ObjectPtr<Object> a = make_object<TIntObj>(11);
    ObjectPtr<Object> b = make_object<TFloatObj>(11);

    EXPECT_TRUE(a->IsInstance<Object>());
    EXPECT_TRUE(a->IsInstance<TNumberObj>());
    EXPECT_TRUE(a->IsInstance<TIntObj>());
    EXPECT_TRUE(!a->IsInstance<TFloatObj>());

    EXPECT_TRUE(a->IsInstance<Object>());
    EXPECT_TRUE(b->IsInstance<TNumberObj>());
    EXPECT_TRUE(!b->IsInstance<TIntObj>());
    EXPECT_TRUE(b->IsInstance<TFloatObj>());
}

TEST(Object, as) {
    ObjectRef a = TInt(10);
    ObjectRef b = TFloat(20);
    // nullable object
    ObjectRef c(nullptr);

    EXPECT_TRUE(a.as<TIntObj>() != nullptr);
    EXPECT_TRUE(a.as<TFloatObj>() == nullptr);
    EXPECT_TRUE(a.as<TNumberObj>() != nullptr);

    EXPECT_TRUE(b.as<TIntObj>() == nullptr);
    EXPECT_TRUE(b.as<TFloatObj>() != nullptr);
    EXPECT_TRUE(b.as<TNumberObj>() != nullptr);

    EXPECT_TRUE(c.as<TIntObj>() == nullptr);
    EXPECT_TRUE(c.as<TFloatObj>() == nullptr);
    EXPECT_TRUE(c.as<TNumberObj>() == nullptr);

    EXPECT_EQ(a.as<TIntObj>()->value, 10);
    EXPECT_EQ(b.as<TFloatObj>()->value, 20);
}

TEST(Object, CAPIAccessor) {
    ObjectRef a = TInt(10);
    TVMFFIObjectHandle obj = details::ObjectUnsafe::RawObjectPtrFromObjectRef(a);
    int32_t type_index = TVMFFIObjectGetTypeIndex(obj);
    EXPECT_EQ(type_index, TIntObj::RuntimeTypeIndex());
}
