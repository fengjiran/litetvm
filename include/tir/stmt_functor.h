//
// Created by 赵丹 on 25-4-11.
//

#ifndef LITETVM_TIR_STMT_FUNCTOR_H
#define LITETVM_TIR_STMT_FUNCTOR_H

#include "node/functor.h"
#include "tir/expr.h"
#include "tir/expr_functor.h"
#include "tir/stmt.h"

#include <unordered_map>

namespace litetvm {
namespace tir {

/*!
 * \brief Same as ExprFunctor except it is applied on statements
 * \tparam FType The function signature.
 * \sa ExprFunctor
 */
template<typename FType>
class StmtFunctor;


}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_STMT_FUNCTOR_H
