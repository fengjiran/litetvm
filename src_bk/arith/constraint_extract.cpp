//
// Created by 赵丹 on 25-4-15.
//

#include "arith/constraint_extract.h"
#include "arith/analyzer.h"
#include "arith/pattern_match.h"

namespace litetvm {
namespace arith {

template<typename F>
void CollectConstraints(PrimExpr expr, F callback, bool keep_composite_constraints) {
    if (keep_composite_constraints) {
        callback(expr);
    }

    PVar<PrimExpr> x, y;
    if ((x && y).Match(expr)) {
        CollectConstraints(x.Eval(), callback, keep_composite_constraints);
        CollectConstraints(y.Eval(), callback, keep_composite_constraints);
    } else if (!keep_composite_constraints) {
        callback(expr);
    }
}

std::vector<PrimExpr> ExtractConstraints(const PrimExpr& expr, bool keep_composite_constraints) {
    std::vector<PrimExpr> out;
    CollectConstraints(
            expr, [&](const PrimExpr& part) { out.push_back(part); }, keep_composite_constraints);
    return out;
}

template<typename F>
void CollectComponents(PrimExpr expr, F callback) {
    PVar<PrimExpr> x, y;
    if ((x || y).Match(expr)) {
        CollectComponents(x.Eval(), callback);
        CollectComponents(y.Eval(), callback);
    } else {
        callback(expr);
    }
}

std::vector<PrimExpr> ExtractComponents(const PrimExpr& expr) {
    std::vector<PrimExpr> out;
    CollectComponents(expr, [&](const PrimExpr& part) { out.push_back(part); });
    return out;
}

}// namespace arith
}// namespace litetvm
