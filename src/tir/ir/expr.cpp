//
// Created by 赵丹 on 25-3-13.
//

#include "tir/expr.h"
#include "runtime/registry.h"
#include "support/str_escape.h"
#include "tir/var.h"

#include <optional>
#include <tir/op.h>
#include <utility>

namespace litetvm {
namespace tir {

using runtime::make_object;

/* \brief Convert an object to a PrimExpr
 *
 * All conversions to a PrimExpr are performed as part of the FFI,
 * when calling a function that accepts a PrimExpr as an argument.
 * If a function must normalize to a PrimExpr (e.g. before accessing the
 * `expr.dtype` field), this function allows the FFI conversions to be
 * explicitly invoked.
 */
TVM_REGISTER_GLOBAL("tir.convert").set_body_typed([](Variant<PrimExpr, Array<PrimExpr>> expr) {
    return expr;
});

#define TVM_DEFINE_BINOP_CONSTRUCTOR(Name)                                                       \
    Name::Name(PrimExpr a, PrimExpr b) {                                                         \
        using T = Name::ContainerType;                                                           \
        CHECK(a.defined()) << "ValueError: a is undefined\n";                                    \
        CHECK(b.defined()) << "ValueError: b is undefined\n";                                    \
        CHECK(a.dtype() == b.dtype()) << "TypeError: mismatched types. " << a.dtype() << " vs. " \
                                      << b.dtype() << "\n";                                      \
        ObjectPtr<T> node = make_object<T>();                                                    \
        node->dtype = a.dtype();                                                                 \
        node->a = std::move(a);                                                                  \
        node->b = std::move(b);                                                                  \
        data_ = std::move(node);                                                                 \
    }

#define TVM_DEFINE_CMPOP_CONSTRUCTOR(Name)                                                          \
    Name::Name(PrimExpr a, PrimExpr b) {                                                            \
        using T = Name::ContainerType;                                                              \
        CHECK(a.defined()) << "ValueError: a is undefined\n";                                       \
        CHECK(b.defined()) << "ValueError: b is undefined\n";                                       \
        CHECK(a.dtype() == b.dtype()) << "TypeError: mismatched types. " << a.dtype() << " vs. "    \
                                      << b.dtype() << "\n";                                         \
        ObjectPtr<T> node = make_object<T>();                                                       \
        DataType a_dtype = a.dtype();                                                               \
        node->dtype =                                                                               \
                DataType::Bool(a_dtype.get_lanes_or_vscale_factor(), a_dtype.is_scalable_vector()); \
        node->a = std::move(a);                                                                     \
        node->b = std::move(b);                                                                     \
        data_ = std::move(node);                                                                    \
    }

// Var
Var::Var(String name_hint, DataType dtype) {
    auto n = make_object<VarNode>();
    n->name_hint = std::move(name_hint);
    n->type_annotation = GetTypeFromRuntimeDataType(dtype);
    n->dtype = dtype;
    data_ = std::move(n);
}

Var::Var(String name_hint, Type type_annotation) {
    auto n = make_object<VarNode>();
    n->name_hint = std::move(name_hint);
    n->dtype = GetRuntimeDataType(type_annotation);
    n->type_annotation = std::move(type_annotation);
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(VarNode);

// SizeVar
SizeVar::SizeVar(String name_hint, DataType t) {
    auto n = make_object<SizeVarNode>();
    n->name_hint = std::move(name_hint);
    n->type_annotation = GetTypeFromRuntimeDataType(t);
    n->dtype = t;
    data_ = std::move(n);
}

SizeVar::SizeVar(String name_hint, Type type_annotation) {
    auto n = make_object<SizeVarNode>();
    n->name_hint = std::move(name_hint);
    n->dtype = GetRuntimeDataType(type_annotation);
    n->type_annotation = std::move(type_annotation);
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(SizeVarNode);
TVM_REGISTER_GLOBAL("tir.SizeVar").set_body_typed([](String s, DataType t) {
    return SizeVar(s, t);
});

// Call
Call::Call(DataType dtype, RelaxExpr op, Array<PrimExpr> args) {
    for (size_t i = 0; i < args.size(); ++i) {
        CHECK(args[i].defined()) << "arg " << i << " is not defined()";
    }

    ObjectPtr<CallNode> node = make_object<CallNode>();
    node->dtype = dtype;
    node->op = std::move(op);
    node->args = std::move(args);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_NODE_TYPE(CallNode);


}// namespace tir
}// namespace litetvm