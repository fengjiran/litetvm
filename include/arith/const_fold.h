//
// Created by 赵丹 on 25-4-3.
//

#ifndef LITETVM_ARITH_CONST_FOLD_H
#define LITETVM_ARITH_CONST_FOLD_H

#include "arith/int_operator.h"
#include "runtime/optional.h"
#include "tir/expr.h"
#include "tir/op.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace litetvm {
namespace arith {

/*!
 * \brief Try to run binary compute with constant folding.
 *
 * \param a The left operand.
 * \param b The right operand.
 * \tparam Op The operator type.
 *
 * \note a and b Must already matched data types with each other.
 * \return NullOpt if constant fold fails, otherwise return the folded result.
 */
template<typename Op>
Optional<PrimExpr> TryConstFold(PrimExpr a, PrimExpr b);


/*!
 * \brief Try to run unary compute with constant folding.
 *
 * \param a The left operand.
 * \tparam Op The operator type.
 *
 * \note a and b Must already matched data types with each other.
 * \return NullOpt if constant fold fails, otherwise return the folded result.
 */
template<typename Op>
Optional<PrimExpr> TryConstFold(PrimExpr a);

/*!
 * \brief Check whether type is used to represent index.
 *
 * Index types are frequently used in shape computation
 * and need to be aggressively constant-folded.
 *
 * \param type The type to represent index.
 * \return the checked result.
 */
inline bool IsIndexType(const DataType& type) {
    return type.is_int() && !type.is_scalable_or_fixed_length_vector() &&
           (type.bits() == 32 || type.bits() == 64);
}

/*! \brief Helper to get const folding result repr in int64. */
inline int64_t GetFoldResultInt64Repr(int64_t x, const DataType& dtype) {
    if (dtype.bits() < 64) {
        x &= (1LL << dtype.bits()) - 1;
    }
    if (dtype.is_int()) {
        // get sign extended value of integer with specified bits
        int64_t m = 1LL << (dtype.bits() - 1);
        x = (x ^ m) - m;
    }
    return x;
}

/*! \brief Helper to get fp32 const folding result repr in double. */
inline double GetFoldResultDoubleRepr(float x) {
    double res = x;
    if (std::isinf(res) || std::isnan(res)) {
        return res;
    }
    // certain platform (eg, on gcc7-i386) do the folding arithmetic
    // on float and write back to double is optimized to double
    // precision arithmetic, this is legal and we check the output
    // range thus to ensure consistency when the float result is inf.
    if (res < std::numeric_limits<float>::lowest()) {
        LOG(WARNING) << "underlying float value overflow";
        return -std::numeric_limits<double>::infinity();
    }

    if (res > std::numeric_limits<float>::max()) {
        LOG(WARNING) << "underlying float value overflow";
        return std::numeric_limits<double>::infinity();
    }
    return res;
}

#define TVM_ARITH_CONST_PROPAGATION(BODY)          \
    using tir::FloatImmNode;                       \
    const IntImmNode* pa = a.as<IntImmNode>();     \
    const IntImmNode* pb = b.as<IntImmNode>();     \
    const FloatImmNode* fa = a.as<FloatImmNode>(); \
    const FloatImmNode* fb = b.as<FloatImmNode>(); \
    BODY;

#define TVM_INDEX_CONST_PROPAGATION(BODY)                   \
    const IntImmNode* pa = a.as<IntImmNode>();              \
    const IntImmNode* pb = b.as<IntImmNode>();              \
    const DataType& ta = a.dtype();                         \
    const DataType& tb = b.dtype();                         \
    if (arith::IsIndexType(ta) && arith::IsIndexType(tb)) { \
        BODY;                                               \
    }

// specialization of constant folders.
template<>
inline Optional<PrimExpr> TryConstFold<tir::Add>(PrimExpr a, PrimExpr b) {
    TVM_ARITH_CONST_PROPAGATION({
        const DataType& rtype = a.dtype();
        if (pa && pb) {
            int64_t res = pa->value + pb->value;
            return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
        }

        if (pa && pa->value == 0) return b;
        if (pb && pb->value == 0) return a;
        if (fa && fb) {
            if (rtype.bits() == 32) {
                return FloatImm(rtype, GetFoldResultDoubleRepr(static_cast<float>(fa->value) +
                                                               static_cast<float>(fb->value)));
            }

            if (rtype.bits() == 64) {
                return FloatImm(rtype, fa->value + fb->value);
            }
        }
        if (fa && fa->value == 0) return b;
        if (fb && fb->value == 0) return a;
    });
    return NullOpt;
}

template<>
inline Optional<PrimExpr> TryConstFold<tir::Sub>(PrimExpr a, PrimExpr b) {
    TVM_ARITH_CONST_PROPAGATION({
        CHECK(!((pa && pa->dtype.is_uint() && pa->value == 0U) &&
                (pb && pb->dtype.is_uint() && pb->value > 0U)))
                << "Checked failed. Minuend 's value is 0U and it's dtype is uint "
                << "while Subtrahend's dtype is uint; which will cause a negative uint";
        const DataType& rtype = a.dtype();
        if (pa && pb) {
            int64_t res = pa->value - pb->value;
            return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
        }
        if (pb && pb->value == 0) return a;
        if (fa && fb) {
            if (rtype.bits() == 32) {
                return FloatImm(rtype, GetFoldResultDoubleRepr(static_cast<float>(fa->value) -
                                                               static_cast<float>(fb->value)));
            }

            if (rtype.bits() == 64) {
                return FloatImm(rtype, fa->value - fb->value);
            }
        }
        if (fb && fb->value == 0) return a;
    });
    return NullOpt;
}

}// namespace arith
}// namespace litetvm

#endif//LITETVM_ARITH_CONST_FOLD_H
