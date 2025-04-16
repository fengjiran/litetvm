//
// Created by 赵丹 on 25-4-16.
//

#include "tir/check_contains.h"

namespace litetvm {
namespace tir {

/*!
 * \brief Toplevel (static) function that tells if an expression contains a subexpression that
          satisfies a given predicate.
 * \param expr The expression to check
 * \param predicate The predicate that must be satisfied
 * \return Whether `expr` contains a subexpression that satisfies `predicate`
 */
bool CheckContains::ExprContains(const PrimExpr& expr,
                                 std::function<bool(const PrimExpr&)> predicate) {
    CheckContains check_contains(predicate);
    check_contains.VisitExpr(expr);
    return check_contains.contains_it_;
}

/*!
 * \brief Toplevel (static) function that tells if a statement contains a subexpression that
          satisfies a given predicate.
 * \param stmt The statement to check
 * \param predicate The predicate that must be satisfied
 * \return Whether `stmt` contains a subexpression that satisfies `predicate`
 */
bool CheckContains::StmtContains(const Stmt& stmt, std::function<bool(const PrimExpr&)> predicate) {
    CheckContains check_contains(predicate);
    check_contains.VisitStmt(stmt);
    return check_contains.contains_it_;
}

/*!
 * \brief Protected constructor of CheckContains.
 * \param predicate The predicate that must be satisfied
 */
CheckContains::CheckContains(std::function<bool(const PrimExpr&)> predicate)
    : predicate_(predicate) {}

/*!
 * \brief The method which overrides the generic dispatcher of StmtExprVisitor for expressions.
 * \param expr The expression to visit
 */
void CheckContains::VisitExpr(const PrimExpr& expr) {
    // If the predicate holds on `expr`, we know `expr` contains something which makes
    // the predicate hold
    if (predicate_(expr)) {
        contains_it_ = true;
    } else {
        // Otherwise we continue to look for it recursively by calling the dispatcher
        StmtExprVisitor::VisitExpr(expr);
    }
}

/*!
 * \brief The method which overrides the generic dispatcher of StmtExprVisitor for statements.
 * \param stmt The statement to visit
 */
void CheckContains::VisitStmt(const Stmt& stmt) {
    // We keep exploring only if `contains_it_` is false
    if (!contains_it_) {
        // and in order to do that we call the general dispatcher
        StmtExprVisitor::VisitStmt(stmt);
    }
    // As otherwise we already have our answer
}

}// namespace tir
}// namespace litetvm
