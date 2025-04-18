//
// Created by 赵丹 on 25-4-18.
//

#include "ir/type_functor.h"


namespace litetvm {

void TypeVisitor::VisitType_(const FuncTypeNode* op) {
    for (auto arg_type: op->arg_types) {
        this->VisitType(arg_type);
    }
    this->VisitType(op->ret_type);
}

void TypeVisitor::VisitType_(const TupleTypeNode* op) {
    for (const Type& t: op->fields) {
        this->VisitType(t);
    }
}

void TypeVisitor::VisitType_(const PrimTypeNode* op) {}

void TypeVisitor::VisitType_(const PointerTypeNode* op) { this->VisitType(op->element_type); }

Type TypeMutator::VisitType(const Type& t) {
    return t.defined() ? TypeFunctor<Type(const Type&)>::VisitType(t) : t;
}

// Type Mutator.
Array<Type> TypeMutator::MutateArray(Array<Type> arr) {
    // The array will do copy on write
    // If no changes are made, the original array will be returned.
    return arr.Map([this](const Type& ty) { return VisitType(ty); });
}

Type TypeMutator::VisitType_(const FuncTypeNode* op) {
    bool changed = false;

    Array<Type> new_args = MutateArray(op->arg_types);
    changed = changed || !new_args.same_as(op->arg_types);

    Type new_ret_type = VisitType(op->ret_type);
    changed = changed || !new_ret_type.same_as(op->ret_type);

    if (!changed) return GetRef<Type>(op);
    return FuncType(new_args, new_ret_type);
}

Type TypeMutator::VisitType_(const TupleTypeNode* op) {
    Array<Type> new_fields = MutateArray(op->fields);
    if (new_fields.same_as(op->fields)) {
        return GetRef<Type>(op);
    }
    return TupleType(new_fields);
}

Type TypeMutator::VisitType_(const PrimTypeNode* op) { return GetRef<Type>(op); }

Type TypeMutator::VisitType_(const PointerTypeNode* op) {
    Type element_type = VisitType(op->element_type);

    if (element_type.same_as(op->element_type)) {
        return GetRef<Type>(op);
    }
    return PointerType(element_type, op->storage_scope);
}

}// namespace litetvm
