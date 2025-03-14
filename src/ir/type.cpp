//
// Created by 赵丹 on 25-3-13.
//

#include "ir/type.h"
#include "runtime/registry.h"

namespace litetvm {

PrimType::PrimType(runtime::DataType dtype) {
    auto n = runtime::make_object<PrimTypeNode>();
    n->dtype = dtype;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(PrimTypeNode);

TVM_REGISTER_GLOBAL("ir.PrimType").set_body_typed([](runtime::DataType dtype) {
    return PrimType(dtype);
});

PointerType::PointerType(Type element_type, String storage_scope) {
    auto n = runtime::make_object<PointerTypeNode>();
    n->element_type = element_type;
    n->storage_scope = storage_scope;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(PointerTypeNode);

TVM_REGISTER_GLOBAL("ir.PointerType")
        .set_body_typed([](Type element_type, String storage_scope = "") {
            return PointerType(element_type, storage_scope);
        });

TypeVar::TypeVar(String name_hint, TypeKind kind) {
    auto n = runtime::make_object<TypeVarNode>();
    n->name_hint = name_hint;
    n->kind = kind;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(TypeVarNode);

TVM_REGISTER_GLOBAL("ir.TypeVar").set_body_typed([](String name, int kind) {
    return TypeVar(name, static_cast<TypeKind>(kind));
});


}// namespace litetvm
