//
// Created by 赵丹 on 25-3-14.
//

#ifndef LITETVM_TARGET_INFO_H
#define LITETVM_TARGET_INFO_H

#include "ir/expr.h"

#include <string>

namespace litetvm {

/*!
 * \brief Memory information of special memory region.
 *  Use MemoryInfo as its container type
 */
class MemoryInfoNode : public Object {
public:
    /*! \brief The addressable unit */
    int64_t unit_bits;
    /*! \brief Maximum number of bits supported in the memory */
    int64_t max_num_bits;
    /*! \brief maximum number of bits to be used in simd op */
    int64_t max_simd_bits;
    /*!
   * \brief head address of the buffer, if visible to CPU
   *  This address can be None.
   */
    PrimExpr head_address;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("unit_bits", &unit_bits);
        v->Visit("max_num_bits", &max_num_bits);
        v->Visit("max_simd_bits", &max_simd_bits);
        v->Visit("head_address", &head_address);
    }

    static constexpr const char* _type_key = "MemoryInfo";
    TVM_DECLARE_FINAL_OBJECT_INFO(MemoryInfoNode, Object);
};

/*! \brief Defines memory info */
class MemoryInfo : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(MemoryInfo, ObjectRef, MemoryInfoNode);
};

/*!
 * \brief get memory info given scope
 * \param scope The scope name.
 * \return info The memory info.
 */
LITETVM_API MemoryInfo GetMemoryInfo(const std::string& scope);

}// namespace litetvm

#endif//LITETVM_TARGET_INFO_H
