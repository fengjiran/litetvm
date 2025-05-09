//
// Created by 赵丹 on 25-4-14.
//

#include "relax/struct_info.h"
#include "relax/expr.h"
#include "runtime/registry.h"

namespace litetvm {
namespace relax {

ObjectStructInfo::ObjectStructInfo() {
    ObjectPtr<ObjectStructInfoNode> n = make_object<ObjectStructInfoNode>();
    // n->span = span;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(ObjectStructInfoNode);

TVM_REGISTER_GLOBAL("relax.ObjectStructInfo").set_body_typed([]() {
    return ObjectStructInfo();
});

// Prim
PrimStructInfo::PrimStructInfo(PrimExpr value) {
    ObjectPtr<PrimStructInfoNode> n = make_object<PrimStructInfoNode>();
    n->dtype = value->dtype;
    n->value = std::move(value);
    // n->span = span;
    data_ = std::move(n);
}

PrimStructInfo::PrimStructInfo(DataType dtype) {
    ObjectPtr<PrimStructInfoNode> n = make_object<PrimStructInfoNode>();
    n->dtype = dtype;
    n->value = NullOpt;
    // n->span = span;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(PrimStructInfoNode);

TVM_REGISTER_GLOBAL("relax.PrimStructInfoFromDtype").set_body_typed([](DataType dtype) {
    return PrimStructInfo(dtype);
});

TVM_REGISTER_GLOBAL("relax.PrimStructInfoFromValue").set_body_typed([](PrimExpr value) {
    return PrimStructInfo(value);
});

// Shape
ShapeStructInfo::ShapeStructInfo(const Array<PrimExpr>& values) {
    ObjectPtr<ShapeStructInfoNode> n = make_object<ShapeStructInfoNode>();
    n->ndim = static_cast<int>(values.size());
    n->values = values.Map([](PrimExpr value) {
        if (value->IsInstance<IntImmNode>()) {
            return cast(DataType::Int(64), value);
        }
        CHECK(value.dtype() == DataType::Int(64))
                << "the value in ShapeStructInfo can only have dtype of int64";
        return value;
    });
    // n->span = span;
    data_ = std::move(n);
}

ShapeStructInfo::ShapeStructInfo(int ndim) {
    ObjectPtr<ShapeStructInfoNode> n = make_object<ShapeStructInfoNode>();
    CHECK_GE(ndim, -1) << "ndim of ShapeStructInfo must be >= -1, but got " << ndim;
    n->ndim = ndim;
    // n->span = span;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(ShapeStructInfoNode);

TVM_REGISTER_GLOBAL("relax.ShapeStructInfo")
        .set_body_typed([](Optional<Array<PrimExpr>> values, int ndim) {
            if (values.defined()) {
                CHECK_EQ(ndim, kUnknownNDim) << "ValueError: Cannot both specify values and ndim";
                return ShapeStructInfo(values.value());
            }
            return ShapeStructInfo(ndim);
        });

// Tensor
TensorStructInfo::TensorStructInfo(Expr shape, DataType dtype, Optional<VDevice> vdevice) {
    ObjectPtr<TensorStructInfoNode> n = make_object<TensorStructInfoNode>();
    // assign ndim before move
    Optional<ShapeStructInfo> sinfo = MatchStructInfo<ShapeStructInfo>(shape);
    CHECK(sinfo) << "We expect shape to contain pre-set shape struct info";
    CHECK(shape.defined()) << "Must provide a shape in this constructor";
    CHECK(shape->IsInstance<ShapeExprNode>() || shape->IsInstance<VarNode>())
            << "We require shape to be normalized when constructing TensorStructInfo";
    n->ndim = sinfo.get()->ndim;
    // assign rest of the fields.
    n->shape = std::move(shape);
    n->dtype = dtype;
    n->vdevice = std::move(vdevice);
    // n->span = span;
    data_ = std::move(n);
}

TensorStructInfo::TensorStructInfo(DataType dtype, int ndim, Optional<VDevice> vdevice) {
    ObjectPtr<TensorStructInfoNode> n = make_object<TensorStructInfoNode>();
    CHECK_GE(ndim, -1) << "ndim of TensorStructInfo must be >= -1, but got " << ndim;
    n->ndim = ndim;
    n->dtype = dtype;
    n->vdevice = std::move(vdevice);
    // n->span = span;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(TensorStructInfoNode);

TVM_REGISTER_GLOBAL("relax.TensorStructInfo")
        .set_body_typed([](Optional<Expr> shape, DataType dtype, int ndim, VDevice vdevice) {
            if (shape.defined()) {
                CHECK_EQ(ndim, kUnknownNDim) << "ValueError: Cannot both specify shape and ndim";
                return TensorStructInfo(shape.value(), dtype, vdevice);
            } else {
                return TensorStructInfo(dtype, ndim, vdevice);
            }
        });


// Tuple
TupleStructInfo::TupleStructInfo(Array<StructInfo> fields) {
    ObjectPtr<TupleStructInfoNode> n = make_object<TupleStructInfoNode>();
    n->fields = std::move(fields);
    // n->span = span;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(TupleStructInfoNode);

TVM_REGISTER_GLOBAL("relax.TupleStructInfo")
        .set_body_typed([](Array<StructInfo> fields) {
            return TupleStructInfo(fields);
        });


// Func
FuncStructInfo::FuncStructInfo(Array<StructInfo> params, StructInfo ret, bool purity) {
    ObjectPtr<FuncStructInfoNode> n = make_object<FuncStructInfoNode>();
    n->params = std::move(params);
    n->ret = std::move(ret);
    n->purity = std::move(purity);
    // n->span = span;
    data_ = std::move(n);
}

FuncStructInfo FuncStructInfo::OpaqueFunc(StructInfoDeriveFunc derive_func, bool purity) {
    ObjectPtr<FuncStructInfoNode> n = make_object<FuncStructInfoNode>();
    n->derive_func = std::move(derive_func);
    n->ret = ObjectStructInfo();
    n->purity = std::move(purity);
    // n->span = span;
    return FuncStructInfo(n);
}

FuncStructInfo FuncStructInfo::OpaqueFunc(StructInfo ret, bool purity) {
    ObjectPtr<FuncStructInfoNode> n = make_object<FuncStructInfoNode>();
    n->ret = std::move(ret);
    n->purity = std::move(purity);
    // n->span = span;
    return FuncStructInfo(n);
}

TVM_REGISTER_NODE_TYPE(FuncStructInfoNode);

TVM_REGISTER_GLOBAL("relax.FuncStructInfo")
        .set_body_typed([](Array<StructInfo> params, StructInfo ret, bool purity) {
            return FuncStructInfo(params, ret, purity);
        });

TVM_REGISTER_GLOBAL("relax.FuncStructInfoOpaqueFunc")
        .set_body_typed([](Optional<StructInfo> ret, Optional<StructInfoDeriveFunc> derive_func,
                           bool purity) {
            if (derive_func.defined()) {
                CHECK(!ret.defined()) << "ValueError: Cannot specify both ret and derive_func";
                return FuncStructInfo::OpaqueFunc(derive_func.value(), purity);
            } else {
                return FuncStructInfo::OpaqueFunc(ret.value_or(ObjectStructInfo()), purity);
            }
        });


// Helper functions
void UpdateStructInfo(Expr expr, StructInfo struct_info) {
    CHECK(!expr->struct_info_.defined())
            << "To ensure idempotency, "
            << "the expression passed to UpdateStructInfo "
            << "must not have any prior StructInfo.  "
            << "However, expression " << expr << " has struct info " << expr->struct_info_
            << ", which cannot be overwritten with " << struct_info;
    expr->struct_info_ = struct_info;
    // also set checked type
    expr->checked_type_ = GetStaticType(struct_info);
}

TVM_REGISTER_GLOBAL("relax.UpdateStructInfo").set_body_typed([](Expr expr, StructInfo struct_info) {
    UpdateStructInfo(expr, struct_info);
});

TVM_REGISTER_GLOBAL("ir.ExprStructInfo").set_body_typed([](Expr expr) {
    return GetStructInfo(expr);
});

}// namespace relax
}// namespace litetvm
