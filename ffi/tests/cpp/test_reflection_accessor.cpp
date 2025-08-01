//
// Created by 赵丹 on 25-6-6.
//
#include "ffi/container/map.h"
#include "ffi/object.h"
#include "ffi/reflection/accessor.h"
#include "ffi/reflection/registry.h"
#include "testing_object.h"

#include <gtest/gtest.h>

namespace {

using namespace litetvm::ffi;
using namespace litetvm::ffi::testing;

struct TestObjA : Object {
    int64_t x;
    int64_t y;

    static constexpr const char* _type_key = "test.TestObjA";
    static constexpr bool _type_mutable = true;
    TVM_FFI_DECLARE_BASE_OBJECT_INFO(TestObjA, Object);
};

struct TestObjADerived : TestObjA {
    int64_t z;

    static constexpr const char* _type_key = "test.TestObjADerived";
    // TVM_FFI_DECLARE_FINAL_OBJECT_INFO(TestObjADerived, TestObjA);
    static constexpr int _type_child_slots = 0;
    // static constexpr bool _type_final = true;
    TVM_FFI_DECLARE_BASE_OBJECT_INFO(TestObjADerived, TestObjA);
};

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;

    TIntObj::RegisterReflection();
    TFloatObj::RegisterReflection();
    TPrimExprObj::RegisterReflection();
    TVarObj::RegisterReflection();
    TFuncObj::RegisterReflection();
    TCustomFuncObj::RegisterReflection();

    refl::ObjectDef<TestObjA>().def_ro("x", &TestObjA::x).def_rw("y", &TestObjA::y);
    refl::ObjectDef<TestObjADerived>().def_ro("z", &TestObjADerived::z);
});

TEST(Reflection, GetFieldByteOffset) {
    EXPECT_EQ(details::ObjectUnsafe::GetObjectOffsetToSubclass<TNumberObj>(), 32);
    EXPECT_EQ(reflection::GetFieldByteOffsetToObject(&TestObjA::x), sizeof(TVMFFIObject));
    EXPECT_EQ(reflection::GetFieldByteOffsetToObject(&TestObjA::y), 8 + sizeof(TVMFFIObject));
    EXPECT_EQ(reflection::GetFieldByteOffsetToObject(&TIntObj::value), sizeof(TVMFFIObject));
}

TEST(Reflection, FieldGetter) {
    ObjectRef a = TInt(10);
    reflection::FieldGetter getter("test.Int", "value");
    EXPECT_EQ(getter(a).cast<int>(), 10);

    ObjectRef b = TFloat(10.0);
    reflection::FieldGetter getter_float("test.Float", "value");
    EXPECT_EQ(getter_float(b).cast<double>(), 10.0);
}

TEST(Reflection, FieldSetter) {
    ObjectRef a = TFloat(10.0);
    reflection::FieldSetter setter("test.Float", "value");
    setter(a, 20.0);
    EXPECT_EQ(a.as<TFloatObj>()->value, 20.0);
}

TEST(Reflection, FieldInfo) {
    const TVMFFIFieldInfo* info_int = reflection::GetFieldInfo("test.Int", "value");
    EXPECT_FALSE(info_int->flags & kTVMFFIFieldFlagBitMaskHasDefault);
    EXPECT_FALSE(info_int->flags & kTVMFFIFieldFlagBitMaskWritable);
    EXPECT_EQ(Bytes(info_int->doc).operator std::string(), "");

    const TVMFFIFieldInfo* info_float = reflection::GetFieldInfo("test.Float", "value");
    EXPECT_EQ(info_float->default_value.v_float64, 10.0);
    EXPECT_TRUE(info_float->flags & kTVMFFIFieldFlagBitMaskHasDefault);
    EXPECT_FALSE(info_float->flags & kTVMFFIFieldFlagBitMaskWritable);
    EXPECT_EQ(Bytes(info_float->doc).operator std::string(), "float value field");

    const TVMFFIFieldInfo* info_prim_expr_dtype = reflection::GetFieldInfo("test.PrimExpr", "dtype");
    AnyView default_value = AnyView::CopyFromTVMFFIAny(info_prim_expr_dtype->default_value);
    EXPECT_EQ(default_value.cast<String>(), "float");
    EXPECT_EQ(default_value.as<String>().value().use_count(), 2);
    EXPECT_TRUE(info_prim_expr_dtype->flags & kTVMFFIFieldFlagBitMaskHasDefault);
    EXPECT_TRUE(info_prim_expr_dtype->flags & kTVMFFIFieldFlagBitMaskWritable);
    EXPECT_EQ(Bytes(info_prim_expr_dtype->doc).operator std::string(), "dtype field");
}

TEST(Reflection, MethodInfo) {
    const TVMFFIMethodInfo* info_int_static_add = reflection::GetMethodInfo("test.Int", "static_add");
    EXPECT_TRUE(info_int_static_add->flags & kTVMFFIFieldFlagBitMaskIsStaticMethod);
    EXPECT_EQ(Bytes(info_int_static_add->doc).operator std::string(), "static add method");

    const TVMFFIMethodInfo* info_float_add = reflection::GetMethodInfo("test.Float", "add");
    EXPECT_FALSE(info_float_add->flags & kTVMFFIFieldFlagBitMaskIsStaticMethod);
    EXPECT_EQ(Bytes(info_float_add->doc).operator std::string(), "add method");

    const TVMFFIMethodInfo* info_float_sub = reflection::GetMethodInfo("test.Float", "sub");
    EXPECT_FALSE(info_float_sub->flags & kTVMFFIFieldFlagBitMaskIsStaticMethod);
    EXPECT_EQ(Bytes(info_float_sub->doc).operator std::string(), "");
}

TEST(Reflection, CallMethod) {
    Function static_int_add = reflection::GetMethod("test.Int", "static_add");
    EXPECT_EQ(static_int_add(TInt(1), TInt(2)).cast<TInt>()->value, 3);

    Function float_add = reflection::GetMethod("test.Float", "add");
    EXPECT_EQ(float_add(TFloat(1), 2.0).cast<double>(), 3.0);

    Function float_sub = reflection::GetMethod("test.Float", "sub");
    EXPECT_EQ(float_sub(TFloat(1), 2.0).cast<double>(), -1.0);

    Function prim_expr_sub = reflection::GetMethod("test.PrimExpr", "sub");
    EXPECT_EQ(prim_expr_sub(TPrimExpr("float", 1), 2.0).cast<double>(), -1.0);
}

TEST(Reflection, ForEachFieldInfo) {
    const TypeInfo* info = TVMFFIGetTypeInfo(TestObjADerived::RuntimeTypeIndex());
    Map<String, int> field_name_to_offset;
    reflection::ForEachFieldInfo(info, [&](const TVMFFIFieldInfo* field_info) {
        field_name_to_offset.Set(String(field_info->name), field_info->offset);
    });
    EXPECT_EQ(field_name_to_offset["x"], sizeof(TVMFFIObject));
    EXPECT_EQ(field_name_to_offset["y"], 8 + sizeof(TVMFFIObject));
    EXPECT_EQ(field_name_to_offset["z"], 16 + sizeof(TVMFFIObject));
}

TEST(Reflection, TypeAttrColumn) {
    reflection::TypeAttrColumn size_attr("test.size");
    EXPECT_EQ(size_attr[TIntObj::_type_index].cast<int>(), sizeof(TIntObj));
}

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef().def_method("testing.Int_GetValue", &TIntObj::GetValue);
});

TEST(Reflection, FuncRegister) {
    Function fget_value = Function::GetGlobalRequired("testing.Int_GetValue");
    TInt a(12);
    EXPECT_EQ(fget_value(a).cast<int>(), 12);
}

}// namespace