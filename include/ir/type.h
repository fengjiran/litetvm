//
// Created by 赵丹 on 25-3-13.
//

#ifndef LITETVM_IR_TYPE_H
#define LITETVM_IR_TYPE_H

// #include "ir/expr.h"
#include "runtime/array.h"
#include "runtime/data_type.h"
#include "runtime/object.h"
#include "node/reflection.h"

#include <string>

namespace litetvm {

using runtime::Object;
using runtime::ObjectRef;

/*!
 * \brief Type is the base type of all types.
 *
 * Relay's type system contains the following subclasses:
 *
 * - PrimType: type of primitive type values used in the low-level IR.
 * - FuncType: type of function.
 * - TensorType: type of certain Tensor values in the expression.
 *
 * There are also advanced types to support generic(polymorphic types).
 * \sa Type
 */
class TypeNode : public Object {
public:
    /*!
   * \brief Span that points to the original source code.
   *        Reserved debug information.
   */
    // mutable Span span;

    static constexpr const char* _type_key = "Type";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    static constexpr uint32_t _type_child_slots = 14;
    TVM_DECLARE_BASE_OBJECT_INFO(TypeNode, Object);
};

/*!
 * \brief Managed reference to TypeNode.
 * \sa TypeNode
 */
class Type : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(Type, ObjectRef, TypeNode);
};

/*!
 * \brief Primitive data types used in the low-level IR.
 *
 * PrimType represents POD-values and handles that are
 * not automatically managed by the runtime.
 *
 * \sa PrimType
 */
class PrimTypeNode : public TypeNode {
public:
    /*!
   * \brief The corresponding dtype field.
   */
    runtime::DataType dtype;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
    }

    bool SEqualReduce(const PrimTypeNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
    }

    static constexpr const char* _type_key = "PrimType";
    TVM_DECLARE_FINAL_OBJECT_INFO(PrimTypeNode, TypeNode);
};

/*
 * \brief Managed reference to PrimTypeNode.
 * \sa PrimTypeNode
 */
class PrimType : public Type {
public:
    /*!
   * \brief Constructor
   * \param dtype The corresponding dtype.
   * \param span The span
   */
    // TVM_DLL explicit PrimType(runtime::DataType dtype, Span span = Span());
    LITETVM_API explicit PrimType(runtime::DataType dtype);

    TVM_DEFINE_OBJECT_REF_METHODS(PrimType, Type, PrimTypeNode);
};

/*!
 * \brief Low-level raw pointer type.
 *
 *  PointerType represents type hints in the TIR to be
 *  passed to the final code generator.
 *
 *  PointerType should not occur in the high-level analysis.
 *
 * \sa PointerType
 */
class PointerTypeNode : public TypeNode {
public:
    /*!
     * \brief The type of the element which the pointer points to.
     */
    Type element_type;
    /*!
     * \brief The storage scope of the pointer
     */
    String storage_scope;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("element_type", &element_type);
        v->Visit("storage_scope", &storage_scope);
    }

    bool SEqualReduce(const PointerTypeNode* other, SEqualReducer equal) const {
        // Make "global" equal to ""
        String lhs_scope = storage_scope.empty() ? "global" : storage_scope;
        String rhs_scope = other->storage_scope.empty() ? "global" : other->storage_scope;
        return equal(element_type, other->element_type) && equal(lhs_scope, rhs_scope);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(element_type);
        // Make "global" equal to ""
        hash_reduce(storage_scope.empty() ? "global" : storage_scope);
    }

    static constexpr const char* _type_key = "PointerType";
    TVM_DECLARE_FINAL_OBJECT_INFO(PointerTypeNode, TypeNode);
};

/*
 * \brief Managed reference to PointerTypeNode.
 * \sa PointerTypeNode
 */
class PointerType : public Type {
public:
    /*!
     * \brief Constructor
     * \param element_type The type of the element which the pointer points to.
     * \param storage_scope The storage scope into which the pointer addresses
     */
    LITETVM_API explicit PointerType(Type element_type, String storage_scope = "");

    TVM_DEFINE_OBJECT_REF_METHODS(PointerType, Type, PointerTypeNode);
};

/*! \brief Possible kinds of TypeVars. */
enum class TypeKind : int {
    kType = 0,
    /*! \brief Template variable in shape expression. */
    kShapeVar = 1,
    kBaseType = 2,
    kConstraint = 4,
    kAdtHandle = 5,
    kTypeData = 6
};

/*! \brief Converts a TypeKind to a string. */
inline String TypeKind2String(TypeKind kind) {
    switch (kind) {
        case TypeKind::kType:
            return "Type";
        case TypeKind::kShapeVar:
            return "ShapeVar";
        case TypeKind::kBaseType:
            return "BaseType";
        case TypeKind::kConstraint:
            return "Constraint";
        case TypeKind::kAdtHandle:
            return "AdtHandle";
        case TypeKind::kTypeData:
            return "TypeData";
    }
    LOG(FATAL) << "ValueError: Unknown TypeKind: " << static_cast<int>(kind);
}

/*!
 * \brief Type parameter in functions.
 *
 * A type variable can be viewed as template parameter in c++ template function.
 *
 * For example, in the following pseudocode,
 * the TypeVar of f is TypeVar("n", kind=kShapeVar).
 * This function can take in a Tensor with shape=(3, 3) and
 * return a Tensor with shape=(9,)
 *
 * \code
 *
 *  template<i32 n>
 *  f(x : Tensor[i32, (n, n)]) -> Tensor[i32, (n * n)]
 *
 * \endcode
 * \sa TypeVar, TypeKind
 */
class TypeVarNode : public TypeNode {
public:
    /*!
     * \brief The name of the variable,
     *  this only acts as a hint to the user,
     *  and is not used for equality.
     */
    String name_hint;
    /*! \brief The kind of type parameter */
    TypeKind kind;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("name_hint", &name_hint);
        v->Visit("kind", &kind);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const TypeVarNode* other, SEqualReducer equal) const {
        return equal(kind, other->kind) && equal.FreeVarEqualImpl(this, other);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(kind);
        hash_reduce.FreeVarHashImpl(this);
    }

    static constexpr const char* _type_key = "TypeVar";
    TVM_DECLARE_FINAL_OBJECT_INFO(TypeVarNode, TypeNode);
};

/*!
 * \brief Managed reference to TypeVarNode
 * \sa TypeVarNode
 */
class TypeVar : public Type {
public:
    /*!
     * \brief Constructor
     * \param name_hint The name of the type var.
     * \param kind The kind of the type var.
     * \param span The span information.
     */
    // TVM_DLL TypeVar(String name_hint, TypeKind kind, Span span = Span());
    LITETVM_API TypeVar(String name_hint, TypeKind kind);

    TVM_DEFINE_OBJECT_REF_METHODS(TypeVar, Type, TypeVarNode);
};

/*!
 * \brief A global type variable that is used for defining new types or type aliases.
 * \sa GlobalTypeVar
 */
class GlobalTypeVarNode : public TypeNode {
public:
    /*!
     * \brief The name of the variable,
     *  this only acts as a hint to the user,
     *  and is not used for equality.
     */
    String name_hint;
    /*! \brief The kind of type parameter */
    TypeKind kind;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("name_hint", &name_hint);
        v->Visit("kind", &kind);
    }

    bool SEqualReduce(const GlobalTypeVarNode* other, SEqualReducer equal) const {
        // name matters for now in global type var.
        return equal(name_hint, other->name_hint) && equal.FreeVarEqualImpl(this, other);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(name_hint);
        hash_reduce.FreeVarHashImpl(this);
    }

    static constexpr const char* _type_key = "GlobalTypeVar";
    TVM_DECLARE_FINAL_OBJECT_INFO(GlobalTypeVarNode, TypeNode);
};

/*!
 * \brief Managed reference to GlobalTypeVarNode
 * \sa GlobalTypeVarNode
 */
class GlobalTypeVar : public Type {
public:
    /*!
     * \brief Constructor
     * \param name_hint The name of the type var.
     * \param kind The kind of the type var.
     * \param span The span of the type.
     */
    // TVM_DLL GlobalTypeVar(String name_hint, TypeKind kind, Span span = Span());
    LITETVM_API GlobalTypeVar(String name_hint, TypeKind kind);

    TVM_DEFINE_OBJECT_REF_METHODS(GlobalTypeVar, Type, GlobalTypeVarNode);
};

/*!
 * \brief The type of tuple values.
 * \sa TupleType
 */
class TupleTypeNode : public TypeNode {
public:
    /*! \brief The type of each field in the tuple. */
    Array<Type> fields;

    TupleTypeNode() = default;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("fields", &fields);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const TupleTypeNode* other, SEqualReducer equal) const {
        return equal(fields, other->fields);
    }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce(fields); }

    static constexpr const char* _type_key = "TupleType";
    TVM_DECLARE_FINAL_OBJECT_INFO(TupleTypeNode, TypeNode);
};

/*!
 * \brief Managed reference to TupleTypeNode.
 * \sa TupleTypeNode.
 */
class TupleType : public Type {
public:
    /*!
     * \brief Constructor
     * \param fields Fields in the tuple.
     * \param span The span of the type.
     */
    LITETVM_API explicit TupleType(Array<Type> fields);

    /*!
     * \brief Create an empty tuple type that constains nothing.
     * \return AN empty tuple type.
     */
    LITETVM_API TupleType static Empty();

    TVM_DEFINE_OBJECT_REF_METHODS(TupleType, Type, TupleTypeNode);
};

/*!
 * \return a type that represents void.
 */
inline Type VoidType() {
    return TupleType::Empty();
}

/*!
 * \brief Check whether the type represents void.
 * \return The check result.
 */
inline bool IsVoidType(const Type& type) {
    auto* n = type.as<TupleTypeNode>();
    return n && n->fields.empty();
}

/*!
 * \brief Potential Constraints in a function.
 * \sa TypeConstraint
 */
class TypeConstraintNode : public TypeNode {
public:
    static constexpr const char* _type_key = "TypeConstraint";
    static constexpr uint32_t _type_child_slots = 1;
    TVM_DECLARE_BASE_OBJECT_INFO(TypeConstraintNode, TypeNode);
};

/*!
 * \brief Managed reference to TypeConstraintNode.
 * \sa TypeConstraintNode, TypeRelation
 */
class TypeConstraint : public Type {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(TypeConstraint, Type, TypeConstraintNode);
};

/*!
 * \brief Function type.
 *
 * We support polymorphic function type.
 * This can be roughly viewed as template function in C++.
 *
 * \sa FuncType, TypeVar, TypeConstraint
 */
class FuncTypeNode : public TypeNode {
public:
    /*! \brief type type of arguments */
    Array<Type> arg_types;
    /*! \brief The type of return value. */
    Type ret_type;
    // The following fields are used in polymorphic(template) functions
    // For normal functions, the following two fields will be empty.
    /*! \brief The type parameters of the function */
    // Array<TypeVar> type_params;
    /*!
     * \brief potential constraint the type need to obey
     * \note this field is reserved for further purposes.
     */
    // Array<TypeConstraint> type_constraints;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("arg_types", &arg_types);
        v->Visit("ret_type", &ret_type);
        // v->Visit("type_params", &type_params);
        // v->Visit("type_constraints", &type_constraints);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const FuncTypeNode* other, SEqualReducer equal) const {
        // type params first as they define type vars.
        return equal(arg_types, other->arg_types) && equal(ret_type, other->ret_type);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        // hash_reduce.DefHash(type_params);
        hash_reduce(arg_types);
        hash_reduce(ret_type);
        // hash_reduce(type_constraints);
    }

    static constexpr const char* _type_key = "FuncType";
    TVM_DECLARE_FINAL_OBJECT_INFO(FuncTypeNode, TypeNode);
};

/*!
 * \brief Managed reference to FuncTypeNode.
 * \sa FuncTypeNode
 */
class FuncType : public Type {
public:
    /*!
     * \brief Constructor
     * \param arg_types The types of the arguments.
     * \param ret_type The type of the return value.
     * \param type_params The type parameters.
     * \param type_constraints The type constraints.
     * \param span The span information.
     * \sa FuncTypeNode for more docs about these fields.
     */
    LITETVM_API FuncType(Array<Type> arg_types, Type ret_type);

    TVM_DEFINE_OBJECT_REF_METHODS(FuncType, Type, FuncTypeNode);
};


/*!
 * \brief Intermediate values that is used to indicate incomplete type
 *         during type inference.
 *
 * If we view the type relations as "computational graph of types",
 * then IncompleteType represents intermediate values of the graph,
 * TypeVar represents the input to the graph.
 *
 * \sa IncompleteType
 */
class IncompleteTypeNode : public TypeNode {
public:
    /*! \brief kind of the type. */
    TypeKind kind;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("kind", &kind);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const IncompleteTypeNode* other, SEqualReducer equal) const {
        return equal(kind, other->kind) && equal.FreeVarEqualImpl(this, other);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(kind);
    }

    static constexpr const char* _type_key = "IncompleteType";
    TVM_DECLARE_FINAL_OBJECT_INFO(IncompleteTypeNode, TypeNode);
};

/*!
 * \brief Managed reference to IncompleteTypeNode.
 * \sa IncompleteTypeNode
 */
class IncompleteType : public Type {
public:
    /*!
     * \brief Constructor.
     * \param kind kind of the type.
     * \param span The span information.
     */
    LITETVM_API explicit IncompleteType(TypeKind kind);

    TVM_DEFINE_OBJECT_REF_METHODS(IncompleteType, Type, IncompleteTypeNode);
};

/*!
 * \brief Reference Type High-level Relay IR.
 *
 * \sa RelayRefType.
 */
class RelayRefTypeNode : public TypeNode {
public:
    /*! \brief The type of value in the Reference. */
    Type value;

    RelayRefTypeNode() = default;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("value", &value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const RelayRefTypeNode* other, SEqualReducer equal) const {
        return equal(value, other->value);
    }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce(value); }

    // Keep the relay prefix in the type as this type is specific
    // to the relay itself.
    static constexpr const char* _type_key = "relay.RefType";
    TVM_DECLARE_FINAL_OBJECT_INFO(RelayRefTypeNode, TypeNode);
};

/*!
 * \brief Managed reference to RelayRefTypeNode.
 * \sa RelayRefTypeNode.
 */
class RelayRefType : public Type {
public:
    LITETVM_API explicit RelayRefType(Type value);
    TVM_DEFINE_OBJECT_REF_METHODS(RelayRefType, Type, RelayRefTypeNode);
};


}// namespace litetvm

#endif//LITETVM_IR_TYPE_H
