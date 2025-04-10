//
// Created by richard on 4/5/25.
//

#include "arith/int_set.h"
#include "arith/interval_set.h"
#include "runtime/registry.h"
#include "tir/expr.h"
#include "tir/expr_functor.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace litetvm {
namespace arith {

using tir::is_one;
using tir::is_zero;
using tir::make_const;
using tir::make_zero;

PrimExpr SymbolicLimits::pos_inf_ = Var("pos_inf", DataType::Handle());
PrimExpr SymbolicLimits::neg_inf_ = Var("neg_inf", DataType::Handle());

IntervalSet::IntervalSet(PrimExpr min_value, PrimExpr max_value) {
    auto node = make_object<IntervalSetNode>();
    node->min_value = std::move(min_value);
    node->max_value = std::move(max_value);
    data_ = std::move(node);
}

IntervalSet MakeIntervalSet(PrimExpr min_value, PrimExpr max_value) {
    return {std::move(min_value), std::move(max_value)};
}
TVM_REGISTER_GLOBAL("arith.IntervalSet").set_body_typed(MakeIntervalSet);

IntervalSet Intersect(Analyzer* analyzer, IntervalSet a, IntervalSet b) {
    PrimExpr max_value = min(a->max_value, b->max_value);
    PrimExpr min_value = max(a->min_value, b->min_value);
    if ((max_value.dtype().is_int() || max_value.dtype().is_uint()) &&
        (min_value.dtype().is_int() || min_value.dtype().is_uint()) &&
        analyzer->CanProve(max_value < min_value)) {
        return IntervalSet::Empty();
    }

    return {min_value, max_value};
}

IntervalSet Union(Analyzer* analyzer, IntervalSet a, IntervalSet b) {
    if (a->IsEmpty()) return b;
    if (b->IsEmpty()) return a;
    PrimExpr max_value = max(a->max_value, b->max_value);
    PrimExpr min_value = min(a->min_value, b->min_value);
    return {min_value, max_value};
}

// type traits
template<typename OP>
struct is_logical_op {
    static const bool value = false;
};

#define TVM_DECLARE_LOGICAL_OP(OP)      \
    template<>                          \
    struct is_logical_op<tir::OP> {     \
        static const bool value = true; \
    };

TVM_DECLARE_LOGICAL_OP(And);
TVM_DECLARE_LOGICAL_OP(Or);
TVM_DECLARE_LOGICAL_OP(EQ);
TVM_DECLARE_LOGICAL_OP(NE);
TVM_DECLARE_LOGICAL_OP(GE);
TVM_DECLARE_LOGICAL_OP(GT);
TVM_DECLARE_LOGICAL_OP(LE);
TVM_DECLARE_LOGICAL_OP(LT);
TVM_DECLARE_LOGICAL_OP(Not);

}// namespace arith
}// namespace litetvm