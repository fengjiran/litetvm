//
// Created by 赵丹 on 25-3-13.
//

#ifndef LITETVM_TIR_EXPR_H
#define LITETVM_TIR_EXPR_H

#include "tir/var.h"

namespace litetvm {
namespace tir {

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
