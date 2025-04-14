//
// Created by 赵丹 on 25-4-14.
//

#ifndef LITETVM_RELAX_EXPR_H
#define LITETVM_RELAX_EXPR_H

#include "ir/expr.h"
#include "ir/function.h"
#include "node/reflection.h"
#include "node/repr_printer.h"
#include "node/structural_equal.h"
#include "node/structural_hash.h"
#include "relax/type.h"
#include "runtime/array.h"
#include "runtime/object.h"
#include "tir/expr.h"
#include "tir/op.h"

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace litetvm {
namespace relax {

using Expr = RelaxExpr;
using ExprNode = RelaxExprNode;

/*!
 * \brief The unique identifier of variables.
 *
 * Id is like name to the variables,
 * except that id is unique for each Var.
 *
 * \note Do not create Id directly, they are created in Var.
 */
class IdNode : public Object {
public:
    /*!
   * \brief The name of the variable,
   *  this only acts as a hint to the user,
   *  and is not used for equality.
   */
    String name_hint;

    void VisitAttrs(AttrVisitor* v) { v->Visit("name_hint", &name_hint); }

    bool SEqualReduce(const IdNode* other, SEqualReducer equal) const {
        return equal.FreeVarEqualImpl(this, other);
    }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce.FreeVarHashImpl(this); }

    static constexpr const char* _type_key = "relax.Id";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    TVM_DECLARE_FINAL_OBJECT_INFO(IdNode, Object);
};

class Id : public ObjectRef {
public:
    /*!
   * \brief The constructor
   * \param name_hint The name of the variable.
   */
    LITETVM_API explicit Id(String name_hint);

    TVM_DEFINE_OBJECT_REF_METHODS(Id, ObjectRef, IdNode);
};

/*!
 * \brief Base type of all structure information.
 *
 * StructInfo stores possible structure information
 * deduced during compile-time. It encapsulates
 * both static type and runtime information such
 * as shape.
 *
 * StructInfo of each non-primitive Expr can be
 * deduced during compilation in a "best-effort" manner.
 *
 * When struct_info appears in function parameter and return
 * signatures. They will imply a runtime check that matches
 * the structure information with the value.
 *
 * When it appears in Expr, they follow "assume-semantics",
 * which means the compiler will take the deduced information as it is
 * and only do best effort prove and checks.
 *
 * Each struct info can be uniquely erased to a static-type.
 * The compiler will still compile the code(with less information)
 * when we erase to the static type.
 *
 * If an StructInfo contains an Expr field, then that field
 * must be normalized already through NormalizeArg.
 * This invariant will be checked in constructors
 * and help us to simplify our assumption
 * during struct info deduction.
 */
class StructInfoNode : public Object {
public:
    /*!
   * \brief Span that points to the original source code.
   *        Reserved debug information.
   */
    // mutable Span span;

    static constexpr const char* _type_key = "StructInfo";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    static constexpr uint32_t _type_child_slots = 7;
    TVM_DECLARE_BASE_OBJECT_INFO(StructInfoNode, Object);
};

/*!
 * \brief Managed reference to StructInfoNode.
 * \sa StructInfoNode
 */
class StructInfo : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(StructInfo, ObjectRef, StructInfoNode);
};

/*!
 * \brief Call corresponds to callable invocation.
 *  Corresponds to operation in computational graph terminology.
 */
class CallNode : public ExprNode {
public:
    /*!
   * \brief The operator(function) being invoked
   *
   *  - It can be tvm::Op which corresponds to the primitive operators.
   *  - It can also be user defined functions (Function, GlobalVar, Var).
   */
    Expr op;

    /*! \brief The arguments(inputs) of the call */
    Array<Expr> args;

    /*! \brief The additional attributes */
    Attrs attrs;

    /*!
   * \brief The structure info arguments of a CallNode.
   * sinfo_args is designed to be non-empty only for intrinsic op (e.g.,
   * call_tir, call_builtin_with_ctx, etc.) and calls to ExternFuncs, with the main
   * usage of structure info inference.
   */
    Array<StructInfo> sinfo_args;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("op", &op);
        v->Visit("args", &args);
        v->Visit("attrs", &attrs);
        v->Visit("sinfo_args", &sinfo_args);
        v->Visit("struct_info_", &struct_info_);
        v->Visit("_checked_type_", &checked_type_);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const CallNode* other, SEqualReducer equal) const {
        // skip sinfo_args check for primitive ops.
        return equal(op, other->op) && equal(args, other->args) && equal(attrs, other->attrs) &&
               equal(sinfo_args, other->sinfo_args) && equal(struct_info_, other->struct_info_);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(op);
        hash_reduce(args);
        hash_reduce(attrs);
        hash_reduce(sinfo_args);
        hash_reduce(struct_info_);
    }

    static constexpr const char* _type_key = "relax.expr.Call";
    TVM_DECLARE_FINAL_OBJECT_INFO(CallNode, ExprNode);
};

class Call : public Expr {
public:
    /*!
   * \brief The constructor
   * \param op The operator to be invoked.
   * \param args The arguments of the call.
   * \param attrs The attributes of the call node.
   * \param sinfo_args The structure info arguments passed to a function.
   * \param span The source span of the expression.
   */
    LITETVM_API Call(Expr op, Array<Expr> args, Attrs attrs = Attrs(),
                     Array<StructInfo> sinfo_args = Array<StructInfo>());

    TVM_DEFINE_OBJECT_REF_METHODS(Call, Expr, CallNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(CallNode);
};

}// namespace relax
}// namespace litetvm

#endif//LITETVM_RELAX_EXPR_H
