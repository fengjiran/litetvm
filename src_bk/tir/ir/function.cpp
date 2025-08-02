//
// Created by 赵丹 on 25-4-14.
//

#include "tir/function.h"
#include "relax/expr.h"
#include "relax/struct_info.h"
#include "runtime/registry.h"
#include "tir/analysis.h"
#include "tir/op.h"
#include "tir/utils.h"

namespace litetvm {
namespace tir {

namespace {
relax::StructInfo InferStructInfo(const PrimFunc& prim_func) {
    Array<relax::StructInfo> params;
    for (const auto& param: prim_func->params) {
        relax::StructInfo param_sinfo = [&]() -> relax::StructInfo {
            if (auto opt_buf = prim_func->buffer_map.Get(param)) {
                auto buf = opt_buf.value();
                relax::ShapeExpr shape(
                        buf->shape.Map([](PrimExpr dim) { return cast(DataType::Int(64), dim); }));
                return relax::TensorStructInfo(shape, buf->dtype);
            }

            if (auto prim_type = param->type_annotation.as<PrimTypeNode>();
                prim_type && prim_type->dtype.is_handle()) {
                return relax::ObjectStructInfo();
            }

            return relax::PrimStructInfo(param->dtype);
        }();
        params.push_back(param_sinfo);
    }

    relax::StructInfo ret = [&]() -> relax::StructInfo {
        if (const auto* prim = prim_func->ret_type.as<PrimTypeNode>()) {
            return relax::PrimStructInfo(prim->dtype);
        } else if (IsVoidType(prim_func->ret_type)) {
            return relax::TupleStructInfo(Array<relax::StructInfo>{});
        } else {
            return relax::ObjectStructInfo();
        }
    }();

    bool purity = prim_func->body.defined() ? IsPureFunction(prim_func) : false;

    return relax::FuncStructInfo(params, ret, purity);
}
}// namespace

// Get the function type of a PrimFunc
PrimFunc::PrimFunc(Array<tir::Var> params, Stmt body, Type ret_type,
                   Map<tir::Var, Buffer> buffer_map, DictAttrs attrs) {
    if (!attrs.defined()) {
        attrs = DictAttrs();
    }

    // Assume void-return type for now
    // TODO(tvm-team) consider type deduction from body.
    if (!ret_type.defined()) {
        ret_type = VoidType();
    }

    if (attrs.defined()) {
        attrs = Downcast<DictAttrs>(NormalizeAttributeObject(attrs));
    }

    auto n = make_object<PrimFuncNode>();
    n->params = std::move(params);
    n->body = std::move(body);
    n->ret_type = std::move(ret_type);
    n->buffer_map = std::move(buffer_map);
    n->attrs = std::move(attrs);
    n->checked_type_ = n->func_type_annotation();
    n->struct_info_ = relax::FuncStructInfo::OpaqueFunc();
    // n->span = std::move(span);
    data_ = std::move(n);

    (*this)->struct_info_ = InferStructInfo(*this);
}

FuncType PrimFuncNode::func_type_annotation() const {
    Array<Type> param_types;
    for (auto param: this->params) {
        param_types.push_back(GetType(param));
    }
    return FuncType(param_types, ret_type);
}

TVM_REGISTER_NODE_TYPE(PrimFuncNode);

class TensorIntrinManager {
public:
    Map<String, tir::TensorIntrin> reg;

    static TensorIntrinManager* Global() {
        static TensorIntrinManager* inst = new TensorIntrinManager();
        return inst;
    }
};

TensorIntrin::TensorIntrin(PrimFunc desc, PrimFunc impl) {
    // Check the number of func var is equal
    CHECK_EQ(desc->params.size(), impl->params.size())
            << "ValueError: The number of parameters of the description and the implementation of the "
               "tensor intrinsic doesn't match.";
    for (size_t i = 0; i < desc->params.size(); i++) {
        CHECK(desc->params[i]->dtype.is_handle()) << "ValueError: Parameters of the description of the "
                                                     "tensor intrinsic should be handle only.";
        CHECK(impl->params[i]->dtype.is_handle()) << "ValueError: Parameters of the implementation of "
                                                     "the tensor intrinsic should be handle only.";
    }
    CHECK_EQ(desc->buffer_map.size(), impl->buffer_map.size());

    ObjectPtr<TensorIntrinNode> n = make_object<TensorIntrinNode>();
    n->desc = std::move(desc);
    n->impl = std::move(impl);
    data_ = std::move(n);
}

void TensorIntrin::Register(String name, TensorIntrin intrin, bool override) {
    TensorIntrinManager* manager = TensorIntrinManager::Global();
    if (!override) {
        CHECK_EQ(manager->reg.count(name), 0)
                << "ValueError: TensorIntrin '" << name << "' has already been registered";
    }
    manager->reg.Set(name, intrin);
}

Optional<TensorIntrin> TensorIntrin::Get(String name, bool allow_missing) {
    const TensorIntrinManager* manager = TensorIntrinManager::Global();
    auto it = manager->reg.find(name);
    if (it == manager->reg.end()) {
        if (allow_missing) {
            return NullOpt;
        } else {
            LOG(FATAL) << "ValueError: TensorIntrin '" << name << "' is not registered";
        }
    }
    return (*it).second;
}

TVM_REGISTER_NODE_TYPE(TensorIntrinNode);

TVM_REGISTER_GLOBAL("tir.PrimFunc")
        .set_body_typed([](Array<tir::Var> params, Stmt body, Type ret_type,
                           Map<tir::Var, Buffer> buffer_map, DictAttrs attrs) {
            return PrimFunc(params, body, ret_type, buffer_map, attrs);
        });

TVM_REGISTER_GLOBAL("tir.TensorIntrin")
        .set_body_typed([](PrimFunc desc_func, PrimFunc intrin_func) {
            return TensorIntrin(desc_func, intrin_func);
        });

TVM_REGISTER_GLOBAL("tir.TensorIntrinRegister").set_body_typed(TensorIntrin::Register);
TVM_REGISTER_GLOBAL("tir.TensorIntrinGet").set_body_typed(TensorIntrin::Get);


}// namespace tir
}// namespace litetvm
