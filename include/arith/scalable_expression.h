//
// Created by 赵丹 on 25-4-11.
//

#ifndef LITETVM_ARITH_SCALABLE_EXPRESSION_H
#define LITETVM_ARITH_SCALABLE_EXPRESSION_H

#include "arith/analyzer.h"
#include "target/target.h"
#include "tir/expr.h"

#include <optional>
#include <vector>

namespace litetvm {
namespace arith {

/*! \brief A list of known vscale values to try for an AArch64 SVE target. */
static const std::vector<unsigned int> kAArch64VScaleValues = {1, 2, 4, 8, 16};

/*!
 * \brief Check if an expr is a call to the vscale intrinsic.
 * \param expr The expr to check
 * \return True if the expr is a call to the vscale intrinsic, false if not.
 */
bool IsVScaleCall(const PrimExpr& expr);

/*!
 * \brief Check if an expr contains a call to the vscale intrinsic.
 * \param expr The expr to check
 * \return True if the expr contains a call to the vscale intrinsic, false if not.
 */
bool ContainsVscaleCall(const PrimExpr& expr);

/*!
 * \brief Substitute a vscale intrinsic call with a known scalar value.
 * \param expr The expr to apply substitutions to.
 * \param vscale_value The scalar value to replace vscale with.
 * \return A rewritten expression with vscale values replaced with a scalar value.
 */
PrimExpr SubstituteVScaleWithKnownValue(const PrimExpr& expr, unsigned int vscale_value);

/*!
 * \brief Returns the vscale multiplier as a nullable type
 * \param lanes The scalable lanes as a PrimExpr
 * \return vscale multiplier as std::optional<int>
 */
std::optional<int> ExtractVscaleFactor(const PrimExpr& lanes);

/*!
 * \brief Check if the expression can be proven when evaluating it on all possible values
           of vscale.
 * \param analyzer An analyzer instance.
 * \param expr The expression to try to prove.
 * \param vscale_values A list of values to substitute vscale with.
 * \return Whether or not the expression can be proven with this technique.
 */
bool CanProveVscaleExpressionFromKnownValues(arith::Analyzer* analyzer, const PrimExpr& expr,
                                             const std::vector<unsigned int>& vscale_values);

/*!
 * \brief Check whether the compilation target supports SVE
 * \param target The target to check.
 * \return Whether SVE is supported
 */
bool TargetHasSVE(Optional<Target> target = NullOpt);


}// namespace arith
}// namespace litetvm

#endif//LITETVM_ARITH_SCALABLE_EXPRESSION_H
