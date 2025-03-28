//
// Created by 赵丹 on 25-3-13.
//

#ifndef LITETVM_TIR_EXPR_H
#define LITETVM_TIR_EXPR_H

#include "tir/var.h"

namespace litetvm {
namespace tir {

/*! \brief String constants, only used in asserts. */
class StringImmNode : public PrimExprNode {
public:
    /*! \brief The constant value content. */
    String value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("value", &value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const StringImmNode* other, SEqualReducer equal) const {
        return equal(value, other->value);
    }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce(value); }

    static constexpr const char* _type_key = "tir.StringImm";
    TVM_DECLARE_FINAL_OBJECT_INFO(StringImmNode, PrimExprNode);
};

/*!
 * \brief Managed reference to StringImmNode.
 * \sa StringImmNode
 */
class StringImm : public PrimExpr {
public:
    // TVM_DLL StringImm(String value, Span span = Span());
    LITETVM_API StringImm(String value);
    TVM_DEFINE_OBJECT_REF_METHODS(StringImm, PrimExpr, StringImmNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(StringImmNode);
};

/*!
 * \brief Cast value from one data type to another.
 * \note The lanes of value should keep fixed.
 */
class CastNode : public PrimExprNode {
public:
    /*! \brief Original data type. */
    PrimExpr value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("value", &value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const CastNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(value, other->value);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(value);
    }

    static constexpr const char* _type_key = "tir.Cast";
    TVM_DECLARE_FINAL_OBJECT_INFO(CastNode, PrimExprNode);
};

/*!
 * \brief Managed reference to CastNode
 * \sa CastNode
 */
class Cast : public PrimExpr {
public:
    // TVM_DLL Cast(DataType dtype, PrimExpr value, Span span = Span());
    LITETVM_API Cast(DataType dtype, PrimExpr value);
    TVM_DEFINE_OBJECT_REF_METHODS(Cast, PrimExpr, CastNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(CastNode);
};


/*!
 * \brief Call node.
 */
class CallNode : public PrimExprNode {
public:
    /*!
   * \brief The operator(function) being invoked
   *
   *  - It can be tvm::Op which corresponds to the primitive operators(intrinsics).
   *  - It can also be another function in the IRModule (GlobalVar).
   */
    RelaxExpr op;

    /*! \brief The arguments. */
    Array<PrimExpr> args;
    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("op", &op);
        v->Visit("args", &args);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const CallNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(op, other->op) && equal(args, other->args);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(op);
        hash_reduce(args);
    }

    static constexpr const char* _type_key = "tir.Call";
    TVM_DECLARE_FINAL_OBJECT_INFO(CallNode, PrimExprNode);
};

/*!
 * \brief Managed reference to CallNode
 * \sa CallNode
 */
class Call : public PrimExpr {
public:
    // TVM_DLL Call(DataType dtype, RelaxExpr op, Array<PrimExpr> args, Span span = Span());
    LITETVM_API Call(DataType dtype, RelaxExpr op, Array<PrimExpr> args);
    TVM_DEFINE_OBJECT_REF_METHODS(Call, PrimExpr, CallNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(CallNode);
};

}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_EXPR_H
