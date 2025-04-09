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

/*!
 * \brief Return the value.
 *
 * \param value The returned value.
 * \param span The location of this operation in the source.
 * \return The return expression.
 */
LITETVM_API PrimExpr ret(const PrimExpr& value);

/*!
 * Query the maximum possible value of dtype.
 * \param dtype The data type.
 * \param span The location of this operation in the source.
 * \return the maximum possible value in this format.
 */
LITETVM_API PrimExpr max_value(const DataType& dtype);

/*!
 * Query the minimum possible value of dtype.
 * \param dtype The data type.
 * \param span The location of this operation in the source.
 * \return the minimum possible value in this format.
 */
LITETVM_API PrimExpr min_value(const DataType& dtype);

/*!
 * Get the value of infinity.
 * \param dtype The data type.
 * \param span The location of this operation in the source.
 * \return the infinity value in this format.
 */
LITETVM_API PrimExpr infinity(const DataType& dtype);

/*!
 * \brief cast value to type.
 *
 * \param t the target type.
 * \param value The value
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note This function may return value if the type is the same.
 */
LITETVM_API PrimExpr cast(const DataType& t, PrimExpr value);

/*!
 * \brief perform reinterpret cast value to type.
 *
 * \param t the target type.
 * \param value The value
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note This function may return value if the type is the same.
 */
LITETVM_API PrimExpr reinterpret(const DataType& t, PrimExpr value);

namespace tir {


/*!
 * \brief Make a const value with certain data type.
 * \param t The target type.
 * \param value The input value
 * \return the result expression.
 * \tparam ValueType The constant value type
 * \param span The location of this operation in the source.
 */
template<typename ValueType,
         typename = std::enable_if_t<std::is_pod_v<ValueType>>>
PrimExpr make_const(DataType t, ValueType value);

}// namespace tir

}// namespace litetvm

#endif//LITETVM_TIR_OP_H
