//
// Created by 赵丹 on 25-3-13.
//

#ifndef LITETVM_TIR_VAR_H
#define LITETVM_TIR_VAR_H

#include "ir/expr.h"
#include "ir/type.h"
#include "runtime/data_type.h"

#include <string>

namespace litetvm {
namespace tir {

/*!
 * \brief A variable node in the IR.
 *
 * A variable is uniquely identified by its address.
 *
 * Each variable is only bound once in the following nodes:
 * - Allocate
 * - For
 * - Let
 * - LetStmt
 */
class VarNode : public PrimExprNode {
public:
    /*!
   * \brief The hint to the variable name.
   * \note Each variable is uniquely identified by its address.
   */
    String name_hint;
    /*!
   * \brief type annotation of the variable.
   *
   * It is an optional field that provides a refined type of the variable than dtype.
   *
   * \sa tvm/ir/type.h for discussion of relations between runtime::DataType and Type.
   */
    Type type_annotation;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("name", &name_hint);
        v->Visit("type_annotation", &type_annotation);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const VarNode* other, SEqualReducer equal) const {
        if (!equal(dtype, other->dtype)) return false;
        if (!equal(type_annotation, other->type_annotation)) return false;
        return equal.FreeVarEqualImpl(this, other);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(type_annotation);
        hash_reduce.FreeVarHashImpl(this);
    }

    static constexpr const char* _type_key = "tir.Var";
    static constexpr uint32_t _type_child_slots = 1;
    TVM_DECLARE_BASE_OBJECT_INFO(VarNode, PrimExprNode);
};

/*! \brief a named variable in TIR */
class Var : public PrimExpr {
public:
    explicit Var(ObjectPtr<Object> n) : PrimExpr(n) {}
    /*!
     * \brief Constructor
     * \param name_hint variable name
     * \param dtype data type
     * \param span The location of this object in the source code.
     */
    // LITETVM_API explicit Var(String name_hint = "v", DataType dtype = DataType::Int(32),
    //                      Span span = Span());
    LITETVM_API explicit Var(String name_hint = "v", DataType dtype = DataType::Int(32));

    /*!
     * \brief Constructor which provides a more detailed type annotation.
     * \param name_hint variable name.
     * \param type_annotation The type annotation.
     * \param span The location of this object in the source code.
     */
    LITETVM_API explicit Var(String name_hint, Type type_annotation);
    // LITETVM_API explicit Var(String name_hint, Type type_annotation, Span span = Span());

    /*!
     * \brief Make a new copy of var with same type, but a different nam
     * \param name The new name to be used.
     * \return the new Var copy
     */
    LITETVM_API Var copy_with_name(const String& name) const;
    /*!
     * \brief Make a new copy of var with same type, append suffix
     * \param suffix The suffix to be appended.
     * \return the new Var copy
     */
    LITETVM_API Var copy_with_suffix(const String& suffix) const;
    /*!
     * \brief Make a new copy of the variable with specified dtype
     * \param dtype The specified dtype
     * \return The new variable
     */
    LITETVM_API Var copy_with_dtype(DataType dtype) const;

    /*!
     * \brief Get pointer to the internal value.
     * \return the corresponding Variable.
     */
    const VarNode* operator->() const {
        return get();
    }
    /*!
     * \brief Get pointer to the internal value.
     * \return the corresponding Variable.
     */
    const VarNode* get() const {
        return static_cast<const VarNode*>(data_.get());
    }
    /*! \brief type indicate the container type */
    using ContainerType = VarNode;
};

/*!
 * \brief A variable node represent a tensor index size,
 * whose value must be non-negative.
 */
class SizeVarNode : public VarNode {
public:
    static constexpr const char* _type_key = "tir.SizeVar";
    TVM_DECLARE_FINAL_OBJECT_INFO(SizeVarNode, VarNode);
};

/*! \brief a named variable represents a tensor index size */
class SizeVar : public Var {
public:
    explicit SizeVar(ObjectPtr<Object> n) : Var(n) {}
    /*!
     * \brief constructor
     * \param name_hint variable name
     * \param t data type
     * \param span The location of this object in the source code.
     */
    // TVM_DLL explicit SizeVar(String name_hint = "s", DataType t = DataType::Int(32),
    //                          Span span = Span());
    LITETVM_API explicit SizeVar(String name_hint = "s", DataType t = DataType::Int(32));

    /*!
     * \brief Constructor which provides a more detailed type annotation.
     * \param name_hint variable name.
     * \param type_annotation The type annotation.
     * \param span The location of this object in the source code.
     */
    // TVM_DLL explicit SizeVar(String name_hint, Type type_annotation, Span span = Span());
    LITETVM_API explicit SizeVar(String name_hint, Type type_annotation);

    /*!
     * \brief Get pointer to the internal value.
     * \return the corresponding Variable.
     */
    const SizeVarNode* operator->() const {
        return get();
    }

    /*!
     * \brief Get pointer to the internal value.
     * \return the corresponding Variable.
     */
    const SizeVarNode* get() const {
        return static_cast<const SizeVarNode*>(data_.get());
    }

    /*! \brief type indicate the container type */
    using ContainerType = SizeVarNode;
};

}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_VAR_H
