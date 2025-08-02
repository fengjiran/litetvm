//
// Created by 赵丹 on 25-4-16.
//

#ifndef LITETVM_TIR_TRANSFORMS_REPLACE_SELECTED_EXPR_H
#define LITETVM_TIR_TRANSFORMS_REPLACE_SELECTED_EXPR_H

#include "tir/expr.h"
#include "tir/stmt.h"
#include "tir/stmt_functor.h"

namespace litetvm {
namespace tir {

/*!
 * \brief Mutator for replacing the expressions selected by a predicate in a statement and/or
          in an expression, which only replace inside of nodes in which it is allowed to perform
          replacecements (given by a second predicate)
 */
class ReplaceSelectedExpr : public StmtExprMutator {
public:
    // Toplevel (static) functions
    static PrimExpr ReplaceSelectedExprInExpr(
            const PrimExpr& expr, std::function<bool(const PrimExpr&)> predicate_selector,
            const PrimExpr& new_expr, std::function<bool(const PrimExpr&)> can_replace_inside);
    static Stmt ReplaceSelectedExprInStmt(const Stmt& stmt,
                                          std::function<bool(const PrimExpr&)> predicate_selector,
                                          const PrimExpr& new_expr,
                                          std::function<bool(const PrimExpr&)> can_replace_inside);

protected:
    // Constructor
    ReplaceSelectedExpr(std::function<bool(const PrimExpr&)> predicate_selector,
                        const PrimExpr& new_expr,
                        std::function<bool(const PrimExpr&)> can_replace_inside);

    PrimExpr VisitExpr(const PrimExpr& expr) override;

private:
    // The predicate used for selecting what will be replaced
    std::function<bool(const PrimExpr&)> predicate_selector_;
    // The expression used for replacing
    const PrimExpr& new_expr_;
    // The predicate used for knowning inside which nodes we can do rewriting
    // (i.e. in which nodes it can recurse)
    std::function<bool(const PrimExpr&)> can_replace_inside_;
};

}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_TRANSFORMS_REPLACE_SELECTED_EXPR_H
