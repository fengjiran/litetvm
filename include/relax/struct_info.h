//
// Created by 赵丹 on 25-4-14.
//

#ifndef LITETVM_RELAX_STRUCT_INFO_H
#define LITETVM_RELAX_STRUCT_INFO_H

#include "ir/env_func.h"
#include "relax/expr.h"
#include "relax/type.h"

namespace litetvm {
namespace relax {

/*!
 * \brief Opaque object.
 */
class ObjectStructInfoNode : public StructInfoNode {
public:
    void VisitAttrs(AttrVisitor* v) {
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const ObjectStructInfoNode* other, SEqualReducer equal) const { return true; }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce(0); }

    static constexpr const char* _type_key = "relax.ObjectStructInfo";
    TVM_DECLARE_FINAL_OBJECT_INFO(ObjectStructInfoNode, StructInfoNode);
};

/*!
 * \brief Managed reference to ObjectStructInfoNode.
 * \sa ObjectStructInfoNode
 */
class ObjectStructInfo : public StructInfo {
public:
    LITETVM_API ObjectStructInfo();

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(ObjectStructInfo, StructInfo, ObjectStructInfoNode);
};

}// namespace relax
}// namespace litetvm

#endif//LITETVM_RELAX_STRUCT_INFO_H
