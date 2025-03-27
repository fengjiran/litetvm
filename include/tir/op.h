//
// Created by 赵丹 on 25-3-24.
//

#ifndef LITETVM_TIR_OP_H
#define LITETVM_TIR_OP_H

#include "ir/expr.h"
#include "ir/op.h"
#include "ir/type.h"

#include "tir/expr.h"

#include <limits>
#include <type_traits>

namespace litetvm {

#define TVM_TIR_REGISTER_OP(OpName) \
    TVM_REGISTER_OP("tir." OpName).set_attr<TScriptPrinterName>("TScriptPrinterName", OpName)

// Most common operators can be overloaded by argument type(PrimExpr).
// So we put them under the root namespace.
//
// We put more developer oriented APIs -- make_const and is_const under tir
// as they are more specific to the tir namespace.

/*!
 * \brief Get the type of the expression under the unified type system.
 *
 * This function could return a more refined type than
 * the runtime type provided by expr->dtype
 *
 * \param expr The input parameter.
 * \return The result type.
 *
 * \sa tvm/ir/type.h for discussion about the relation between Type and runtime::DataType.
 */
LITETVM_API Type GetType(const PrimExpr& expr);


/*!
 * \brief Get the type corresponding to DataType
 * \param dtype The data type
 * \return The result type
 *
 * \sa tvm/ir/type.h for discussion about the relation between Type and runtime::DataType.
 */
LITETVM_API Type GetTypeFromRuntimeDataType(const DataType& dtype);

/*!
 * \brief Get the implied DataType for storing values with type during runtime.
 *
 * \param type The input type.
 * \return The result runtime::DataType.
 *
 * \sa tvm/ir/type.h for discussion about the relation between Type and runtime::DataType.
 */
LITETVM_API runtime::DataType GetRuntimeDataType(const Type& type);

}// namespace litetvm

#endif//LITETVM_TIR_OP_H
