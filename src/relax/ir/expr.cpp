//
// Created by 赵丹 on 25-4-14.
//

#include "relax/expr.h"

#include <unordered_set>

namespace litetvm {
namespace relax {

using litetvm::ReprPrinter;
using runtime::Optional;

TVM_REGISTER_NODE_TYPE(IdNode);

Id::Id(String name_hint) {
    ObjectPtr<IdNode> n = make_object<IdNode>();
    n->name_hint = std::move(name_hint);
    data_ = std::move(n);
}

Call::Call(Expr op, Array<Expr> args, Attrs attrs, Array<StructInfo> sinfo_args) {
    CHECK(!op->struct_info_.defined() || op->struct_info_->IsInstance<FuncStructInfoNode>())
            << "ValueError: "
            << "Call expects its operator to have FuncStructInfo, "
            << "but operator " << op << ", which was called with arguments " << args
            << ", has struct info " << op->struct_info_;

    ObjectPtr<CallNode> n = make_object<CallNode>();
    n->op = std::move(op);
    n->args = std::move(args);
    n->attrs = std::move(attrs);
    n->sinfo_args = std::move(sinfo_args);
    // n->span = std::move(span);
    data_ = std::move(n);
}

}// namespace relax
}// namespace litetvm
