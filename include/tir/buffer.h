//
// Created by 赵丹 on 25-4-3.
//

#ifndef LITETVM_TIR_BUFFER_H
#define LITETVM_TIR_BUFFER_H

#include "ir/expr.h"
#include "node/script_printer.h"
#include "runtime/array.h"
#include "runtime/string.h"
#include "tir/var.h"

#include <string>

namespace litetvm {
namespace tir {

class Stmt;

#ifndef TVM_INDEX_DEFAULT_I64
#define TVM_INDEX_DEFAULT_I64 1
#endif
/*! \brief if TVM_INDEX_DEFAULT_I64 is set, return int64, otherwise return int32 */
inline DataType DefaultIndexType() {
#if TVM_INDEX_DEFAULT_I64
    return DataType::Int(64);
#else
    return DataType::Int(32);
#endif
}

/*! \brief buffer type */
enum class BufferType : int {
    kDefault = 1,
    // Maps buffer[i][j][k] -> buffer[i][0][k] if dimension i's shape equals 1.
    kAutoBroadcast = 2,
};

/*! \brief Node to represent a buffer */
class BufferNode : public Object {
public:
    // Data fields.
    /*!
   * \brief The pointer to the head of the data
   * \sa data_alignment The alignment of data in bytes.
   */
    Var data;

    /*! \brief data type in the content of the tensor */
    DataType dtype;

    /*! \brief The type of the buffer prior to flattening
   *
   * This contains the shape as it is accessed by
   * BufferLoad/BufferStore nodes, and used by the low-level code
   * generators.
   */
    Array<PrimExpr> shape;

    /*!
   * \brief Separators between input axes when generating flattened output axes
   *
   * For buffers representing flat 1-d memory (e.g. any buffer in
   * RAM), this should be an empty array.  For buffers representing
   * non-flat memory, each entry in axis_separators should be the
   * first input axis that is part of a new flattened axis.
   */
    Array<IntImm> axis_separators;

    /*!
   * \brief The strides of each dimension
   *  This can be an empty array, indicating array is contiguous
   */
    Array<PrimExpr> strides;

    /*! \brief The offset in terms of number of dtype elements (including lanes) */
    PrimExpr elem_offset;

    // Meta data
    /*! \brief optional name of the buffer */
    String name;

    /*! \brief Alignment requirement of data pointer in bytes. */
    int data_alignment;

    /*!
   * \brief Factor of elem_offset field,
   *  elem_offset is guaranteed to be multiple of offset_factor.
   */
    int offset_factor;

    /*! \brief buffer type */
    BufferType buffer_type;
    /*!
   * \brief Span that points to the original source code.
   *        Reserved debug information.
   */
    // mutable Span span;
    /*! \brief constructor */
    BufferNode() = default;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("data", &data);
        v->Visit("dtype", &dtype);
        v->Visit("shape", &shape);
        v->Visit("strides", &strides);
        v->Visit("axis_separators", &axis_separators);
        v->Visit("elem_offset", &elem_offset);
        v->Visit("name", &name);
        v->Visit("data_alignment", &data_alignment);
        v->Visit("offset_factor", &offset_factor);
        v->Visit("buffer_type", &buffer_type);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const BufferNode* other, SEqualReducer equal) const {
        // Use DefEqual as buffer can define variables in its semantics,
        // skip name as name is not important.
        return equal.DefEqual(data, other->data) && equal(dtype, other->dtype) &&
               equal.DefEqual(shape, other->shape) && equal.DefEqual(strides, other->strides) &&
               equal.DefEqual(axis_separators, other->axis_separators) &&
               equal.DefEqual(elem_offset, other->elem_offset) &&
               equal(data_alignment, other->data_alignment) && equal(buffer_type, other->buffer_type);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce.DefHash(data);
        hash_reduce(dtype);
        hash_reduce.DefHash(shape);
        hash_reduce.DefHash(strides);
        hash_reduce.DefHash(elem_offset);
        hash_reduce.DefHash(axis_separators);
        hash_reduce(data_alignment);
        hash_reduce(buffer_type);
    }

    /*! \return preferred index type for this buffer node */
    DataType DefaultIndexType() const {
        return !shape.empty() ? shape[0].dtype() : tir::DefaultIndexType();
    }

    /*! \brief Determine the offset in the buffer of the given index.
   *
   * Returns the buffer offset, in number of elements of type dtype,
   * without adjusting for number of lanes.  (e.g. The number of
   * float16x4 elements in a buffer of type float16x4.)
   */
    Array<PrimExpr> ElemOffset(Array<PrimExpr> index) const;

    static constexpr const char* _type_key = "tir.Buffer";
    static constexpr const bool _type_has_method_sequal_reduce = true;
    static constexpr const bool _type_has_method_shash_reduce = true;
    TVM_DECLARE_FINAL_OBJECT_INFO(BufferNode, Object);
    TVM_OBJECT_ENABLE_SCRIPT_PRINTER();
};


}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_BUFFER_H
