//
// Created by 赵丹 on 25-3-24.
//

#ifndef LITETVM_TIR_OP_H
#define LITETVM_TIR_OP_H

#include "ir/expr.h"
#include "ir/op.h"
#include "ir/type.h"

#include "tir/builtin.h"
#include "tir/expr.h"
#include "tir/stmt.h"

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

/*!
 * \brief add operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator+(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr add(PrimExpr a, PrimExpr b);

/*!
 * \brief subtraction operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator-(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr sub(PrimExpr a, PrimExpr b);

/*!
 * \brief negation.
 *
 * \param a input.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator-(PrimExpr a);
LITETVM_API PrimExpr neg(PrimExpr a);

/*!
 * \brief multiplication operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator*(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr mul(PrimExpr a, PrimExpr b);

/*!
 * \brief compute division in C semantics.
 *
 * a / b as in C/C++.
 *
 * When operands are integers, it directly corresponds to truncdiv.
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator/(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr div(PrimExpr a, PrimExpr b);

/*!
 * \brief greater
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator>(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr greater(PrimExpr a, PrimExpr b);


/*!
 * \brief greater_equal
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator>=(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr greater_equal(PrimExpr a, PrimExpr b);

/*!
 * \brief less
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator<(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr less(PrimExpr a, PrimExpr b);

/*!
 * \brief less_equal
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator<=(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr less_equal(PrimExpr a, PrimExpr b);

/*!
 * \brief equal
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator==(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr equal(PrimExpr a, PrimExpr b);

/*!
 * \brief not_equal
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator!=(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr not_equal(PrimExpr a, PrimExpr b);

/*!
 * \brief left shift operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator<<(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr left_shift(PrimExpr a, PrimExpr b);

/*!
 * \brief right shift operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator>>(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr right_shift(PrimExpr a, PrimExpr b);

/*!
 * \brief and
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note This operator does eager constant folding.
 */
LITETVM_API PrimExpr operator&&(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr logical_and(PrimExpr a, PrimExpr b);

/*!
 * \brief or
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note This operator does eager constant folding.
 */
LITETVM_API PrimExpr operator||(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr logical_or(PrimExpr a, PrimExpr b);

/*!
 * \brief not
 *
 * \param a left operand
 * \return The result expression.
 * \note This operator does eager constant folding.
 */
LITETVM_API PrimExpr operator!(PrimExpr a);
LITETVM_API PrimExpr logical_not(PrimExpr a);

/*!
 * \brief take bitwise and of two values
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator&(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr bitwise_and(PrimExpr a, PrimExpr b);

/*!
 * \brief take bitwise or of two values
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator|(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr bitwise_or(PrimExpr a, PrimExpr b);

/*!
 * \brief take bitwise xor of two values
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator^(PrimExpr a, PrimExpr b);
LITETVM_API PrimExpr bitwise_xor(PrimExpr a, PrimExpr b);

/*!
 * \brief take bitwise negation of two values
 *
 * \param a the input expression.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator~(PrimExpr a);
LITETVM_API PrimExpr bitwise_neg(PrimExpr a);


/*!
 * \brief compute trunc(a / b)
 *
 * This is the default integer division behavior in C.
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr truncdiv(PrimExpr a, PrimExpr b);

/*!
 * \brief compute the remainder of truncdiv
 *
 * This is the default integer division behavior in C.
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr truncmod(PrimExpr a, PrimExpr b);

/*!
 * \brief compute floor(a / b) where a and b are non-negative.
 *
 * Use this function for index split calculation.
 *
 * This function might take advantage of the fact
 * that a and b are non-negative.
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr indexdiv(PrimExpr a, PrimExpr b);

/*!
 * \brief compute ceil(a / b) where a and b are non-negative.
 *
 * Use this function for shape split calculation.
 *
 * This function might take advantage of the fact
 * that a and b are non-negative.
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       shape types(int32, int64) when possible.
 */
LITETVM_API PrimExpr shapediv(PrimExpr a, PrimExpr b);

/*!
 * \brief compute the remainder floor(a / b) where a and b are non-negative.
 *
 * Use this function for index split calculation.
 * This function might take advantage of the fact
 * that a and b are non-negative.
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr indexmod(PrimExpr a, PrimExpr b);

/*!
 * \brief compute floor(a / b)
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr floordiv(PrimExpr a, PrimExpr b);

/*!
 * \brief compute ceil(a / b)
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr ceildiv(PrimExpr a, PrimExpr b);

/*!
 * \brief compute the remainder of floordiv
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr floormod(PrimExpr a, PrimExpr b);

/*!
 * \brief take maximum of two values
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr max(PrimExpr a, PrimExpr b);

/*!
 * \brief take minimum of two values
 *
 * \param a left operand
 * \param b right operand
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr min(PrimExpr a, PrimExpr b);

/*!
 * \brief Conditional expression.
 *
 * \param cond The condition
 * \param true_value The value when results are true.
 * \param false_value The value when results are false.
 * \param span The location of this operation in the source.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr if_then_else(PrimExpr cond, PrimExpr true_value, PrimExpr false_value);

/*!
 * \brief Mark condition as likely.
 * \param cond The condition
 * \param span The location of this operation in the source.
 * \return The marked expression.
 */
LITETVM_API PrimExpr likely(PrimExpr cond);

/*!
 * \brief Calculate power(x, y)
 * \param x The left operand.
 * \param y The right operand.
 * \param span The location of this operation in the source.
 */
LITETVM_API PrimExpr pow(PrimExpr x, PrimExpr y);

/*!
 * \brief Calculate absolute value of x.
 * \param x The input data
 * \param span The location of this operation in the source.
 *
 * \return The absolute value of input data x
 */
LITETVM_API PrimExpr abs(PrimExpr x);


/*!
 * \brief Check if x is NaN.
 * \param x The input data
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
LITETVM_API PrimExpr isnan(PrimExpr x);

/*!
 * \brief Check if x is finite.
 * \param x The input data
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
LITETVM_API PrimExpr isfinite(PrimExpr x);

/*!
 * \brief Check if x is infinite.
 * \param x The input data
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
LITETVM_API PrimExpr isinf(PrimExpr x);

/*!
 * \brief sum of source expression over axis
 * \param source The source expression.
 * \param axis List of iteration variables that will be used for reduction.
 * \param init The value with which to initialize the output.
 * \param span The location of this operation in the source.
 * \return The result.
 */
LITETVM_API PrimExpr sum(PrimExpr source, Array<tir::IterVar> axis, Array<PrimExpr> init = {});

/*!
 * \brief logical And of source expression over axis
 * \param source The source expression.
 * \param axis List of iteration variables that will be used for reduction.
 * \param init The value with which to initialize the output.
 * \param span The location of this operation in the source.
 */
LITETVM_API PrimExpr all(PrimExpr source, Array<tir::IterVar> axis, Array<PrimExpr> init = {});

/*!
 * \brief logical Or of source expression over axis
 * \param source The source expression.
 * \param axis List of iteration variables that will be used for reduction.
 * \param init The value with which to initialize the output.
 * \param span The location of this operation in the source.
 * \return The result.
 */
LITETVM_API PrimExpr any(PrimExpr source, Array<tir::IterVar> axis, Array<PrimExpr> init = {});

/*!
 * \brief max of source expression over axis
 * \param source The source expression.
 * \param axis List of iteration variables that will be used for reduction.
 * \param init The value with which to initialize the output.
 * \param span The location of this operation in the source.
 * \return The result.
 */
LITETVM_API PrimExpr max(PrimExpr source, Array<tir::IterVar> axis, Array<PrimExpr> init = {});

/*!
 * \brief max of source expression over axis
 * \param source The source expression.
 * \param axis List of iteration variables that will be used for reduction.
 * \param init The value with which to initialize the output.
 * \param span The location of this operation in the source.
 * \return The result.
 */
LITETVM_API PrimExpr min(PrimExpr source, Array<tir::IterVar> axis, Array<PrimExpr> init = {});

/*!
 * \brief product of source expression over axis
 * \param source The source expression.
 * \param axis List of iteration variables that will be used for reduction.
 * \param init The value with which to initialize the output.
 * \param span The location of this operation in the source.
 * \return The result.
 */
LITETVM_API PrimExpr prod(PrimExpr source, Array<tir::IterVar> axis, Array<PrimExpr> init = {});

/*!
 * \brief Calculate floor(x)
 * \param x The input expression.
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
LITETVM_API PrimExpr floor(PrimExpr x);

/*!
 * \brief Calculate ceil(x)
 * \param x The input expression.
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
LITETVM_API PrimExpr ceil(PrimExpr x);

/*!
 * \brief Calculate round(x)
 * \param x The input expression.
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
LITETVM_API PrimExpr round(PrimExpr x);

/*!
 * \brief Calculates std::nearbyint(x)
 * \param x The input expression.
 * \param span The location of this operation in the source.
 * \return The result expression.
 * This is a faster alternate to round.
 */
LITETVM_API PrimExpr nearbyint(PrimExpr x);

/*!
 * \brief Calculate trunc(x)
 * \param x The input expression.
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
LITETVM_API PrimExpr trunc(PrimExpr x);

/*!
 * \brief Construct a large uint constant by its low 32 bits and high 32bits.
 * \param dtype The final data type.
 * \param low The lower 32 bits.
 * \param high The higher 32 bits.
 * \param span The location of this operation in the source.
 * \return The constructed expression.
 */
LITETVM_API PrimExpr LargeUIntImm(DataType dtype, int64_t low, int64_t high);

/*!
 * \brief Execute a multiplication between two Q-numbers x and y
 * followed by a right shift s. The mathematical expression is:
 *
 *    out = round(x*y*2^-s)
 *
 * Please note that the two Q-numbers x and y are supposed to have
 * the same number of fractional bits q.
 *
 * More about Q-numbers here: https://en.wikipedia.org/wiki/Q_(number_format)
 *
 * The rounding rule is to the nearest value, rounding half up
 * (i.e., round(x.1) = x and round (x.5) = x+1)
 * \param x first Q-number
 * \param y second Q-number
 * \param q number of fractional bits in x and y. Needs to be > 0
 * \param s integer right shift
 * \param span The location of this operation in the source.
 * \return The constructed expression.
 */
LITETVM_API PrimExpr q_multiply_shift(PrimExpr x, PrimExpr y, PrimExpr q, PrimExpr s);

/*!
 * \brief Fast_erf_float expression from Eigen
 *
 * \param arg The input expression.
 * \param bits The number of bits in the type.
 * \return The constructed expression.
 */
LITETVM_API PrimExpr fast_erf_float_expr(PrimExpr arg, int bits);

// Intrinsic operators
#define TVM_DECLARE_INTRIN_UNARY(OpName)                                                 \
    inline PrimExpr OpName(PrimExpr x) {                                                 \
        static const Op& op = Op::Get("tir." #OpName);                                   \
        if (x.dtype().is_bfloat16()) {                                                   \
            DataType bf16_dtype = x.dtype();                                             \
            DataType fp32_dtype((int) DLDataTypeCode::kDLFloat, 32, bf16_dtype.lanes()); \
            PrimExpr x_fp32 = tir::Cast(fp32_dtype, {x});                                \
            PrimExpr result_fp32 = tir::Call(fp32_dtype, op, {x_fp32});                  \
            return tir::Cast(bf16_dtype, {result_fp32});                                 \
        } else {                                                                         \
            return tir::Call(x.dtype(), op, {x});                                        \
        }                                                                                \
    }

TVM_DECLARE_INTRIN_UNARY(exp);
TVM_DECLARE_INTRIN_UNARY(exp2);
TVM_DECLARE_INTRIN_UNARY(exp10);
TVM_DECLARE_INTRIN_UNARY(erf);
TVM_DECLARE_INTRIN_UNARY(tanh);
TVM_DECLARE_INTRIN_UNARY(sigmoid);
TVM_DECLARE_INTRIN_UNARY(sqrt);
TVM_DECLARE_INTRIN_UNARY(rsqrt);
TVM_DECLARE_INTRIN_UNARY(log);
TVM_DECLARE_INTRIN_UNARY(log2);
TVM_DECLARE_INTRIN_UNARY(log10);
TVM_DECLARE_INTRIN_UNARY(log1p);
TVM_DECLARE_INTRIN_UNARY(popcount);
TVM_DECLARE_INTRIN_UNARY(tan);
TVM_DECLARE_INTRIN_UNARY(cos);
TVM_DECLARE_INTRIN_UNARY(cosh);
TVM_DECLARE_INTRIN_UNARY(sin);
TVM_DECLARE_INTRIN_UNARY(sinh);
TVM_DECLARE_INTRIN_UNARY(asin);
TVM_DECLARE_INTRIN_UNARY(acos);
TVM_DECLARE_INTRIN_UNARY(atan);
TVM_DECLARE_INTRIN_UNARY(acosh);
TVM_DECLARE_INTRIN_UNARY(asinh);
TVM_DECLARE_INTRIN_UNARY(atanh);
TVM_DECLARE_INTRIN_UNARY(clz);

#define TVM_DECLARE_INTRIN_BINARY(OpName)              \
    inline PrimExpr OpName(PrimExpr x, PrimExpr y) {   \
        static const Op& op = Op::Get("tir." #OpName); \
        return tir::Call(x.dtype(), op, {x, y});       \
    }

TVM_DECLARE_INTRIN_BINARY(atan2);
TVM_DECLARE_INTRIN_BINARY(nextafter);
TVM_DECLARE_INTRIN_BINARY(copysign);
TVM_DECLARE_INTRIN_BINARY(hypot);
TVM_DECLARE_INTRIN_BINARY(ldexp);


namespace tir {

/*!
 * \brief Check if type is a pointer to a runtime element type.
 * \param type The type to be checked.
 * \param element_type The corresponding element type.
 * \return The check results
 */
inline bool IsPointerType(const Type& type, const DataType& element_type) {
    if (!type.defined()) return false;
    if (const auto* ptr_type = type.as<PointerTypeNode>()) {
        if (const auto* prim_type = ptr_type->element_type.as<PrimTypeNode>()) {
            return prim_type->dtype == element_type;
        }
    }
    return false;
}

/*!
 * \brief Make a const value with certain data type.
 * \param t The target type.
 * \param value The input value
 * \return the result expression.
 * \tparam ValueType The constant value type
 * \param span The location of this operation in the source.
 */
template<typename ValueType,
         typename = std::enable_if_t<std::is_trivial_v<ValueType> && std::is_standard_layout_v<ValueType>>>
PrimExpr make_const(DataType t, ValueType value);

/*!
 * \brief Make a const zero expr.
 * \param t The target type.
 * \param span The location of this operation in the source.
 * \return the result expression.
 */
inline PrimExpr make_zero(DataType t);

/*!
 * \brief Make a constant true expression.
 * \param lanes The number of lanes in the bool
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
inline PrimExpr const_true(int lanes = 1) {
    return make_const(DataType::UInt(1, lanes), 1);
}

/*!
 * \brief Make a constant false expression.
 * \param lanes The number of lanes in the bool
 * \param span The location of this operation in the source.
 * \return The result expression.
 */
inline PrimExpr const_false(int lanes = 1) {
    return make_const(DataType::UInt(1, lanes), 0);
}

/*!
 * \brief Get x as constant int expression.
 * \param x The expression
 * \return the address to the int expression,
 *         return nullptr, if x is not IntImm.
 */
inline const int64_t* as_const_int(const PrimExpr& x) {
    if (!x.defined()) return nullptr;
    if (const IntImmNode* op = x.as<IntImmNode>()) {
        return &(op->value);
    }

    return nullptr;
}

/*!
 * \brief Check whether x is a constant integer expression.
 * \param x The input argument
 * \param value the value to be compared against.
 * \return whether x is constant expression.
 */
inline bool is_const_int(const PrimExpr& x, int64_t value);

/*!
 * \brief Check whether stmt is nop.
 * \param stmt The input statement
 * \return whether stmt is nop
 */
inline bool is_no_op(const Stmt& stmt);

/*!
 * \brief Check whether x is a constant integer 1
 * \param x The input argument.
 * \note This only return true for integer types.
 * \return whether x is constant 1
 */
inline bool is_one(const PrimExpr& x) { return is_const_int(x, 1); }

/*!
 * \brief Check whether x is a constant integer 0
 * \param x The input argument
 * \return whether x is constant 0
 * \note This only return true for integer types.
 */
inline bool is_zero(const PrimExpr& x) { return is_const_int(x, 0); }

/*!
 * \brief Check whether x is an integer constant.
 * \note This only return true for integer types.
 * \return whether x is constant
 */
inline bool is_const_int(const PrimExpr& x);

/*!
 * \brief Check whether x is an integer/float constant.
 * \note This only return true for integer types.
 * \return whether x is constant
 */
inline bool is_const_number(const PrimExpr& x);

/*!
 * \brief Left fold.
 * \param freduce The reduction function.
 * \param init_value The initial value.
 * \param values The values to be folded.
 * \param span The location of the fold in the source.
 * \return The result.
 * \tparam FReduce The type of the reduction.
 */
template<typename FReduce>
PrimExpr foldl(FReduce freduce, PrimExpr init_value, const Array<PrimExpr>& values) {
    for (PrimExpr val: values) {
        init_value = freduce(init_value, val);
    }
    return init_value;
}

/*!
 * \brief Check whether x is a constant power of two
 * If x is power of two, write the power to the shift.
 *
 * \param x The input expression.
 * \param shift The output shift if x is power of two.
 * \return whether x is constant power of two
 */
LITETVM_API bool is_const_power_of_two_integer(const PrimExpr& x, int* shift);

// Implementation details after this
inline bool is_const_int(const PrimExpr& x) { return as_const_int(x); }

inline bool is_const_number(const PrimExpr& x) {
    if (x.as<tir::IntImmNode>()) {
        return true;
    } else if (x.as<tir::FloatImmNode>()) {
        return true;
    } else if (const auto* op = x.as<tir::BroadcastNode>()) {
        return (op->value->IsInstance<tir::IntImmNode>() || op->value->IsInstance<tir::FloatImmNode>());
    }
    return false;
}

inline bool is_positive_const(const PrimExpr& a) {
    const int64_t* as_int = as_const_int(a);
    return as_int && (*as_int > 0);
}

inline bool is_negative_const(const PrimExpr& a) {
    const int64_t* as_int = as_const_int(a);
    return as_int && (*as_int < 0);
}

inline bool is_const_int(const PrimExpr& x, int64_t value) {
    const int64_t* as_int = as_const_int(x);
    return as_int && (*as_int == value);
}

inline bool is_no_op(const Stmt& stmt) {
    if (!stmt.defined()) return true;
    if (const auto* op = stmt.as<EvaluateNode>()) {
        return is_const_int(op->value);
    }
    if (const auto* op = stmt.as<SeqStmtNode>()) {
        return op->seq.size() == 0;
    }
    return false;
}

template<typename ValueType>
PrimExpr MakeConstScalar(DataType t, ValueType value) {
    if (t.is_int()) return IntImm(t, static_cast<int64_t>(value));
    if (t.is_uint()) {
        // Use IntImm if it is a small integer
        uint64_t uval = static_cast<uint64_t>(value);
        if (value < static_cast<ValueType>(0)) {
            LOG(FATAL) << "cannot make uint from negative value " << value;
        } else if (uval <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
            return IntImm(t, static_cast<int64_t>(value));
        } else {
            uint64_t mask = (static_cast<uint64_t>(1) << 32U) - 1U;
            uint64_t low = uval & mask;
            uint64_t high = uval >> 32U;
            return LargeUIntImm(t, static_cast<int64_t>(low), static_cast<int64_t>(high));
        }
    }
    if (t.is_float() || t.is_bfloat16() || t.is_float8() || t.is_float4())
        return FloatImm(t, static_cast<double>(value));
    // For now, we store const scalar values of custom datatypes within doubles; later, during the
    // datatypes lowering pass, we will lower the value to its true representation in the format
    // specified by the datatype.
    // TODO(gus) when do we need to start worrying about doubles not being precise enough?
    if (static_cast<uint8_t>(t.code()) >= static_cast<uint8_t>(DataType::TypeCode::kCustomBegin)) {
        return FloatImm(t, static_cast<double>(value));
    }
    LOG(FATAL) << "cannot make const for type " << t;
    throw;
}

template<>
inline PrimExpr MakeConstScalar(DataType t, bool value) {
    return MakeConstScalar(t, static_cast<int>(value));
}

template<typename ValueType, typename>
PrimExpr make_const(DataType t, ValueType value) {
    if (t.is_scalar()) {
        return MakeConstScalar(t, value);
    }

    if (t.is_fixed_length_vector()) {
        return Broadcast(MakeConstScalar(t.element_of(), value), t.lanes());
    }

    PrimExpr lanes = Mul(Call(DataType::Int(32), tir::builtin::vscale(), {}), t.vscale_factor());
    return Broadcast(MakeConstScalar(t.element_of(), value), lanes);
}

inline PrimExpr make_zero(DataType t) {
    if (t.is_handle()) {
        return reinterpret(t, make_const(DataType::UInt(64), 0));
    }
    return make_const(t, 0);
}

}// namespace tir

// additional const expression overloading
#define TVM_DEFINE_ASSIGN_OP_OVERLOAD(Name, OpFunc) \
    inline PrimExpr Name(PrimExpr& a, PrimExpr b) { \
        a = OpFunc(a, b);                           \
        return a;                                   \
    }

#define TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD(Name)                                     \
    inline PrimExpr Name(const PrimExpr& a, float b) { return Name(a, PrimExpr(b)); } \
    inline PrimExpr Name(float a, const PrimExpr& b) { return Name(PrimExpr(a), b); } \
    inline PrimExpr Name(int a, const PrimExpr& b) {                                  \
        return Name(tir::make_const(b.dtype(), a), b);                                \
    }                                                                                 \
    inline PrimExpr Name(const PrimExpr& a, int b) {                                  \
        return Name(a, tir::make_const(a.dtype(), b));                                \
    }                                                                                 \
    inline PrimExpr Name(const PrimExpr& a, double b) {                               \
        return Name(a, tir::make_const(DataType::Float(64), b));                      \
    }

#define TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(Name)        \
    inline PrimExpr Name(const PrimExpr& a, float b) {           \
        return Name(a, PrimExpr(b));                             \
    }                                                            \
    inline PrimExpr Name(float a, const PrimExpr& b) {           \
        return Name(PrimExpr(a), b);                             \
    }                                                            \
    inline PrimExpr Name(int a, const PrimExpr& b) {             \
        return Name(tir::make_const(b.dtype(), a), b);           \
    }                                                            \
    inline PrimExpr Name(const PrimExpr& a, int b) {             \
        return Name(a, tir::make_const(a.dtype(), b));           \
    }                                                            \
    inline PrimExpr Name(const PrimExpr& a, double b) {          \
        return Name(a, tir::make_const(DataType::Float(64), b)); \
    }

#define TVM_DEFINE_LOGICAL_OP_CONST_VAL_OVERLOAD(Name)                               \
    inline PrimExpr Name(const PrimExpr& a, bool b) { return Name(a, PrimExpr(b)); } \
    inline PrimExpr Name(bool a, const PrimExpr& b) { return Name(PrimExpr(a), b); }

#define TVM_DEFINE_LOGICAL_OP_CONST_VAL_OVERLOAD_SPANNED(Name) \
    inline PrimExpr Name(const PrimExpr& a, bool b) {          \
        return Name(a, PrimExpr(b));                           \
    }                                                          \
    inline PrimExpr Name(bool a, const PrimExpr& b) {          \
        return Name(PrimExpr(a), b);                           \
    }

#define TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD(Name)     \
    inline PrimExpr Name(const PrimExpr& a, int b) {   \
        return Name(a, tir::make_const(a.dtype(), b)); \
    }                                                  \
    inline PrimExpr Name(int a, const PrimExpr& b) { return Name(tir::make_const(b.dtype(), a), b); }

#define TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(Name) \
    inline PrimExpr Name(const PrimExpr& a, int b) {       \
        return Name(a, tir::make_const(a.dtype(), b));     \
    }                                                      \
    inline PrimExpr Name(int a, const PrimExpr& b) {       \
        return Name(tir::make_const(b.dtype(), a), b);     \
    }

TVM_DEFINE_ASSIGN_OP_OVERLOAD(operator+=, operator+);
TVM_DEFINE_ASSIGN_OP_OVERLOAD(operator-=, operator-);
TVM_DEFINE_ASSIGN_OP_OVERLOAD(operator*=, operator*);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD(operator+);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD(operator-);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD(operator*);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD(operator>);// NOLINT(*)
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD(operator>=);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD(operator<);// NOLINT(*)
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD(operator<=);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(max);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(min);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(div);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(add);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(sub);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(mul);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(greater);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(greater_equal);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(less);
TVM_DEFINE_BINOP_CONST_VAL_OVERLOAD_SPANNED(less_equal);
// integer related ops
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(indexdiv);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(indexmod);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(truncdiv);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(truncmod);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(floordiv);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(floormod);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(right_shift);// NOLINT(*)
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(left_shift); // NOLINT(*)
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(bitwise_and);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(bitwise_or);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD_SPANNED(bitwise_xor);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD(operator>>);// NOLINT(*)
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD(operator<<);// NOLINT(*)
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD(operator&);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD(operator|);
TVM_DEFINE_INT_OP_CONST_VAL_OVERLOAD(operator^);
// logical ops
TVM_DEFINE_LOGICAL_OP_CONST_VAL_OVERLOAD(operator&&);
TVM_DEFINE_LOGICAL_OP_CONST_VAL_OVERLOAD(operator||);
TVM_DEFINE_LOGICAL_OP_CONST_VAL_OVERLOAD_SPANNED(logical_and);
TVM_DEFINE_LOGICAL_OP_CONST_VAL_OVERLOAD_SPANNED(logical_or);

/*!
 * \brief Helper function to raise a compiler error about division ambiguity.
 * \note The call to this function will always results in a compiler error.
 * \tparam TA Any class type.
 */
template<typename TA>
void DivAmbiguityError(const TA& a) {
    constexpr bool div_ambiguity = !std::is_class<TA>::value;
    static_assert(div_ambiguity,
                  "TVM supports multiple types of integer divisions, "
                  "please call div, indexdiv/indexmod, "
                  "floordiv/floormod or truncdiv/truncmod directly "
                  "to avoid ambiguity in the code. "
                  "Checkout these functions in tir/op.h.");
}

// The following code are not intended to be used in the codebase.
// Instead, they generate clear compiler errors that ask developers
// to use the specific division function.
// The second template argument is necessary to make sure the
// code compiles lazily by the compiler during invocation.
template<typename TB>
PrimExpr operator/(const PrimExpr& a, const TB& b) {
    DivAmbiguityError(a);
    return a;
}

template<typename TB>
PrimExpr operator/=(const PrimExpr& a, const TB& b) {
    DivAmbiguityError(a);
    return a;
}

template<typename TB>
PrimExpr operator%(const PrimExpr& a, const TB& b) {
    DivAmbiguityError(a);
    return a;
}

}// namespace litetvm

#endif//LITETVM_TIR_OP_H
