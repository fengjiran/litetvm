//
// Created by 赵丹 on 25-5-14.
//
#include "ir/op.h"
#include "tir/analysis.h"
#include "tir/expr.h"
#include "tir/expr_functor.h"
#include "tir/op_attr_types.h"

namespace litetvm {
namespace tir {

class ExprSideEffect : public ExprVisitor {
public:
    void VisitExpr(const PrimExpr& e) final {
        if (kind_ == CallEffectKind::kUpdateState) return;
        ExprVisitor::VisitExpr(e);
    }

    void VisitExpr_(const BufferLoadNode* op) final {
        this->UpdateEffect(CallEffectKind::kReadState);
        ExprVisitor::VisitExpr_(op);
    }

    void VisitExpr_(const CallNode* op) final {
        static auto op_call_effect = Op::GetAttrMap<TCallEffectKind>("TCallEffectKind");

        if (auto opt = op->op.as<Op>()) {
            this->UpdateEffect(static_cast<CallEffectKind>(op_call_effect[opt.value()]->value));
        } else {
            this->UpdateEffect(CallEffectKind::kOpaque);
        }
        ExprVisitor::VisitExpr_(op);
    }

    void UpdateEffect(CallEffectKind effect_kind) {
        if (effect_kind > CallEffectKind::kUpdateState) {
            effect_kind = CallEffectKind::kUpdateState;
        }
        if (effect_kind > kind_) {
            kind_ = effect_kind;
        }
    }

    CallEffectKind kind_{CallEffectKind::kPure};
};

CallEffectKind SideEffect(const PrimExpr& e) {
    ExprSideEffect visitor;
    visitor(e);
    return visitor.kind_;
}

}// namespace tir
}// namespace litetvm
