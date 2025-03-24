//
// Created by 赵丹 on 25-3-21.
//

#ifndef LITETVM_RELAX_TYPE_H
#define LITETVM_RELAX_TYPE_H

#include "ir/attrs.h"
#include "ir/type.h"
#include "tir/expr.h"

#include <string>

namespace litetvm {
namespace relax {

/*! \brief Indicates the number of dimensions of a tensor is unknown at compile time. */
static constexpr int kUnknownNDim = -1;

class ShapeTypeNode : public TypeNode {
public:
    /*! \brief size of the shape. */
    int ndim;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("ndim", &ndim);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const ShapeTypeNode* other, SEqualReducer equal) const {
        return equal(ndim, other->ndim);
    }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce(ndim); }

    static constexpr const char* _type_key = "relax.ShapeType";
    TVM_DECLARE_FINAL_OBJECT_INFO(ShapeTypeNode, TypeNode);
};

class ShapeType : public Type {
public:
    // TODO(relax-team): remove the default value later.
    // TVM_DLL ShapeType(int ndim = kUnknownNDim, Span span = Span());
    LITETVM_API ShapeType(int ndim = kUnknownNDim);

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(ShapeType, Type, ShapeTypeNode);
};

/*!
 * \brief Dynamic version of TensorType
 *
 * Use relax::TensorStructInfo for more detailed (possibly dynamic) shape constrains
 */
class TensorTypeNode : public TypeNode {
public:
    /*!
     * \brief The number of dimensions of the tensor, use -1 to denote tensor with unknown number of
     * dimensions.
     */
    int ndim;
    /*! \brief The content data type, use void to denote the dtype is unknown. */
    DataType dtype;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("ndim", &ndim);
        v->Visit("dtype", &dtype);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const TensorTypeNode* other, SEqualReducer equal) const {
        return equal(ndim, other->ndim) && equal(dtype, other->dtype);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(ndim);
        hash_reduce(dtype);
    }

    bool IsUnknownNdim() const {
        return ndim == kUnknownNDim;
    }

    bool IsUnknownDtype() const {
        return dtype.is_void();
    }

    static constexpr const char* _type_key = "relax.DynTensorType";
    TVM_DECLARE_FINAL_OBJECT_INFO(TensorTypeNode, TypeNode);
};

/*!
 * \brief Managed reference to TensorTypeNode.
 * \sa TensorTypeNode.
 */
class TensorType : public Type {
public:
    /*!
     * \brief Constructor.
     * \param ndim The number of dimensions of the tensor.
     * \param dtype The runtime dtype of the tensor's elements.
     * \param span The span.
     */
    // TVM_DLL TensorType(int ndim, DataType dtype, Span span = Span());
    LITETVM_API TensorType(int ndim, DataType dtype);

    /*!
     * \brief Create a TensorType with unknown ndim.
     */
    // TVM_DLL static TensorType CreateUnknownNDim(DataType dtype, Span span = Span());
    LITETVM_API static TensorType CreateUnknownNDim(DataType dtype);

    TVM_DEFINE_OBJECT_REF_METHODS(TensorType, Type, TensorTypeNode);
};

using TensorTypeNode = TensorTypeNode;
using TensorType = TensorType;

class ObjectTypeNode : public TypeNode {
public:
    void VisitAttrs(AttrVisitor* v) {
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const ObjectTypeNode* other, SEqualReducer equal) const {
        return true;
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(0);
    }

    static constexpr const char* _type_key = "relax.ObjectType";
    TVM_DECLARE_FINAL_OBJECT_INFO(ObjectTypeNode, TypeNode);
};

class ObjectType : public Type {
public:
    // TVM_DLL ObjectType(Span span = Span());
    LITETVM_API ObjectType();

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(ObjectType, Type, ObjectTypeNode);
};

class PackedFuncTypeNode : public TypeNode {
public:
    void VisitAttrs(AttrVisitor* v) {
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const PackedFuncTypeNode* other, SEqualReducer equal) const {
        return true;
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(0);
    }

    static constexpr const char* _type_key = "relax.PackedFuncType";
    TVM_DECLARE_FINAL_OBJECT_INFO(PackedFuncTypeNode, TypeNode);
};

class PackedFuncType : public Type {
public:
    // TVM_DLL PackedFuncType(Span span = Span());
    LITETVM_API PackedFuncType();

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(PackedFuncType, Type, PackedFuncTypeNode);
};

}// namespace relax
}// namespace litetvm

#endif//LITETVM_RELAX_TYPE_H
