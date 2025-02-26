//
// Created by 赵丹 on 25-2-26.
//

#ifndef EXPR_H
#define EXPR_H

#include "runtime/object.h"

namespace litetvm {

using runtime::Object;
using runtime::ObjectRef;
using runtime::String;

/*!
 * \brief Base type of all the expressions.
 * \sa Expr
 */
class BaseExprNode : public Object {
public:
  /*!
   * \brief Span that points to the original source code.
   *        Reserved debug information.
   */
  // mutable Span span;

  static constexpr const char* _type_key = "BaseExpr";
  static constexpr bool _type_has_method_sequal_reduce = true;
  static constexpr bool _type_has_method_shash_reduce = true;
  static constexpr uint32_t _type_child_slots = 62;
  TVM_DECLARE_BASE_OBJECT_INFO(BaseExprNode, Object);
};

/*!
 * \brief Managed reference to BaseExprNode.
 * \sa BaseExprNode
 */
class BaseExpr : public ObjectRef {
public:
  TVM_DEFINE_OBJECT_REF_METHODS(BaseExpr, ObjectRef, BaseExprNode);
};

}

#endif //EXPR_H
