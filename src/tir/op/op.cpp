//
// Created by 赵丹 on 25-3-24.
//

#include "tir/op.h"
#include "runtime/registry.h"
#include "target/registry.h"
#include "tir/builtin.h"
#include "tir/expr.h"

namespace litetvm {

using namespace tir;

// macro to register an unary op
#define TVM_TIR_REGISTER_PURE_UNARY_OP(OpName)                               \
    TVM_TIR_REGISTER_OP(OpName).set_num_inputs(1).set_attr<TCallEffectKind>( \
            "TCallEffectKind", Integer(CallEffectKind::kPure))

// macro to register an binary op
#define TVM_TIR_REGISTER_PURE_BINARY_OP(OpName)                              \
    TVM_TIR_REGISTER_OP(OpName).set_num_inputs(2).set_attr<TCallEffectKind>( \
            "TCallEffectKind", Integer(CallEffectKind::kPure))

runtime::DataType GetRuntimeDataType(const Type& type) {
    if (auto* n = type.as<PrimTypeNode>()) {
        return n->dtype;
    }

    if (type.as<PointerTypeNode>()) {
        return DataType::Handle();
    }

    if (IsVoidType(type)) {
        return DataType::Void();
    }

    LOG(FATAL) << "Type " << type << " does not have a corresponding runtime::DataType";
}

Type GetTypeFromRuntimeDataType(const DataType& dtype) {
    if (dtype.is_void()) {
        return VoidType();
    }
    return PrimType(dtype);
}

Type GetType(const PrimExpr& expr) {
    // TODO(tqchen): add recursive type inference for Call here
    // once we introduced the corresponding fields to the IR.
    if (auto* ptr = expr.as<tir::VarNode>()) {
        // If Var has a more refined type annotation,
        // return the type anotation
        if (ptr->type_annotation.defined()) {
            return ptr->type_annotation;
        }
    }

    if (auto* access = expr.as<CallNode>()) {
        if (access->op.same_as(builtin::tvm_access_ptr())) {
            CHECK(access->args.size()) << "Builtin tvm_access_ptr() may not have empty arguments";
            auto type_annotation = Downcast<Call>(access->args[0]);
            static auto builtin_op = Op::Get("tir.type_annotation");
            CHECK(type_annotation->op.same_as(builtin_op))
                    << "Expected the first argument of builtin tvm_access_ptr() "
                    << "to be a type annotation, but found " << type_annotation->op;
            return PointerType(PrimType(type_annotation->dtype));
        }
    }

    if (auto* address_of = expr.as<CallNode>()) {
        if (address_of->op.same_as(builtin::address_of())) {
            CHECK_EQ(address_of->args.size(), 1)
                    << "Builtin address_of() expects a single argument, but received arguments "
                    << address_of->args;
            auto* address = address_of->args[0].as<BufferLoadNode>();
            CHECK(address)
                    << "Builtin address_of() expects the argument to be a BufferLoad, but received argument "
                    << address_of->args[0];

            return PointerType(PrimType(address->dtype));
        }
    }
    // Default: return the type indicated by the dtype.
    runtime::DataType dtype = expr.dtype();
    return GetTypeFromRuntimeDataType(dtype);
}

PrimExpr ret(const PrimExpr& value) {
    CHECK(value.defined());
    return Call(value.dtype(), builtin::ret(), {value});
}

TVM_REGISTER_GLOBAL("tir.ret").set_body_typed(ret);

// maximum and min limits
PrimExpr max_value(const DataType& dtype) {
    using namespace tir;
    CHECK_EQ(dtype.lanes(), 1);
    if (dtype.is_int()) {
        if (dtype.bits() == 64) {
            return IntImm(dtype, std::numeric_limits<int64_t>::max());
        }
        if (dtype.bits() < 64) {
            int64_t val = 1;
            val = (val << (dtype.bits() - 1)) - 1;
            return IntImm(dtype, val);
        }
    } else if (dtype.is_uint()) {
        if (dtype.bits() == 64) {
            return make_const(dtype, std::numeric_limits<uint64_t>::max());
        }
        if (dtype.bits() < 64) {
            uint64_t val = 1;
            val = (val << static_cast<uint64_t>(dtype.bits())) - 1;
            return IntImm(dtype, static_cast<int64_t>(val));
        }
    } else if (dtype.is_float()) {
        if (dtype.bits() == 64) {
            return FloatImm(dtype, std::numeric_limits<double>::max());
        }
        if (dtype.bits() == 32) {
            return FloatImm(dtype, std::numeric_limits<float>::max());
        }
        if (dtype.bits() == 16) {
            return FloatImm(dtype, 65504.0);
        }
    } else if (dtype.is_bfloat16()) {
        return FloatImm(dtype, std::numeric_limits<float>::max());
    } else if (dtype.is_float8()) {
        // according to https://arxiv.org/pdf/2209.05433.pdf
        if (dtype.code() == static_cast<int>(DataType::TypeCode::kFloat8_e5m2)) {
            return FloatImm(dtype, 57344.0);
        }
        if (dtype.code() == static_cast<int>(DataType::TypeCode::kFloat8_e4m3fn)) {
            return FloatImm(dtype, 448.0);
        }
    } else if (dtype.is_float4()) {
        return FloatImm(dtype, 6.0);
    }
    LOG(FATAL) << "Cannot decide max_value for type" << dtype;
}

PrimExpr min_value(const DataType& dtype) {
    using namespace tir;
    CHECK_EQ(dtype.lanes(), 1);
    if (datatype::Registry::Global()->GetTypeRegistered(dtype.code())) {
        // TODO(tkonolige): need to convert all registered min functions to use the span.
        auto f = datatype::GetMinFunc(dtype.code());
        CHECK(f) << "No minimum function registered for custom dtype " << (unsigned int) dtype.code();
        // TODO(@hypercubestart) Document this change (and others associated with the overflowing
        // floatimm min bug)
        return (*f)(dtype.bits());
    }
    if (dtype.is_int()) {
        if (dtype.bits() == 64) {
            return IntImm(dtype, std::numeric_limits<int64_t>::lowest());
        }
        if (dtype.bits() < 64) {
            int64_t val = 1;
            val = -(val << (dtype.bits() - 1));
            return IntImm(dtype, val);
        }
    } else if (dtype.is_uint()) {
        return IntImm(dtype, 0);
    } else if (dtype.is_float()) {
        if (dtype.bits() == 64) {
            return FloatImm(dtype, std::numeric_limits<double>::lowest());
        }
        if (dtype.bits() == 32) {
            return FloatImm(dtype, std::numeric_limits<float>::lowest());
        }
        if (dtype.bits() == 16) {
            return FloatImm(dtype, -65504.0);
        }
    } else if (dtype.is_bfloat16()) {
        return FloatImm(dtype, std::numeric_limits<float>::lowest());
    } else if (dtype.is_float8()) {
        // according to https://arxiv.org/pdf/2209.05433.pdf
        if (dtype.code() == static_cast<int>(DataType::TypeCode::kFloat8_e5m2)) {
            return FloatImm(dtype, -57344.0);
        }
        if (dtype.code() == static_cast<int>(DataType::TypeCode::kFloat8_e4m3fn)) {
            return FloatImm(dtype, -448.0);
        }
    } else if (dtype.is_float4()) {
        return FloatImm(dtype, -6.0);
    }
    LOG(FATAL) << "Cannot decide min_value for type" << dtype;
}

// infinity
PrimExpr infinity(const DataType& dtype) {
    using namespace tir;
    CHECK_EQ(dtype.lanes(), 1);
    if (dtype.is_float()) {
        if (dtype.bits() == 64) {
            return FloatImm(dtype, std::numeric_limits<double>::infinity());
        }
        if (dtype.bits() == 32 || dtype.bits() == 16) {
            return FloatImm(dtype, std::numeric_limits<float>::infinity());
        }
    }
    LOG(FATAL) << "Cannot decide infinity for type " << dtype;
}

PrimExpr cast(const DataType& t, PrimExpr value) {
    using tir::FloatImmNode;
    if (value.dtype() == t) return value;
    // const fold IntImm as they are used in index computations
    if (t.is_scalar()) {
        if (const IntImmNode* op = value.as<IntImmNode>()) {
            return make_const(t, op->value);
        }
        if (const FloatImmNode* op = value.as<FloatImmNode>()) {
            return make_const(t, op->value);
        }
        CHECK(!value.dtype().is_handle()) << "Can't cast a handle to other types.";
        return Cast(t, value);
    }

    DataType vtype = t.element_of();
    if (!value.dtype().is_scalable_or_fixed_length_vector()) {
        // manually unroll cast
        if (value.dtype() != vtype) {
            if (const IntImmNode* op = value.as<IntImmNode>()) {
                value = make_const(vtype, op->value);
            } else if (const FloatImmNode* op = value.as<FloatImmNode>()) {
                value = make_const(vtype, op->value);
            } else {
                value = Cast(vtype, value);
            }
        }

        if (t.is_scalable_vector()) {
            return Broadcast(value, Mul(t.vscale_factor(), Call(DataType::Int(32), builtin::vscale(), {})));
        }
        return Broadcast(value, t.lanes());
    }

    /* value is a vector */
    CHECK(value.dtype().is_scalable_vector() == t.is_scalable_vector());

    bool lanes_match = false;
    if (value.dtype().is_scalable_vector()) {
        lanes_match = value.dtype().vscale_factor() == t.vscale_factor();
    } else {
        lanes_match = value.dtype().lanes() == t.lanes();
    }
    CHECK(lanes_match);
    if (const auto* broadcast = value.as<BroadcastNode>()) {
        return Broadcast(cast(vtype, broadcast->value), broadcast->lanes);
    }
    if (const auto* ramp = value.as<RampNode>()) {
        if (t.is_int() || t.is_uint()) {
            // only cast to index data type can be folded to ramp
            return Ramp(cast(vtype, ramp->base), cast(vtype, ramp->stride), ramp->lanes);
        }
    }
    return Cast(t, value);
}

// reinterpret
PrimExpr reinterpret(const DataType& t, PrimExpr value) {
    if (value.dtype() == t) return value;
    if (!t.is_scalable_vector() && !value.dtype().is_scalable_vector()) {
        CHECK(value.dtype().bits() * value.dtype().lanes() == t.bits() * t.lanes() ||
              ((value.dtype().is_float4_e2m1fn() || t.is_float4_e2m1fn()) &&
               value.dtype().bytes() * value.dtype().lanes() == t.bytes() * t.lanes()))
                << "Reinterpret requires size match " << t << " vs " << value.dtype();
    }
    return Call(t, builtin::reinterpret(), {value});
}


}// namespace litetvm
