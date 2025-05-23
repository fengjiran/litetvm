//
// Created by richard on 5/13/25.
//

/*!
 * \file tvm/relax/utils.h
 * \brief Utility classes and functions for working with the Relax IR.
 */
#ifndef LITETVM_RELAX_UTILS_H
#define LITETVM_RELAX_UTILS_H

#include "arith/analyzer.h"
#include "ir/module.h"
#include "relax/expr.h"

namespace litetvm {
namespace relax {

/*!
 * \brief Bind the variables to a Relax expression. This is a helper
 * function usually called by other pass functions to help optimizations.
 * If any free variables are introduced into a function, those are added
 * to the function parameters.
 * Additionally this may change the order of parameters if you map a variable
 * to a variable.
 *
 * \param expr The input expression.
 * \param binds The variable to expression map that will be used to help the
 *        binding.
 * \param symbolic_var_map The map from symbolic var to the expr it binds to.
 *
 * \return The updated expression.
 */
LITETVM_API Expr Bind(const Expr& expr, const Map<Var, Expr>& binds,
                      const Map<tir::Var, PrimExpr>& symbolic_var_map = {});

/*!
 * \brief Bind the symbolic variables to a StructInfo. This is a helper function usually called by
 * other pass functions to help optimizations.
 */
LITETVM_API StructInfo Bind(const StructInfo& sinfo,
                            const Map<tir::Var, PrimExpr>& symbolic_var_map);

/*!
 * \brief Infer a binding map for symbolic variables
 *
 * If a set of relax variables are replaced within an expression, this
 * may result in removal of the definition site of a symbolic
 * variable.  This utility function determines the symbolic variable
 * replacements that can be inferred based on the replaced relax
 * variables, and can be used alongside the `Bind` utility function to
 * replace both the relax variables and the implied symbolic
 * variables.
 *
 * \param binds A map of relax variables to relax expressions
 *
 * \param analyzer The analyzer to use for simplifications
 *
 * \return A map of TIR variables to TIR expressions
 */
LITETVM_API Map<tir::Var, PrimExpr> InferSymbolicVarMap(
        const Map<Var, Expr>& binds, arith::Analyzer* analyzer);

/*!
 * \brief Check if the given StructInfo is for a boolean scalar (tensor of rank 0 with a boolean
 * dtype).
 *
 * \param sinfo The input StructInfo.
 * \param permit_unknown_rank If true, it will permit the input type to have unknown rank
 *   (ndim of -1), which will require a dynamic check.
 * \param permit_unknown_dtype If true, it will permit the input type to have an unknown dtype
 *   (namely, void), which will require a dynamic check.
 *
 * \return True iff the input type is a boolean scalar type (or, depending on options, has unknown
 *   rank or dtype)
 */
LITETVM_API bool IsBoolStructInfo(const StructInfo& sinfo, bool permit_unknown_rank = true,
                                  bool permit_unknown_dtype = true);

/*!
 * \brief Check if the given expression is a "leaf" node or tuple node for normalization purposes.
 *
 *    The following expressions are defined as leaf nodes: Var, Constant, ShapeExpr,
 *    GlobalVar, Op, ExternFunc.
 *
 *    Tuples are included in this list mainly for convenience in grouping operator arguments.
 *    *Note*: Since tuples can contain nested expressions, it is necessary to ensure that
 *    values nested inside them are also leaves.
 *
 * \param expr The input expression
 *
 * \return True iff the input expression is a "leaf" node (a value allowed to appear
 *    inline without being bound to a var during normalization).
 */
LITETVM_API bool IsLeafOrTuple(const Expr& expr);

/*!
 * \brief Check if the given Call node is an impure operation. If the callee is a general
 * expression, this simply requires checking the purity field of the FuncStructInfo. If it is an Op,
 * then this checks the `fPurity` field.
 *
 * \param call The input call
 *
 * \return True iff the call is impure (definitely or possibly results in a visible side effect).
 *   That is, a call is considered pure only if definitely does not result in a visible side effect.
 */
LITETVM_API bool IsImpureCall(const Call& call);

/*!
 * \brief Copy the given function. All variables that are bound inside the original function
 *  would be copied to satisfy the restriction in the well-formed check: Variables in
 *  Relax must be bound exactly once. This also ensures that both the function and its copy
 *  can be inserted into the same IRModule, and be asserted on the structural equality
 *  agaisnt IRModule created by TVMScript.
 *
 * \param func The relax function to copy.
 * \return The copied function.
 */
LITETVM_API Function CopyWithNewVars(Function func);

/*!
 * \brief Transform all dataflow structure to non-dataflow version.
 */
Expr ToNonDataflow(const Expr& e);

/*!
 * \brief Get the value bound in the binding.
 */
Expr GetBoundValue(const Binding& b);

}// namespace relax
}// namespace litetvm

#endif//LITETVM_RELAX_UTILS_H
