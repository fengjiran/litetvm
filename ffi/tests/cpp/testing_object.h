//
// Created by 赵丹 on 25-5-20.
//

#ifndef LITETVM_FFI_TESTING_OBJECT_H
#define LITETVM_FFI_TESTING_OBJECT_H

#include "ffi/memory.h"
#include "ffi/object.h"
#include "ffi/reflection/reflection.h"
#include "ffi/string.h"

namespace litetvm {
namespace ffi {
namespace testing {

// We deliberately pad extra
// in the header to test cases
// where the object subclass address
// do not align with the base object address
// not handling properly will cause buffer overflow
class BasePad {
public:
    int64_t extra[4];
};

class TNumberObj : public BasePad, public Object {
public:
    // declare as one slot, with float as overflow
    static constexpr uint32_t _type_child_slots = 1;
    static constexpr const char* _type_key = "test.Number";
    TVM_FFI_DECLARE_BASE_OBJECT_INFO(TNumberObj, Object);
};

class TNumber : public ObjectRef {
public:
    TVM_FFI_DEFINE_OBJECT_REF_METHODS(TNumber, ObjectRef, TNumberObj);
};

class TIntObj : public TNumberObj {
public:
    int64_t value;

    TIntObj(int64_t value) : value(value) {}

    NODISCARD int64_t GetValue() const {
        return value;
    }

    static constexpr const char* _type_key = "test.Int";

    TVM_FFI_DECLARE_FINAL_OBJECT_INFO(TIntObj, TNumberObj);
};

TVM_FFI_REFLECTION_DEF(TIntObj).def_readonly("value", &TIntObj::value);

class TInt : public TNumber {
public:
    explicit TInt(int64_t value) {
        data_ = make_object<TIntObj>(value);
    }

    TVM_FFI_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(TInt, TNumber, TIntObj);
};

class TFloatObj : public TNumberObj {
public:
    double value;

    TFloatObj(double value) : value(value) {}

    static constexpr const char* _type_key = "test.Float";
    TVM_FFI_DECLARE_FINAL_OBJECT_INFO(TFloatObj, TNumberObj);
};

TVM_FFI_REFLECTION_DEF(TFloatObj).def_readonly("value", &TFloatObj::value);

class TFloat : public TNumber {
public:
    explicit TFloat(double value) {
        data_ = make_object<TFloatObj>(value);
    }

    TVM_FFI_DEFINE_OBJECT_REF_METHODS(TFloat, TNumber, TFloatObj);
};

// TPrimExpr is used for testing FallbackTraits
class TPrimExprObj : public Object {
public:
    std::string dtype;
    double value;

    TPrimExprObj(std::string dtype, double value) : dtype(dtype), value(value) {}

    static constexpr const char* _type_key = "test.PrimExpr";
    TVM_FFI_DECLARE_FINAL_OBJECT_INFO(TPrimExprObj, Object);
};

class TPrimExpr : public ObjectRef {
public:
    explicit TPrimExpr(std::string dtype, double value) {
        data_ = make_object<TPrimExprObj>(dtype, value);
    }

    TVM_FFI_DEFINE_OBJECT_REF_METHODS(TPrimExpr, ObjectRef, TPrimExprObj);
};
}// namespace testing

template<>
inline constexpr bool use_default_type_traits_v<testing::TPrimExpr> = true;

template<>
struct TypeTraits<testing::TPrimExpr> : ObjectRefWithFallbackTraitsBase<testing::TPrimExpr, StrictBool, int64_t, double, String> {
    static TVM_FFI_INLINE testing::TPrimExpr ConvertFallbackValue(StrictBool value) {
        return testing::TPrimExpr("bool", value);
    }

    static TVM_FFI_INLINE testing::TPrimExpr ConvertFallbackValue(int64_t value) {
        return testing::TPrimExpr("int64", static_cast<double>(value));
    }

    static TVM_FFI_INLINE testing::TPrimExpr ConvertFallbackValue(double value) {
        return testing::TPrimExpr("float32", value);
    }
    // hack into the dtype to store string
    static TVM_FFI_INLINE testing::TPrimExpr ConvertFallbackValue(String value) {
        return testing::TPrimExpr(value, 0);
    }
};

}// namespace ffi
}// namespace litetvm

#endif//LITETVM_FFI_TESTING_OBJECT_H
