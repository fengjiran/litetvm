//
// Created by 赵丹 on 25-4-14.
//

#include "ir/function.h"
#include "runtime/registry.h"
#include "tir/function.h"

namespace litetvm {

TVM_REGISTER_GLOBAL("ir.BaseFunc_Attrs").set_body_typed([](BaseFunc func) { return func->attrs; });

TVM_REGISTER_GLOBAL("ir.BaseFuncCopy").set_body_typed([](BaseFunc func) { return func; });

TVM_REGISTER_GLOBAL("ir.BaseFuncWithAttr")
        .set_body_typed([](BaseFunc func, String key, ObjectRef value) -> BaseFunc {
            if (func->IsInstance<tir::PrimFuncNode>()) {
                return WithAttr(Downcast<tir::PrimFunc>(std::move(func)), key, value);
            } else if (func->IsInstance<relax::FunctionNode>()) {
                return WithAttr(Downcast<relax::Function>(std::move(func)), key, value);
            } else {
                LOG(FATAL) << "Do not support function type " << func->GetTypeKey();
            }
        });

TVM_REGISTER_GLOBAL("ir.BaseFuncWithAttrs")
        .set_body_typed([](BaseFunc func, Map<String, ObjectRef> attr_map) -> BaseFunc {
            if (func->IsInstance<tir::PrimFuncNode>()) {
                return WithAttrs(Downcast<tir::PrimFunc>(std::move(func)), attr_map);
            }
            if (const auto* f = runtime::RegistryManager::Global().Get("relax.FuncWithAttrs")) {
                if (Optional<BaseFunc> ret = (*f)(func, attr_map)) {
                    return ret.value();
                }
            }
            LOG(FATAL) << "Do not support function type " << func->GetTypeKey();
        });

TVM_REGISTER_GLOBAL("ir.BaseFuncWithoutAttr")
        .set_body_typed([](BaseFunc func, String key) -> BaseFunc {
            if (func->IsInstance<tir::PrimFuncNode>()) {
                return WithoutAttr(Downcast<tir::PrimFunc>(std::move(func)), key);
            } else if (func->IsInstance<relax::FunctionNode>()) {
                return WithoutAttr(Downcast<relax::Function>(std::move(func)), key);
            } else {
                LOG(FATAL) << "Do not support function type " << func->GetTypeKey();
                return func;
            }
        });

}// namespace litetvm