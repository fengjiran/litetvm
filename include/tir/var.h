//
// Created by 赵丹 on 25-3-13.
//

#ifndef LITETVM_TIR_VAR_H
#define LITETVM_TIR_VAR_H

#include "ir/expr.h"
#include "ir/type.h"
#include "runtime/data_type.h"

#include <string>

namespace litetvm {
namespace tir {

/*!
 * \brief A variable node in the IR.
 *
 * A variable is uniquely identified by its address.
 *
 * Each variable is only bound once in the following nodes:
 * - Allocate
 * - For
 * - Let
 * - LetStmt
 */
class VarNode : public PrimExprNode {
public:
    /*!
   * \brief The hint to the variable name.
   * \note Each variable is uniquely identified by its address.
   */
    String name_hint;
    /*!
   * \brief type annotation of the variable.
   *
   * It is an optional field that provides a refined type of the variable than dtype.
   *
   * \sa tvm/ir/type.h for discussion of relations between runtime::DataType and Type.
   */
    Type type_annotation;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("name", &name_hint);
        v->Visit("type_annotation", &type_annotation);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const VarNode* other, SEqualReducer equal) const {
        if (!equal(dtype, other->dtype)) return false;
        if (!equal(type_annotation, other->type_annotation)) return false;
        return equal.FreeVarEqualImpl(this, other);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(type_annotation);
        hash_reduce.FreeVarHashImpl(this);
    }

    static constexpr const char* _type_key = "tir.Var";
    static constexpr uint32_t _type_child_slots = 1;
    TVM_DECLARE_BASE_OBJECT_INFO(VarNode, PrimExprNode);
};



}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_VAR_H
