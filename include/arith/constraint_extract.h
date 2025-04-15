//
// Created by 赵丹 on 25-4-15.
//

#ifndef LITETVM_ARITH_CONSTRAINT_EXTRACT_H
#define LITETVM_ARITH_CONSTRAINT_EXTRACT_H

#include "tir/expr.h"

#include <vector>

namespace litetvm {
namespace arith {

/* \brief Returns constraints that are true if the expression is true.
 *
 * Utility to break up a boolean expression into independent
 * constraints.
 *
 * Example: `i==5 && j==3` => `[i==5 && j==3, i==5, j==3]`
 * Example: `i==5 || j==3` => `[i==5 || j==3]`
 * Example: `!(i>5 || j==3)` => `[!(i==5 || j==3), i<=5, j!=3]`
 *
 * If `keep_composite_constraints` is true (default), a constraint
 * that can be decomposed will be included in the output.  If false,
 * they will be excluded.
 *
 * Example, removing composite: `!(i>5 || j==3)` => `[i<=5, j!=3]`
 *
 * Intended for use in bounds analysis or simplification within a
 * conditional, or identifying independent conditionals that may be
 * hoisted.
 *
 * \param expr The expression to be analyzers
 *
 * \param keep_composite_constraints Whether to include composite
 * constraints in the output.
 *
 * \returns A vector of independent constraints
 */
std::vector<PrimExpr> ExtractConstraints(const PrimExpr& expr,
                                         bool keep_composite_constraints = true);

/* \brief Returns components that are false if the expression is false.
 *
 * Utility to break up a boolean expression into independent
 * components.
 *
 * Example: `i==5 || j==3` => `[i==5, j==3]`
 * Example: `i==5 && j==3` => `[i==5 && j==3]`
 * Example: `!(i>5 && j==3)` => `[i<=5, j!=3]`
 *
 * Intended for use in bounds analysis or simplification within a
 * conditional, or identifying independent conditionals that may be
 * hoisted.
 *
 * \param expr The expression to be analyzers
 *
 * \returns A vector of independent constraints
 */
std::vector<PrimExpr> ExtractComponents(const PrimExpr& expr);


}// namespace arith
}// namespace litetvm

#endif//LITETVM_ARITH_CONSTRAINT_EXTRACT_H
