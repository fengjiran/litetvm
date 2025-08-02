//
// Created by 赵丹 on 25-4-16.
//

#ifndef LITETVM_TIR_CHECK_CONTAINS_H
#define LITETVM_TIR_CHECK_CONTAINS_H

#include "tir/expr.h"
#include "tir/stmt_functor.h"

namespace litetvm {
namespace tir {

/*!
 * \brief Visitor which tells if a given expression or statement contains a subexpression
          that satisfies a given predicate
 */
class CheckContains : public StmtExprVisitor {
public:
    // Toplevel (static) functions
    static bool ExprContains(const PrimExpr& expr, std::function<bool(const PrimExpr&)> predicate);
    static bool StmtContains(const Stmt& stmt, std::function<bool(const PrimExpr&)> predicate);

protected:
    // Constructor
    explicit CheckContains(std::function<bool(const PrimExpr&)> predicate);

    void VisitExpr(const PrimExpr& expr) override;
    void VisitStmt(const Stmt& stmt) override;

private:
    std::function<bool(const PrimExpr&)> predicate_;
    bool contains_it_ = false;
};

}// namespace tir

}// namespace litetvm

#endif//LITETVM_TIR_CHECK_CONTAINS_H
