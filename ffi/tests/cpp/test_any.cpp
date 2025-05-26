//
// Created by 赵丹 on 25-5-26.
//
#include "ffi/any.h"
#include "testing_object.h"

#include <gtest/gtest.h>

namespace {
using namespace litetvm::ffi;
using namespace litetvm::ffi::testing;

TEST(Any, Int) {
    AnyView view0;
    EXPECT_EQ(view0.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFINone);

    Optional<int64_t> opt_v0 = view0.as<int64_t>();
    EXPECT_TRUE(!opt_v0.has_value());

    EXPECT_THROW(
            {
                try {
                    MAYBE_UNUSED auto v0 = view0.cast<int>();
                } catch (const Error& error) {
                    EXPECT_EQ(error.kind(), "TypeError");
                    std::string what = error.what();
                    EXPECT_NE(what.find("Cannot convert from type `None` to `int`"), std::string::npos);
                    throw;
                }
            },
            ::litetvm::ffi::Error);

    AnyView view1 = 1;
    EXPECT_EQ(view1.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFIInt);
    EXPECT_EQ(view1.CopyToTVMFFIAny().v_int64, 1);

    auto int_v1 = view1.cast<int>();
    EXPECT_EQ(int_v1, 1);

    int64_t v1 = 2;
    view0 = v1;
    EXPECT_EQ(view0.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFIInt);
    EXPECT_EQ(view0.CopyToTVMFFIAny().v_int64, 2);
}

TEST(Any, bool) {
    AnyView view0;
    Optional<bool> opt_v0 = view0.as<bool>();
    EXPECT_TRUE(!opt_v0.has_value());

    EXPECT_THROW(
            {
                try {
                    MAYBE_UNUSED auto v0 = view0.cast<bool>();
                } catch (const Error& error) {
                    EXPECT_EQ(error.kind(), "TypeError");
                    std::string what = error.what();
                    EXPECT_NE(what.find("Cannot convert from type `None` to `bool`"), std::string::npos);
                    throw;
                }
            },
            ::litetvm::ffi::Error);

    AnyView view1 = true;
    EXPECT_EQ(view1.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFIBool);
    EXPECT_EQ(view1.CopyToTVMFFIAny().v_int64, 1);

    auto int_v1 = view1.cast<int>();
    EXPECT_EQ(int_v1, 1);

    bool v1 = false;
    view0 = v1;
    EXPECT_EQ(view0.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFIBool);
    EXPECT_EQ(view0.CopyToTVMFFIAny().v_int64, 0);
}

TEST(Any, nullptrcmp) {
    AnyView view0;
    EXPECT_EQ(view0.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFINone);
    EXPECT_TRUE(view0 == nullptr);
    EXPECT_FALSE(view0 != nullptr);

    view0 = 1;
    EXPECT_TRUE(view0 != nullptr);
    EXPECT_FALSE(view0 == nullptr);

    Any any0 = view0;
    EXPECT_TRUE(any0 != nullptr);
    EXPECT_FALSE(any0 == nullptr);

    any0 = nullptr;
    EXPECT_TRUE(any0 == nullptr);
    EXPECT_FALSE(any0 != nullptr);
}

TEST(Any, Float) {
    AnyView view0;
    EXPECT_EQ(view0.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFINone);

    Optional<double> opt_v0 = view0.as<double>();
    EXPECT_TRUE(!opt_v0.has_value());

    EXPECT_THROW(
            {
                try {
                    MAYBE_UNUSED auto v0 = view0.cast<double>();
                } catch (const Error& error) {
                    EXPECT_EQ(error.kind(), "TypeError");
                    std::string what = error.what();
                    EXPECT_NE(what.find("Cannot convert from type `None` to `float`"), std::string::npos);
                    throw;
                }
            },
            ::litetvm::ffi::Error);

    AnyView view1_int = 1;
    auto float_v1 = view1_int.cast<float>();
    EXPECT_EQ(float_v1, 1);

    AnyView view2 = 2.2;
    EXPECT_EQ(view2.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFIFloat);
    EXPECT_EQ(view2.CopyToTVMFFIAny().v_float64, 2.2);

    float v1 = 2;
    view0 = v1;
    EXPECT_EQ(view0.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFIFloat);
    EXPECT_EQ(view0.CopyToTVMFFIAny().v_float64, 2);
}

TEST(Any, Device) {
    AnyView view0;
    EXPECT_EQ(view0.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFINone);

    Optional<DLDevice> opt_v0 = view0.as<DLDevice>();
    EXPECT_TRUE(!opt_v0.has_value());

    EXPECT_THROW(
            {
                try {
                    MAYBE_UNUSED auto v0 = view0.cast<DLDevice>();
                } catch (const Error& error) {
                    EXPECT_EQ(error.kind(), "TypeError");
                    std::string what = error.what();
                    EXPECT_NE(what.find("Cannot convert from type `None` to `Device`"), std::string::npos);
                    throw;
                }
            },
            ::litetvm::ffi::Error);

    DLDevice device{kDLCUDA, 1};

    AnyView view1_device = device;
    auto dtype_v1 = view1_device.cast<DLDevice>();
    EXPECT_EQ(dtype_v1.device_type, kDLCUDA);
    EXPECT_EQ(dtype_v1.device_id, 1);

    Any any2 = DLDevice{kDLCPU, 0};
    TVMFFIAny ffi_v2 = details::AnyUnsafe::MoveAnyToTVMFFIAny(std::move(any2));
    EXPECT_EQ(ffi_v2.type_index, TypeIndex::kTVMFFIDevice);
    EXPECT_EQ(ffi_v2.v_device.device_type, kDLCPU);
    EXPECT_EQ(ffi_v2.v_device.device_id, 0);
}

TEST(Any, DLTensor) {
    AnyView view0;

    Optional<DLTensor*> opt_v0 = view0.as<DLTensor*>();
    EXPECT_TRUE(!opt_v0.has_value());

    EXPECT_THROW(
            {
                try {
                    MAYBE_UNUSED auto v0 = view0.cast<DLTensor*>();
                } catch (const Error& error) {
                    EXPECT_EQ(error.kind(), "TypeError");
                    std::string what = error.what();
                    EXPECT_NE(what.find("Cannot convert from type `None` to `DLTensor*`"), std::string::npos);
                    throw;
                }
            },
            ::litetvm::ffi::Error);

    DLTensor dltensor;

    AnyView view1_dl = &dltensor;
    auto dl_v1 = view1_dl.cast<DLTensor*>();
    EXPECT_EQ(dl_v1, &dltensor);
}

TEST(Any, Object) {
    AnyView view0;
    EXPECT_EQ(view0.CopyToTVMFFIAny().type_index, TypeIndex::kTVMFFINone);

    // int object is not nullable
    Optional<TInt> opt_v0 = view0.as<TInt>();
    EXPECT_TRUE(!opt_v0.has_value());

    TInt v1(11);
    EXPECT_EQ(v1.use_count(), 1);

    // view won't increase refcount
    AnyView view1 = v1;
    EXPECT_EQ(v1.use_count(), 1);

    // any will trigger ref count increase
    Any any1 = v1;
    EXPECT_EQ(v1.use_count(), 2);

    // copy to another view
    AnyView view2 = any1;
    EXPECT_EQ(v1.use_count(), 2);

    // convert to weak raw object ptr
    const auto* v1_ptr = view2.cast<const TIntObj*>();
    EXPECT_EQ(v1.use_count(), 2);
    EXPECT_EQ(v1_ptr->value, 11);
    Any any2 = v1_ptr;
    EXPECT_EQ(v1.use_count(), 3);
    EXPECT_TRUE(any2.as<TInt>().has_value());

    // convert to raw opaque ptr
    void* raw_v1_ptr = const_cast<TIntObj*>(v1_ptr);
    any2 = raw_v1_ptr;
    EXPECT_TRUE(any2.as<void*>().value() == v1_ptr);

    // convert to ObjectRef
    {
        auto v1_obj_ref = view2.cast<TNumber>();
        EXPECT_EQ(v1.use_count(), 3);
        any2 = v1_obj_ref;
        EXPECT_EQ(v1.use_count(), 4);
        EXPECT_TRUE(any2.as<TInt>().has_value());
        any2.reset();
    }

    // convert that triggers error
    EXPECT_THROW(
            {
                try {
                    MAYBE_UNUSED auto v0 = view1.cast<TFloat>();
                } catch (const Error& error) {
                    EXPECT_EQ(error.kind(), "TypeError");
                    std::string what = error.what();
                    std::cout << what;
                    EXPECT_NE(what.find("Cannot convert from type `test.Int` to `test.Float`"),
                              std::string::npos);
                    throw;
                }
            },
            ::litetvm::ffi::Error);

    // Try to convert to number
    auto number0 = any1.cast<TNumber>();
    EXPECT_EQ(v1.use_count(), 3);
    EXPECT_TRUE(number0.as<TIntObj>());
    EXPECT_EQ(number0.as<TIntObj>()->value, 11);
    EXPECT_TRUE(!any1.as<int>().has_value());

    auto int1 = view2.cast<TInt>();
    EXPECT_EQ(v1.use_count(), 4);
    any1.reset();
    EXPECT_EQ(v1.use_count(), 3);
}

}// namespace