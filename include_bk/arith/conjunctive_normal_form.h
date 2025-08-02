//
// Created by 赵丹 on 25-4-15.
//

#ifndef LITETVM_ARITH_CONJUNCTIVE_NORMAL_FORM_H
#define LITETVM_ARITH_CONJUNCTIVE_NORMAL_FORM_H

#include "tir/expr.h"

namespace litetvm {

namespace arith {

class Analyzer;

/*! \brief Convert boolean expression to AND of ORs and simplify
 *
 * \param expr The PrimExpr to be simplified
 *
 * \param analyzer The analyzer with which to simplify
 *
 * \return The simplified expression
 */
PrimExpr SimplifyAsAndOfOrs(const PrimExpr& expr, Analyzer* analyzer);

}// namespace arith

}// namespace litetvm

#endif//LITETVM_ARITH_CONJUNCTIVE_NORMAL_FORM_H
