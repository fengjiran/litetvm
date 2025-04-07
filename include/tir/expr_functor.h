//
// Created by 赵丹 on 25-4-7.
//

#ifndef LITETVM_TIR_EXPR_FUNCTOR_H
#define LITETVM_TIR_EXPR_FUNCTOR_H

#include "node/functor.h"
#include "tir/expr.h"

namespace litetvm {
namespace tir {

/*!
 * \brief A dynamical functor that dispatches on in the first Expr argument.
 *  You can use this as a more powerful Visitor, since it allows you to
 *  define function signatures of Visit Function.
 *
 *  This helps you to avoid to book-keep return value of Visitor via state,
 *  which can cause bugs easily when state is incorrectly maintained.
 *
 * \code
 *  // A functor that set variable to b. and calculate results.
 *  class MyExprFunctor
 *    : public tir::ExprFunctor<int(const Expr&, int)> {
 *   public:
 *    int VisitExpr_(const Variable* op, int b) final {
 *     return b;
 *    }
 *    int VisitExpr_(const IntImm* op, int b) final {
 *      return op->value;
 *    }
 *    int VisitExpr_(const Add* op, int b) final {
 *     return Visit(op->a, b) + Visit(op->b, b);
 *    }
 *  };
 *  MyExprFunctor f;
 *  Var x("x");
 *  ICHECK_EQ(f(x + 1, 2), 3);
 * \endcode
 *
 * \note Why do we need this more powerful Functor:
 *
 *  We often need to implement a transformer tasks.
 *  Say we want to take Expr and transform it to some analysis result;
 *  This easily be done incorrectly using plain Visitor. See IRVisitor's
 *  document for possible error cases.
 *
 * \tparam FType function signiture
 *  This type if only defined for FType with function signiture R(const Expr&, Args...)
 */
template<typename FType>
class ExprFunctor;

// functions to be overriden.
#define EXPR_FUNCTOR_DEFAULT \
    { return VisitExprDefault_(op, std::forward<Args>(args)...); }

#define IR_EXPR_FUNCTOR_DISPATCH(OP)                                                           \
    vtable.template set_dispatch<OP>([](const ObjectRef& n, TSelf* self, Args... args) {       \
        return self->VisitExpr_(static_cast<const OP*>(n.get()), std::forward<Args>(args)...); \
    });



}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_EXPR_FUNCTOR_H
