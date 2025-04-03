//
// Created by 赵丹 on 25-3-24.
//

#include "tir/op.h"
#include "runtime/registry.h"
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

    if (auto* access = expr.as<tir::CallNode>()) {
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

    if (auto* address_of = expr.as<tir::CallNode>()) {
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

}// namespace litetvm
