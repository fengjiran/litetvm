//
// Created by 赵丹 on 25-3-24.
//

#include "ir/env_func.h"
#include "node/repr_printer.h"
#include "runtime/registry.h"

namespace litetvm {

using runtime::make_object;
using runtime::PackedFunc;
using runtime::TVMArgs;
using runtime::TVMRetValue;

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<EnvFuncNode>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const EnvFuncNode*>(node.get());
            p->stream << "EnvFunc(" << op->name << ")";
        });

ObjectPtr<Object> CreateEnvNode(const std::string& name) {
    auto* f = runtime::RegistryManager::Global().Get(name);
    CHECK(f != nullptr) << "Cannot find global function \'" << name << '\'';
    ObjectPtr<EnvFuncNode> n = make_object<EnvFuncNode>();
    n->func = *f;
    n->name = name;
    return n;
}

EnvFunc EnvFunc::Get(const String& name) {
    return EnvFunc(CreateEnvNode(name));
}

TVM_REGISTER_GLOBAL("ir.EnvFuncGet").set_body_typed(EnvFunc::Get);

TVM_REGISTER_GLOBAL("ir.EnvFuncCall").set_body([](TVMArgs args, TVMRetValue* rv) {
    EnvFunc env(args[0]);
    CHECK_GE(args.size(), 1);
    env->func.CallPacked(TVMArgs(args.values + 1, args.type_codes + 1, args.size() - 1), rv);
});

TVM_REGISTER_GLOBAL("ir.EnvFuncGetPackedFunc").set_body_typed([](const EnvFunc& n) {
    return n->func;
});

TVM_REGISTER_NODE_TYPE(EnvFuncNode)
        .set_creator(CreateEnvNode)
        .set_repr_bytes([](const Object* n) -> std::string {
            return static_cast<const EnvFuncNode*>(n)->name;
        });

}// namespace litetvm
