//
// Created by 赵丹 on 25-4-11.
//

#include "arith/scalable_expression.h"
#include "arith/pattern_match.h"
#include "tir/expr.h"
#include "tir/op.h"
#include "tir/check_contains.h"
#include "tir/replace_selected_expr.h"

#include <vector>

namespace litetvm {
namespace arith {

bool IsVScaleCall(const PrimExpr& expr) {
    if (auto call = expr.as<tir::CallNode>()) {
        return call->op.same_as(tir::builtin::vscale());
    }
    return false;
}

bool ContainsVscaleCall(const PrimExpr& expr) {
    return tir::CheckContains::ExprContains(expr, IsVScaleCall);
}

PrimExpr SubstituteVScaleWithKnownValue(const PrimExpr& expr, unsigned int vscale_value) {
    std::function<bool(const PrimExpr&)> predicate_selector = [](const PrimExpr& current_expr) {
        return IsVScaleCall(current_expr);
    };
    std::function<bool(const PrimExpr&)> can_replace_inside = [](const PrimExpr& current_expr) {
        return true;
    };

    return tir::ReplaceSelectedExpr::ReplaceSelectedExprInExpr(
            expr, predicate_selector, tir::MakeConstScalar(DataType::Int(32), vscale_value),
            can_replace_inside);
}

std::optional<int> ExtractVscaleFactor(const PrimExpr& lanes) {
    PVar<IntImm> multiplier;
    PCallExpr<PVscaleOp> vscale;

    if (PMatchesOneOf(multiplier * vscale, vscale * multiplier).Match(lanes)) {
        return multiplier.Eval()->value;
    } else {
        return std::nullopt;
    }
}

bool CanProveVscaleExpressionFromKnownValues(arith::Analyzer* analyzer, const PrimExpr& expr,
                                             const std::vector<unsigned int>& vscale_values) {
    bool can_prove_expr = true;
    for (const unsigned int vscale_value: vscale_values) {
        PrimExpr result = SubstituteVScaleWithKnownValue(expr, vscale_value);
        result = analyzer->Simplify(result);
        const int64_t* as_int = tir::as_const_int(result);
        if (!as_int || *as_int == 0) {
            can_prove_expr = false;
            break;
        }
    }
    return can_prove_expr;
}

bool TargetHasSVE(Optional<Target> target) {
    if (!target.defined()) {
        target = Target::Current();
    }
    if (target.defined()) {
        return Downcast<Target>(target)->GetFeature<Bool>("has_sve").value_or(Bool(false));
    }
    return false;
}

}// namespace arith
}// namespace litetvm