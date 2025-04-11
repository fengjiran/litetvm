//
// Created by richard on 4/5/25.
//

#ifndef LITETVM_ARITH_ANALYZER_H
#define LITETVM_ARITH_ANALYZER_H

#include "arith/int_set.h"
#include "ir/expr.h"
#include "support/with.h"

#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace litetvm {
namespace arith {

//-------------------------------------------------------
// Base integer analysis API.
//
// We have multiple type of analyzers to do relaxed
// integer set analysis(bound analysis, modulo) and
// equivalence checking and simplification.
//
// Importantly, each analyzer may need result from
// another analyzer.
//-------------------------------------------------------

// Forward declare Analyzer
class Analyzer;

using tir::Var;

enum class DivMode {
    /*! \brief Truncated division. */
    kTruncDiv,
    /*! \brief Floor division. */
    kFloorDiv
};

/*!
 * \brief The strength used in top-level condition proves
 * \note The higher, the more time consuming it can be.
 *
 * Do not use level beyond kDefault in internal recursive rewriting in arith
 * analysis and only use it at top-level simplification to avoid speed issues.
 */
enum class ProofStrength : int {
    /*! \brief default strength, can be used in. */
    kDefault = 0,
    /*!
     * \brief Prove using symbolic bound analysis
     */
    kSymbolicBound = 1,
};

/*!
 * \brief Constant integer up and lower bound(inclusive).
 *  Useful for value bound analysis.
 *
 *  set = [min_value, max_value]
 */
class ConstIntBoundNode : public Object {
public:
    int64_t min_value;
    int64_t max_value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("min_value", &min_value);
        v->Visit("max_value", &max_value);
    }

    bool SEqualReduce(const ConstIntBoundNode* other, SEqualReducer equal) const {
        return equal(min_value, other->min_value) && equal(max_value, other->max_value);
    }

    /*! \brief Number to represent +inf */
    static constexpr int64_t kPosInf = std::numeric_limits<int64_t>::max();
    /*!
     * \brief Number to represent -inf
     * \note We can make use the of fact that -kPosInf == kNegInf in the project.
     */
    static constexpr int64_t kNegInf = -kPosInf;

    static constexpr const char* _type_key = "arith.ConstIntBound";
    TVM_DECLARE_FINAL_OBJECT_INFO(ConstIntBoundNode, Object);
};

/*!
 * \brief reference class to ConstIntBoundNode
 * \sa ConstIntBoundNode
 */
class ConstIntBound : public ObjectRef {
public:
    /*!
     * \brief constructor by fields.
     * \param min_value The mininum value.
     * \param max_value The maximum value.
     */
    LITETVM_API ConstIntBound(int64_t min_value, int64_t max_value);

    static constexpr int64_t kPosInf = ConstIntBoundNode::kPosInf;
    static constexpr int64_t kNegInf = ConstIntBoundNode::kNegInf;
    TVM_DEFINE_OBJECT_REF_METHODS(ConstIntBound, ObjectRef, ConstIntBoundNode);
};

/*!
 * \brief Analyzer to get constant integer bound over expression.
 */
class ConstIntBoundAnalyzer {
public:
    using BoundMapType = std::unordered_map<PrimExpr, ConstIntBound, ObjectPtrHash, ObjectPtrEqual>;
    /*!
     * \brief analyze the expr
     * \param expr The expression of interest.
     * \return the result of the analysis.
     */
     LITETVM_API ConstIntBound operator()(const PrimExpr& expr) const;

    /*!
     * \brief analyze the expr with the intermediate memorized to avoid redundant computation
     * \param expr The expression of interest.
     * \param bound The lookup table to store the intermediate results
     * \return the result of the analysis.
     */
     LITETVM_API ConstIntBound operator()(const PrimExpr& expr, BoundMapType* bound);

    /*!
     * \brief Update constant int bound information of var.
     *
     * \param var The variable of interest.
     * \param info The bound information.
     * \param allow_override whether we allow override of existing information.
     */
     LITETVM_API void Update(const Var& var, const ConstIntBound& info, bool allow_override = false);
    /*!
     * \brief Bind variable to a range.
     *
     * \param var The variable.
     * \param range The range we bind to.
     * \param allow_override Whether we allow overriding an existing var's range.
     */
     LITETVM_API void Bind(const Var& var, const Range& range, bool allow_override = false);

private:
    friend class Analyzer;
    friend class ConstraintContext;
    explicit ConstIntBoundAnalyzer(Analyzer* parent);
     LITETVM_API ~ConstIntBoundAnalyzer();
    /*!
     * \brief Update the internal state to enter constraint.
     * \param constraint A constraint expression.
     *
     * \return an exit function that must be called to cleanup the constraint can be nullptr.
     */
    std::function<void()> EnterConstraint(const PrimExpr& constraint);
    struct Entry;
    class Impl;
    /*! \brief Internal impl */
    Impl* impl_;
};

}// namespace arith
}// namespace litetvm

#endif//LITETVM_ARITH_ANALYZER_H
