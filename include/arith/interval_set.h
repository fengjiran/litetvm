//
// Created by richard on 4/5/25.
//

#ifndef LITETVM_ARITH_INTERVAL_SET_H
#define LITETVM_ARITH_INTERVAL_SET_H

#include "arith/const_fold.h"
#include "tir/op.h"

namespace litetvm {
namespace arith {

// Acknowledgement: IntervalSet design originates from Halide.
/*!
 * \brief Symbolic interval set.
 *
 * \note We intentionally keep the internal of IntSet private,
         as we might change it later.
 */
class IntervalSetNode : public IntSetNode {
public:
    /*! \brief Minimum value in the interval. */
    PrimExpr min_value;
    /*! \brief Maximum value in the interval. */
    PrimExpr max_value;

    // visitor overload.
    void VisitAttrs(AttrVisitor* v) {
        v->Visit("min_value", &min_value);
        v->Visit("max_value", &max_value);
    }

    /*! \return Whether the interval has upper bound. */
    bool HasUpperBound() const { return !is_pos_inf(max_value) && !IsEmpty(); }
    /*! \return Whether the interval has lower bound. */
    bool HasLowerBound() const { return !is_neg_inf(min_value) && !IsEmpty(); }
    /*! \return Whether the interval is a single point. */
    bool IsSinglePoint() const {
        // NOTE: we are only doing cheap check as this is a frequently called routine,
        // do manual prove of min and max for stronger single point check.
        return min_value.same_as(max_value);
    }

    /*! \return whether interval represent nothing */
    bool IsEmpty() const {
        // during computations, either extreme could occur.
        return is_pos_inf(min_value) || is_neg_inf(max_value);
    }
    /*! \return whether interval represent everything */
    bool IsEverything() const {
        return is_neg_inf(min_value) && is_pos_inf(max_value);
    }

    static constexpr const char* _type_key = "arith.IntervalSet";
    TVM_DECLARE_FINAL_OBJECT_INFO(IntervalSetNode, IntSetNode);
};

/*!
 * \brief Interval set used for symbolic integer analysis.
 * \sa IntervalSetNode
 */
class IntervalSet : public IntSet {
public:
    /*!
     * \brief Make a new instance of interval set.
     * \param min_value The minimum value in the interval.
     * \param max_value The maximum value in the interval.
     * \return The created set.
     */
    LITETVM_API IntervalSet(PrimExpr min_value, PrimExpr max_value);

    /*!
     * \brief Create an IntervalSet that represents a single point.
     * \param value The value to be represented.
     * \return The result set.
     */
    static IntervalSet SinglePoint(PrimExpr value) {
        return {value, value};
    }

    /*!
     * \brief Create an IntervalSet that represents everything.
     * \param value The value to be represented.
     * \return The result set.
     */
    static IntervalSet Everything() {
        return {neg_inf(), pos_inf()};
    }

    /*!
     * \brief Create an empty eet.
     * \return The result set.
     */
    static IntervalSet Empty() { return IntervalSet(pos_inf(), neg_inf()); }

    TVM_DEFINE_OBJECT_REF_COW_METHOD(IntervalSetNode);
    TVM_DEFINE_OBJECT_REF_METHODS(IntervalSet, IntSet, IntervalSetNode);
};

}// namespace arith
}// namespace litetvm

#endif//LITETVM_ARITH_INTERVAL_SET_H
