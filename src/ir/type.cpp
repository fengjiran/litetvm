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

GlobalTypeVar::GlobalTypeVar(String name_hint, TypeKind kind) {
    auto n = runtime::make_object<GlobalTypeVarNode>();
    n->name_hint = name_hint;
    n->kind = kind;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(GlobalTypeVarNode);

TVM_REGISTER_GLOBAL("ir.GlobalTypeVar").set_body_typed([](String name, int kind) {
    return GlobalTypeVar(name, static_cast<TypeKind>(kind));
});

TupleType::TupleType(Array<Type> fields) {
    auto n = runtime::make_object<TupleTypeNode>();
    n->fields = std::move(fields);
    data_ = std::move(n);
}

TupleType TupleType::Empty() {
    return TupleType(Array<Type>());
}

TVM_REGISTER_NODE_TYPE(TupleTypeNode);

TVM_REGISTER_GLOBAL("ir.TupleType").set_body_typed([](Array<Type> fields) {
    return TupleType(fields);
});

FuncType::FuncType(Array<Type> arg_types, Type ret_type, Array<TypeVar> type_params, Array<TypeConstraint> type_constraints) {
    auto n = runtime::make_object<FuncTypeNode>();
    n->arg_types = std::move(arg_types);
    n->ret_type = std::move(ret_type);
    n->type_params = std::move(type_params);
    n->type_constraints = std::move(type_constraints);
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(FuncTypeNode);

TVM_REGISTER_GLOBAL("ir.FuncType")
        .set_body_typed([](Array<Type> arg_types, Type ret_type, Array<TypeVar> type_params,
                           Array<TypeConstraint> type_constraints) {
            return FuncType(arg_types, ret_type, type_params, type_constraints);
        });

IncompleteType::IncompleteType(TypeKind kind) {
    auto n = runtime::make_object<IncompleteTypeNode>();
    n->kind = kind;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(IncompleteTypeNode);

TVM_REGISTER_GLOBAL("ir.IncompleteType").set_body_typed([](int kind) {
    return IncompleteType(static_cast<TypeKind>(kind));
});

RelayRefType::RelayRefType(Type value) {
    auto n = runtime::make_object<RelayRefTypeNode>();
    n->value = value;
    data_ = std::move(n);
}

TVM_REGISTER_GLOBAL("ir.RelayRefType").set_body_typed([](Type value) {
    return RelayRefType(value);
});

TVM_REGISTER_NODE_TYPE(RelayRefTypeNode);


}// namespace litetvm
