//
// Created by 赵丹 on 25-4-15.
//

#include "node/object_path.h"
#include "node/reflection.h"
#include "node/structural_equal.h"
#include "runtime/registry.h"
#include "tir/analysis.h"

namespace litetvm {
namespace tir {

class DeepCmpSEqualHandler : public SEqualReducer::Handler {
public:
    // use direct recursion.
    bool SEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars, const Optional<ObjectPathPair>&) final {
        if (lhs.same_as(rhs)) return true;
        if (!lhs.defined() && rhs.defined()) return false;
        if (!rhs.defined() && lhs.defined()) return false;
        if (lhs->type_index() != rhs->type_index()) return false;
        return vtable_->SEqualReduce(lhs.get(), rhs.get(), SEqualReducer(this, nullptr, false)) &&
               !fail_;
    }

    void DeferFail(const ObjectPathPair&) final {
        fail_ = true;
    }

    bool IsFailDeferralEnabled() final {
        return false;
    }

    ObjectRef MapLhsToRhs(const ObjectRef& lhs) final {
        return ObjectRef(nullptr);
    }
    
    void MarkGraphNode() final {}

private:
    // reflection vtable
    ReflectionVTable* vtable_ = ReflectionVTable::Global();
    bool fail_ = false;
};

bool ExprDeepEqual::operator()(const PrimExpr& lhs, const PrimExpr& rhs) const {
    // quick path
    if (lhs.same_as(rhs)) return true;
    if (!lhs.defined() && rhs.defined()) return false;
    if (!rhs.defined() && lhs.defined()) return false;
    if (lhs->type_index() != rhs->type_index()) return false;
    if (auto* plhs = lhs.as<IntImmNode>()) {
        auto* prhs = rhs.as<IntImmNode>();
        return plhs->dtype == prhs->dtype && plhs->value == prhs->value;
    }
    return DeepCmpSEqualHandler().SEqualReduce(lhs, rhs, false, NullOpt);
}

TVM_REGISTER_GLOBAL("tir.analysis.expr_deep_equal")
        .set_body_typed([](const PrimExpr& lhs, const PrimExpr& rhs) {
            return ExprDeepEqual()(lhs, rhs);
        });

}// namespace tir
}// namespace litetvm
