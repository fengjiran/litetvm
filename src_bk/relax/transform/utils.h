//
// Created by 赵丹 on 25-4-17.
//

#ifndef LITETVM_RELAX_TRANSFORM_UTILS_H
#define LITETVM_RELAX_TRANSFORM_UTILS_H

#include "ir/module.h"
#include "relax/expr.h"
#include "relax/expr_functor.h"
#include "tir/expr_functor.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace litetvm {
namespace relax {

/*!
 * \brief Renew the definition of symbolic vars in Relax.
 * \details This mutator is used to prevent the same symbolic var from being used in different
 *          functions, which is malformed.
 */
class SymbolicVarRenewMutator : public ExprMutator, tir::ExprMutator {
public:
    static Function Renew(const Function& function) {
        SymbolicVarRenewMutator mutator;
        return Downcast<Function>(mutator.VisitExpr(function));
    }
    SymbolicVarRenewMutator() = default;

protected:
    using relax::ExprMutator::VisitExpr;
    using relax::ExprMutator::VisitExpr_;
    using tir::ExprMutator::VisitExpr_;

    PrimExpr VisitPrimExpr(const PrimExpr& expr) final { return tir::ExprMutator::VisitExpr(expr); }

    // TODO(Siyuan): enhance the method to the following steps:
    // 1. Visit and replace all tir::Vars at the definition point
    // 2. Revisit the function again and update the use side.
    PrimExpr VisitExpr_(const tir::VarNode* op) final {
        auto it = var_map_.find(GetRef<tir::Var>(op));
        if (it != var_map_.end()) {
            return (*it).second;
        } else {
            auto n = make_object<tir::VarNode>(*op);
            tir::Var v(n);
            var_map_.Set(GetRef<tir::Var>(op), v);
            return v;
        }
    }

    Expr VisitExpr_(const FunctionNode* op) {
        Array<Var> params;
        bool all_params_unchanged = true;
        for (Var param: op->params) {
            Var new_param = this->VisitVarDef(param);
            params.push_back(new_param);
            if (!param.same_as(new_param)) {
                var_remap_[param->vid] = new_param;
                all_params_unchanged = false;
            }
        }

        Expr body = this->VisitWithNewScope(op->body, params);

        if (all_params_unchanged && body.same_as(op->body)) {
            return GetRef<Expr>(op);
        }
        auto new_ret_sinfo = this->VisitExprDepStructInfoField(op->ret_struct_info);
        return Function(params, body, new_ret_sinfo, op->is_pure, op->attrs);
    }

    Map<tir::Var, tir::Var> var_map_;
};

/*!
 * \brief Copy a function while renewing the relax Vars and the tir Vars.
 * \details All variables that are bound inside the original function would be copied to satisfy
 * the restriction in the well-formed check: Variables in Relax must be bound exactly once.
 */
class FunctionCopier : public SymbolicVarRenewMutator {
public:
    FunctionCopier() = default;
    Function Copy(Function func) { return Downcast<Function>(VisitExpr(func)); }
    Map<Var, Var> GetVarMap() { return relax_var_map_; }

private:
    using relax::ExprMutator::VisitExpr;

    Var VisitVarDef_(const DataflowVarNode* var) override {
        Var new_var = SymbolicVarRenewMutator::VisitVarDef_(var);
        Var copied_var = DataflowVar(new_var->name_hint(), GetStructInfo(new_var));
        var_remap_[var->vid] = copied_var;
        relax_var_map_.Set(GetRef<Var>(var), copied_var);
        return copied_var;
    }

    Var VisitVarDef_(const VarNode* var) override {
        Var new_var = SymbolicVarRenewMutator::VisitVarDef_(var);
        Var copied_var = Var(new_var->name_hint(), GetStructInfo(new_var));
        var_remap_[var->vid] = copied_var;
        relax_var_map_.Set(GetRef<Var>(var), copied_var);
        return copied_var;
    }

    Map<Var, Var> relax_var_map_;
};

}// namespace relax
}// namespace litetvm

#endif//LITETVM_RELAX_TRANSFORM_UTILS_H
