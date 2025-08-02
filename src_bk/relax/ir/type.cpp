//
// Created by 赵丹 on 25-3-21.
//

#include "relax/type.h"
#include "runtime/registry.h"

namespace litetvm {
namespace relax {

TVM_REGISTER_NODE_TYPE(ShapeTypeNode);

ShapeType::ShapeType(int ndim) {
    ObjectPtr<ShapeTypeNode> n = make_object<ShapeTypeNode>();
    n->ndim = ndim;
    data_ = std::move(n);
}

TVM_REGISTER_GLOBAL("relax.ShapeType").set_body_typed([](int ndim) {
    return ShapeType(ndim);
});

TVM_REGISTER_NODE_TYPE(TensorTypeNode);

TensorType::TensorType(int ndim, DataType dtype) {
    ObjectPtr<TensorTypeNode> n = make_object<TensorTypeNode>();
    n->ndim = ndim;
    n->dtype = dtype;
    data_ = std::move(n);
}

TensorType TensorType::CreateUnknownNDim(DataType dtype) {
    ObjectPtr<TensorTypeNode> n = make_object<TensorTypeNode>();
    n->ndim = -1;
    n->dtype = dtype;
    return TensorType(std::move(n));
}

TVM_REGISTER_GLOBAL("relax.TensorType").set_body_typed([](int ndim, DataType dtype) {
    return TensorType(ndim, dtype);
});

TVM_REGISTER_NODE_TYPE(ObjectTypeNode);


ObjectType::ObjectType() {
    ObjectPtr<ObjectTypeNode> n = make_object<ObjectTypeNode>();
    data_ = std::move(n);
}

TVM_REGISTER_GLOBAL("relax.ObjectType").set_body_typed([]() { return ObjectType(); });


PackedFuncType::PackedFuncType() {
    ObjectPtr<PackedFuncTypeNode> n = make_object<PackedFuncTypeNode>();
    // n->span = span;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(PackedFuncTypeNode);

TVM_REGISTER_GLOBAL("relax.PackedFuncType").set_body_typed([]() {
    return PackedFuncType();
});

}// namespace relax
}// namespace litetvm
