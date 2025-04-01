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

Var Var::copy_with_name(const String& name) const {
    const VarNode* node = get();
    ObjectPtr<VarNode> new_ptr;
    if (auto* ptr = this->as<SizeVarNode>()) {
        new_ptr = make_object<SizeVarNode>(*ptr);
    } else {
        new_ptr = make_object<VarNode>(*node);
    }
    new_ptr->name_hint = name;
    return Var(new_ptr);
}

Var Var::copy_with_suffix(const String& suffix) const {
    return this->copy_with_name(get()->name_hint + suffix);
}

Var Var::copy_with_dtype(DataType dtype) const {
    const VarNode* node = get();
    ObjectPtr<VarNode> new_ptr;
    if (auto* ptr = this->as<SizeVarNode>()) {
        new_ptr = make_object<SizeVarNode>(*ptr);
    } else {
        new_ptr = make_object<VarNode>(*node);
    }
    new_ptr->type_annotation = GetTypeFromRuntimeDataType(dtype);
    new_ptr->dtype = dtype;
    return Var(new_ptr);
}

TVM_REGISTER_NODE_TYPE(VarNode);
TVM_REGISTER_GLOBAL("tir.Var").set_body_typed([](String name_hint, TVMArgValue type) {
    if (type.IsObjectRef<Type>()) {
        return Var(name_hint, type.operator Type());
    }
    return Var(name_hint, type.operator DataType());
});

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

// IterVar
IterVar::IterVar(Range dom, Var var, IterVarType t, String thread_tag) {
    auto n = make_object<IterVarNode>();
    if (dom.defined() && dom->extent.defined()) {
        CHECK(dom->extent.dtype().is_int())
                << "The dtype of the domain of an IterVar must be an integer type. However, the domain's "
                   "dtype is "
                << dom->extent.dtype();
        CHECK_EQ(dom->extent.dtype(), var.dtype())
                << "The dtype of the extent of an IterVar (" << dom->extent.dtype()
                << ") must match its associated Var's dtype (" << var.dtype() << ")";
    }

    n->dom = dom;
    n->var = var;
    n->iter_type = t;
    n->thread_tag = std::move(thread_tag);
    data_ = std::move(n);
}

TVM_REGISTER_GLOBAL("tir.IterVar")
        .set_body_typed([](Range dom, Var var, int iter_type, String thread_tag) {
            return IterVar(dom, var, static_cast<IterVarType>(iter_type), thread_tag);
        });
TVM_REGISTER_NODE_TYPE(IterVarNode);

// StringImm
StringImm::StringImm(String value) {
    ObjectPtr<StringImmNode> node = make_object<StringImmNode>();
    node->dtype = DataType::Handle();
    node->value = std::move(value);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.StringImm").set_body_typed([](String value) {
    return StringImm(value);
});
TVM_REGISTER_NODE_TYPE(StringImmNode);

// Cast
Cast::Cast(DataType t, PrimExpr value) {
    CHECK(value.defined());
    CHECK_EQ(t.get_lanes_or_vscale_factor(), value.dtype().get_lanes_or_vscale_factor());
    CHECK(t.is_scalable_vector() == value.dtype().is_scalable_vector());
    ObjectPtr<CastNode> node = make_object<CastNode>();
    node->dtype = t;
    node->value = std::move(value);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.Cast").set_body_typed([](DataType dtype, PrimExpr value) {
    return Cast(dtype, value);
});
TVM_REGISTER_NODE_TYPE(CastNode);

// Add
TVM_DEFINE_BINOP_CONSTRUCTOR(Add);
TVM_REGISTER_NODE_TYPE(AddNode);
TVM_REGISTER_GLOBAL("tir.Add").set_body_typed([](PrimExpr a, PrimExpr b) {
    return Add(a, b);
});

// Sub
TVM_DEFINE_BINOP_CONSTRUCTOR(Sub);
TVM_REGISTER_NODE_TYPE(SubNode);
TVM_REGISTER_GLOBAL("tir.Sub").set_body_typed([](PrimExpr a, PrimExpr b) {
    return Sub(a, b);
});

// Mul
TVM_DEFINE_BINOP_CONSTRUCTOR(Mul);
TVM_REGISTER_NODE_TYPE(MulNode);
TVM_REGISTER_GLOBAL("tir.Mul").set_body_typed([](PrimExpr a, PrimExpr b) {
    return Mul(a, b);
});

// Div
TVM_DEFINE_BINOP_CONSTRUCTOR(Div);
TVM_REGISTER_NODE_TYPE(DivNode);
TVM_REGISTER_GLOBAL("tir.Div").set_body_typed([](PrimExpr a, PrimExpr b) {
    return Div(a, b);
});

// Mod
TVM_DEFINE_BINOP_CONSTRUCTOR(Mod);
TVM_REGISTER_NODE_TYPE(ModNode);
TVM_REGISTER_GLOBAL("tir.Mod").set_body_typed([](PrimExpr a, PrimExpr b) {
    return Mod(a, b);
});

// FloorDiv
TVM_DEFINE_BINOP_CONSTRUCTOR(FloorDiv);
TVM_REGISTER_NODE_TYPE(FloorDivNode);
TVM_REGISTER_GLOBAL("tir.FloorDiv").set_body_typed([](PrimExpr a, PrimExpr b) {
    return FloorDiv(a, b);
});

// FloorMod
TVM_DEFINE_BINOP_CONSTRUCTOR(FloorMod);
TVM_REGISTER_NODE_TYPE(FloorModNode);
TVM_REGISTER_GLOBAL("tir.FloorMod").set_body_typed([](PrimExpr a, PrimExpr b) {
    return FloorMod(a, b);
});

// Min
TVM_DEFINE_BINOP_CONSTRUCTOR(Min);
TVM_REGISTER_NODE_TYPE(MinNode);
TVM_REGISTER_GLOBAL("tir.Min").set_body_typed([](PrimExpr a, PrimExpr b) {
    return Min(a, b);
});

// Max
TVM_DEFINE_BINOP_CONSTRUCTOR(Max);
TVM_REGISTER_NODE_TYPE(MaxNode);
TVM_REGISTER_GLOBAL("tir.Max").set_body_typed([](PrimExpr a, PrimExpr b) {
    return Max(a, b);
});

// EQ
TVM_DEFINE_CMPOP_CONSTRUCTOR(EQ);
TVM_REGISTER_NODE_TYPE(EQNode);
TVM_REGISTER_GLOBAL("tir.EQ").set_body_typed([](PrimExpr a, PrimExpr b) {
    return EQ(a, b);
});

// NE
TVM_DEFINE_CMPOP_CONSTRUCTOR(NE);
TVM_REGISTER_NODE_TYPE(NENode);
TVM_REGISTER_GLOBAL("tir.NE").set_body_typed([](PrimExpr a, PrimExpr b) {
    return NE(a, b);
});

// LT
TVM_DEFINE_CMPOP_CONSTRUCTOR(LT);
TVM_REGISTER_NODE_TYPE(LTNode);
TVM_REGISTER_GLOBAL("tir.LT").set_body_typed([](PrimExpr a, PrimExpr b) {
    return LT(a, b);
});

// LE
TVM_DEFINE_CMPOP_CONSTRUCTOR(LE);
TVM_REGISTER_NODE_TYPE(LENode);
TVM_REGISTER_GLOBAL("tir.LE").set_body_typed([](PrimExpr a, PrimExpr b) {
    return LE(a, b);
});

// GT
TVM_DEFINE_CMPOP_CONSTRUCTOR(GT);
TVM_REGISTER_NODE_TYPE(GTNode);
TVM_REGISTER_GLOBAL("tir.GT").set_body_typed([](PrimExpr a, PrimExpr b) {
    return GT(a, b);
});

// GE
TVM_DEFINE_CMPOP_CONSTRUCTOR(GE);
TVM_REGISTER_NODE_TYPE(GENode);
TVM_REGISTER_GLOBAL("tir.GE").set_body_typed([](PrimExpr a, PrimExpr b) {
    return GE(a, b);
});


// And
And::And(PrimExpr a, PrimExpr b) {
    CHECK(a.defined()) << "ValueError: a is undefined";
    CHECK(b.defined()) << "ValueError: b is undefined";
    CHECK(a.dtype().is_bool());
    CHECK(b.dtype().is_bool());
    CHECK(a.dtype() == b.dtype()) << "TypeError: mismatched types";

    ObjectPtr<AndNode> node = make_object<AndNode>();
    node->dtype = DataType::Bool(a.dtype().get_lanes_or_vscale_factor(), a.dtype().is_scalable_vector());
    node->a = std::move(a);
    node->b = std::move(b);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.And").set_body_typed([](PrimExpr a, PrimExpr b) {
    return And(a, b);
});

TVM_REGISTER_NODE_TYPE(AndNode);

// Or
Or::Or(PrimExpr a, PrimExpr b) {
    CHECK(a.defined()) << "ValueError: a is undefined";
    CHECK(b.defined()) << "ValueError: b is undefined";
    CHECK(a.dtype().is_bool());
    CHECK(b.dtype().is_bool());
    CHECK(a.dtype() == b.dtype()) << "TypeError: mismatched types";

    ObjectPtr<OrNode> node = make_object<OrNode>();
    node->dtype =
            DataType::Bool(a.dtype().get_lanes_or_vscale_factor(), a.dtype().is_scalable_vector());
    node->a = std::move(a);
    node->b = std::move(b);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.Or").set_body_typed([](PrimExpr a, PrimExpr b) {
    return Or(a, b);
});

TVM_REGISTER_NODE_TYPE(OrNode);


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