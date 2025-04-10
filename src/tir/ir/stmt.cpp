//
// Created by 赵丹 on 25-4-10.
//

#include "tir/stmt.h"
#include "arith/analyzer.h"
#include "runtime/registry.h"
#include "tir/op.h"
#include "tir/op_attr_types.h"

namespace litetvm {
namespace tir {

// Evaluate
Evaluate::Evaluate(PrimExpr value) {
    CHECK(value.defined());

    ObjectPtr<EvaluateNode> node = make_object<EvaluateNode>();
    node->value = std::move(value);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.Evaluate").set_body_typed([](PrimExpr value) {
    return Evaluate(value);
});

TVM_REGISTER_NODE_TYPE(EvaluateNode);

// SeqStmt
SeqStmt::SeqStmt(Array<Stmt> seq) {
    bool requires_flattening = std::any_of(
            seq.begin(), seq.end(), [](const Stmt& stmt) { return stmt->IsInstance<SeqStmtNode>(); });

    if (requires_flattening) {
        auto flattened = SeqStmt::Flatten(seq);
        if (auto* ptr = flattened.as<SeqStmtNode>()) {
            seq = ptr->seq;
        } else {
            seq = {flattened};
        }
    }

    CHECK_NE(seq.size(), 0) << "An empty SeqStmt is prohibited.  "
                            << "To write a no-op, use Evaluate(0), "
                            << "or the result of SeqStmt::Flatten()";
    CHECK_NE(seq.size(), 1) << "A SeqStmt of length 1 is prohibited.  "
                            << "Use the node " << seq[0] << "directly, "
                            << "or for dynamic usage, normalize using SeqStmt::Flatten()";

    auto node = make_object<SeqStmtNode>();
    node->seq = std::move(seq);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.SeqStmt").set_body_typed([](Array<Stmt> seq) {
    return SeqStmt(std::move(seq));
});

TVM_REGISTER_NODE_TYPE(SeqStmtNode);

}// namespace tir
}// namespace litetvm
