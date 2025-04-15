//
// Created by 赵丹 on 25-4-15.
//

#ifndef LITETVM_TIR_ANALYSIS_H
#define LITETVM_TIR_ANALYSIS_H

#include "target/target.h"
#include "tir/function.h"
#include "tir/op_attr_types.h"
#include "tir/stmt.h"
// #include "arith/analyzer.h"

#include <optional>
#include <string>

namespace litetvm {

namespace arith {
class Analyzer;
}

namespace tir {

/*!
 * \brief Compare two expressions recursively and check if they are equal
 *        to each other without var remapping.
 *
 *  This function does not remap variable bindings, it will not
 *  return true for (let x = 1 in x + 1) vs (let y = 1 in y + 1), unless x.same_as(y).
 *
 *  Use StructuralEqual for such cases.
 *
 *  Due to the restriction of not remapping variables, this function can run
 *  faster than StructuralEqual and can be used as a utility function during arithmetic
 *  simplifications.
 *
 * \sa StructuralEqual
 */
struct ExprDeepEqual {
    LITETVM_API bool operator()(const PrimExpr& lhs, const PrimExpr& rhs) const;
};

/*!
 * \brief Visit the PrimFuncs in the IRModule
 * \tparam FLambda The type of the PrimFunc visitor
 * \param mod The IRModule to be visited
 * \param fvisit The visitor to the PrimFuncs in the IRModule
 */
template<class FLambda>
void VisitPrimFuncs(const IRModule& mod, FLambda fvisit) {
    for (const auto& kv: mod->functions) {
        const BaseFunc& base_func = kv.second;
        if (const auto* prim_func = base_func.as<PrimFuncNode>()) {
            fvisit(prim_func);
        }
    }
}

}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_ANALYSIS_H
