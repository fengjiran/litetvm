//
// Created by richard on 3/6/25.
//
#include "ir/expr.h"

#include "runtime/memory.h"
#include "runtime/registry.h"
#include "support/scalars.h"
#include "tir/expr.h"
#include "tir/op.h"
#include <utility>

namespace litetvm {

// using runtime::make_object;

PrimExpr::PrimExpr(int32_t value) : PrimExpr(IntImm(DataType::Int(32), value)) {}
//
PrimExpr::PrimExpr(float value) : PrimExpr(FloatImm(DataType::Float(32), value)) {}

IntImm::IntImm(DataType dtype, int64_t value) {
    CHECK(dtype.is_scalar()) << "ValueError: IntImm can only take scalar, but " << dtype << " was supplied.";
    CHECK(dtype.is_int() || dtype.is_uint()) << "ValueError: IntImm supports only int or uint type, but " << dtype << " was supplied.";

    if (dtype.is_uint()) {
        CHECK_GE(value, 0U) << "ValueError: Literal value " << value << " is negative for unsigned integer type " << dtype;
        if (dtype.bits() < 64) {
            CHECK_LT(value, 1LL << dtype.bits()) << "ValueError: Literal value " << value << " exceeds maximum of " << dtype;
        }
    } else if (dtype.bits() == 1) {
        // int(1)
        CHECK(value == 0 || value == 1) << "ValueError: " << value << " exceeds range of " << dtype;
    } else if (dtype.bits() < 64) {
        CHECK_GE(value, -(1LL << (dtype.bits() - 1))) << "ValueError: Literal value " << value << " exceeds minimum of " << dtype;
        CHECK_LT(value, 1LL << (dtype.bits() - 1)) << "ValueError: Literal value " << value << " exceeds maximum of " << dtype;
    }

    ObjectPtr<IntImmNode> node = make_object<IntImmNode>();
    node->dtype = dtype;
    node->value = value;
    // node->span = span;
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("ir.IntImm").set_body_typed([](DataType dtype, int64_t value) {
    return IntImm(dtype, value);
});

TVM_REGISTER_NODE_TYPE(IntImmNode);

FloatImm::FloatImm(DataType dtype, double value) {
    CHECK_EQ(dtype.lanes(), 1) << "ValueError: FloatImm can only take scalar.";

    CHECK(dtype.is_float() || dtype.is_bfloat16() || dtype.is_float8() || dtype.is_float4() ||
          dtype.code() >= static_cast<int>(DataType::TypeCode::kCustomBegin))
            << "ValueError: FloatImm supports only float, but " << dtype << " was supplied.";

    // check range for float32 and float16 since they have specified range.
    if (!std::isinf(value) && !std::isnan(value)) {
        if (dtype.bits() == 32) {
            CHECK_GE(value, std::numeric_limits<float>::lowest()) << "ValueError: Literal value " << value << " exceeds minimum of " << dtype;
            CHECK_LE(value, std::numeric_limits<float>::max()) << "ValueError: Literal value " << value << " exceeds maximum of " << dtype;
        } else if (dtype.is_float16()) {
            CHECK_GE(value, -support::kMaxFloat16) << "ValueError: Literal value " << value << " exceeds minimum of " << dtype;
            CHECK_LE(value, support::kMaxFloat16) << "ValueError: Literal value " << value << " exceeds maximum of " << dtype;
        } else if (dtype.is_bfloat16()) {
            CHECK_GE(value, -support::kMaxBFloat16) << "ValueError: Literal value " << value << " exceeds minimum of " << dtype;
            CHECK_LE(value, support::kMaxBFloat16) << "ValueError: Literal value " << value << " exceeds maximum of " << dtype;
        } else if (dtype.is_float8()) {
            double bound = dtype.code() == static_cast<int>(DataType::TypeCode::kFloat8_e4m3fn) ? support::kMaxE4M3FN : support::kMaxE5M2;
            CHECK_GE(value, -bound) << "ValueError: Literal value " << value << " exceeds minimum of " << dtype;
            CHECK_LE(value, bound) << "ValueError: Literal vaule " << value << " exceeds maximum of " << dtype;
        } else if (dtype.is_float4()) {
            CHECK_GE(value, -support::kMaxE2M1FN) << "ValueError: Literal value " << value << " exceeds minimum of " << dtype;
            CHECK_LE(value, support::kMaxE2M1FN) << "ValueError: Literal value " << value << " exceeds maximum of " << dtype;
        }
    }
    ObjectPtr<FloatImmNode> node = make_object<FloatImmNode>();
    node->dtype = dtype;
    node->value = value;
    // node->span = span;
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("ir.FloatImm").set_body_typed([](DataType dtype, double value) {
    return FloatImm(dtype, value);
});

TVM_REGISTER_NODE_TYPE(FloatImmNode);

Range::Range(PrimExpr begin, PrimExpr end) {
    auto extent = tir::is_zero(begin) ? end : end - begin;
    auto n = make_object<RangeNode>(begin, extent);
    data_ = std::move(n);
}

Range Range::FromMinExtent(PrimExpr min, PrimExpr extent) {
    return Range(make_object<RangeNode>(std::move(min), std::move(extent)));
}

TVM_REGISTER_NODE_TYPE(RangeNode);

TVM_REGISTER_GLOBAL("ir.Range_from_min_extent").set_body_typed(Range::FromMinExtent);

TVM_REGISTER_GLOBAL("ir.Range").set_body_typed([](PrimExpr begin, Optional<PrimExpr> end) -> Range {
    if (end.defined()) {
        return {begin, end.value()};
    }
    return {IntImm(begin->dtype, 0), begin};
});

GlobalVar::GlobalVar(String name_hint, Type type) {
    auto n = make_object<GlobalVarNode>();
    n->name_hint = std::move(name_hint);
    n->checked_type_ = std::move(type);
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(GlobalVarNode);

TVM_REGISTER_GLOBAL("ir.GlobalVar").set_body_typed([](String name, Type type) {
    return GlobalVar(std::move(name), std::move(type));
});

TVM_REGISTER_GLOBAL("ir.DebugPrint").set_body_typed([](ObjectRef ref) {
    std::stringstream ss;
    ss << ref;
    return ss.str();
});


}// namespace litetvm