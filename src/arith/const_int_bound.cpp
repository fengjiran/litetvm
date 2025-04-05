//
// Created by richard on 4/5/25.
//

#include "arith/analyzer.h"
#include "arith/int_operator.h"
#include "runtime/registry.h"
#include "tir/builtin.h"

namespace litetvm {
namespace arith {

using namespace tir;

TVM_REGISTER_NODE_TYPE(ConstIntBoundNode);

ConstIntBound::ConstIntBound(int64_t min_value, int64_t max_value) {
    auto node = make_object<ConstIntBoundNode>();
    node->min_value = min_value;
    node->max_value = max_value;
    data_ = std::move(node);
}

ConstIntBound MakeConstIntBound(int64_t min_value, int64_t max_value) {
    return {min_value, max_value};
}

TVM_REGISTER_GLOBAL("arith.ConstIntBound").set_body_typed(MakeConstIntBound);

inline void PrintBoundValue(std::ostream& os, int64_t val) {
    if (val == ConstIntBound::kPosInf) {
        os << "pos_inf";
    } else if (val == ConstIntBound::kNegInf) {
        os << "neg_inf";
    } else {
        os << val;
    }
}


TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<ConstIntBoundNode>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const ConstIntBoundNode*>(node.get());
            p->stream << "ConstIntBound[";
            PrintBoundValue(p->stream, op->min_value);
            p->stream << ',';
            PrintBoundValue(p->stream, op->max_value);
            p->stream << ']';
        });



}// namespace arith
}// namespace litetvm