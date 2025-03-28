//
// Created by 赵丹 on 25-3-24.
//

#include "tir/op.h"
#include "runtime/registry.h"
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

}// namespace litetvm
