//
// Created by 赵丹 on 25-3-13.
//

#ifndef LITETVM_IR_TYPE_H
#define LITETVM_IR_TYPE_H

#include "runtime/array.h"
#include "runtime/data_type.h"
#include "runtime/object.h"

#include <string>

namespace litetvm {

using runtime::Object;
using runtime::ObjectRef;

/*!
 * \brief Type is the base type of all types.
 *
 * Relay's type system contains following subclasses:
 *
 * - PrimType: type of primitive type values used in the low-level IR.
 * - FuncType: type of function.
 * - TensorType: type of certain Tensor values in the expression.
 *
 * There are also advanced types to support generic(polymorphic types).
 * \sa Type
 */
class TypeNode : public Object {
public:
    /*!
   * \brief Span that points to the original source code.
   *        Reserved debug information.
   */
    // mutable Span span;

    static constexpr const char* _type_key = "Type";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    static constexpr uint32_t _type_child_slots = 14;
    TVM_DECLARE_BASE_OBJECT_INFO(TypeNode, Object);
};

/*!
 * \brief Managed reference to TypeNode.
 * \sa TypeNode
 */
class Type : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(Type, ObjectRef, TypeNode);
};

}// namespace litetvm

#endif//LITETVM_IR_TYPE_H
