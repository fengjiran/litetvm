//
// Created by 赵丹 on 25-2-26.
//

#ifndef LITETVM_IR_EXPR_H
#define LITETVM_IR_EXPR_H

#include "node/script_printer.h"
#include "runtime/object.h"
#include "runtime/string.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>

namespace litetvm {

using runtime::DataType;
using runtime::Object;
using runtime::ObjectRef;
using runtime::String;

/*!
 * \brief Base type of all the expressions.
 * \sa Expr
 */
class BaseExprNode : public Object {
public:
    /*!
   * \brief Span that points to the original source code.
   *        Reserved debug information.
   */
    // mutable Span span;

    static constexpr const char* _type_key = "BaseExpr";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    static constexpr uint32_t _type_child_slots = 62;
    TVM_DECLARE_BASE_OBJECT_INFO(BaseExprNode, Object);
};

/*!
 * \brief Managed reference to BaseExprNode.
 * \sa BaseExprNode
 */
class BaseExpr : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(BaseExpr, ObjectRef, BaseExprNode);
};

/*!
 * \brief Base node of all primitive expressions.
 *
 *  A primitive expression deals with low-level
 *  POD data types and handles without
 *  doing life-cycle management for objects.
 *
 *  PrimExpr is used in the low-level code
 *  optimizations and integer analysis.
 *
 * \sa PrimExpr
 */
class PrimExprNode : public BaseExprNode {
public:
    /*!
   * \brief The runtime data type of the primitive expression.
   *
   * runtime::DataType(dtype) provides coarse grained type information
   * during compile time and runtime. It is eagerly built in
   * PrimExpr expression construction and can be used for
   * quick type checking.
   *
   * dtype is sufficient to decide the Type of the PrimExpr
   * when it corresponds to POD value types such as i32.
   *
   * When dtype is DataType::Handle(), the expression could correspond to
   * a more fine-grained Type, and we can get the type by running lazy type inference.
   */
    DataType dtype;

    TVM_OBJECT_ENABLE_SCRIPT_PRINTER();

    static constexpr const char* _type_key = "PrimExpr";
    static constexpr uint32_t _type_child_slots = 38;
    TVM_DECLARE_BASE_OBJECT_INFO(PrimExprNode, BaseExprNode);
};

/*!
 * \brief Reference to PrimExprNode.
 * \sa PrimExprNode
 */
class PrimExpr : public BaseExpr {
public:
    /*!
   * \brief construct from integer.
   * \param value The value to be constructed.
   */
    LITETVM_API PrimExpr(int32_t value);// NOLINT(*)

    /*!
   * \brief construct from float.
   * \param value The value to be constructed.
   */
    LITETVM_API PrimExpr(float value);// NOLINT(*)

    /*! \return the data type of this expression. */
    DataType dtype() const { return get()->dtype; }

    TVM_DEFINE_OBJECT_REF_METHODS(PrimExpr, BaseExpr, PrimExprNode);

private:
    // Internal function for conversion.
    friend struct runtime::PackedFuncValueConverter<PrimExpr>;
    LITETVM_API static PrimExpr FromObject_(ObjectRef ref);
};

/*!
 * \brief add operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator+(PrimExpr a, PrimExpr b);

/*!
 * \brief subtraction operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator-(PrimExpr a, PrimExpr b);

/*!
 * \brief negation.
 *
 * \param a input.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator-(PrimExpr a);

/*!
 * \brief multiplication operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator*(PrimExpr a, PrimExpr b);

/*!
 * \brief division operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator/(PrimExpr a, PrimExpr b);

/*!
 * \brief left shift operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator<<(PrimExpr a, PrimExpr b);

/*!
 * \brief right shift operator
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator>>(PrimExpr a, PrimExpr b);

/*!
 * \brief greater
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator>(PrimExpr a, PrimExpr b);

/*!
 * \brief greater_equal
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator>=(PrimExpr a, PrimExpr b);

/*!
 * \brief less
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator<(PrimExpr a, PrimExpr b);

/*!
 * \brief less_equal
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator<=(PrimExpr a, PrimExpr b);

/*!
 * \brief equal
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator==(PrimExpr a, PrimExpr b);

/*!
 * \brief not_equal
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator!=(PrimExpr a, PrimExpr b);

/*!
 * \brief and
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note This operator does eager constant folding.
 */
LITETVM_API PrimExpr operator&&(PrimExpr a, PrimExpr b);

/*!
 * \brief or
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note This operator does eager constant folding.
 */
LITETVM_API PrimExpr operator||(PrimExpr a, PrimExpr b);

/*!
 * \brief not
 *
 * \param a left operand
 * \return The result expression.
 * \note This operator does eager constant folding.
 */
LITETVM_API PrimExpr operator!(PrimExpr a);

/*!
 * \brief take bitwise and of two values
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator&(PrimExpr a, PrimExpr b);

/*!
 * \brief take bitwise or of two values
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator|(PrimExpr a, PrimExpr b);

/*!
 * \brief take bitwise xor of two values
 *
 * \param a left operand
 * \param b right operand
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator^(PrimExpr a, PrimExpr b);

/*!
 * \brief take bitwise negation of two values
 *
 * \param a the input expression.
 * \return The result expression.
 * \note this function does eager constant folding for
 *       index types(int32, int64) when possible.
 */
LITETVM_API PrimExpr operator~(PrimExpr a);

// PrimExprs that are useful as runtime containers.
//
/*!
 * \brief Constant integer literals in the program.
 * \sa IntImm
 */
class IntImmNode : public PrimExprNode {
public:
    /*! \brief the Internal value. */
    int64_t value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("value", &value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const IntImmNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(value, other->value);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(value);
    }

    static constexpr const char* _type_key = "IntImm";
    TVM_DECLARE_FINAL_OBJECT_INFO(IntImmNode, PrimExprNode);
};

/*!
 * \brief Managed reference class to IntImmNode.
 *
 * \sa IntImmNode
 */
class IntImm : public PrimExpr {
public:
    /*!
   * \brief Constructor.
   * \param dtype The data type of the value.
   * \param value The internal value.
   * \param span The location of this object in the source code.
   */
    // LITETVM_API IntImm(DataType dtype, int64_t value, Span span = Span());
    LITETVM_API IntImm(DataType dtype, int64_t value);

    TVM_DEFINE_OBJECT_REF_METHODS(IntImm, PrimExpr, IntImmNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(IntImmNode);
};

/*!
 * \brief Constant floating point literals in the program.
 * \sa FloatImm
 */
class FloatImmNode : public PrimExprNode {
public:
    /*! \brief The constant value content. */
    double value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("value", &value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const FloatImmNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(value, other->value);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(value);
    }

    static constexpr const char* _type_key = "FloatImm";
    TVM_DECLARE_FINAL_OBJECT_INFO(FloatImmNode, PrimExprNode);
};

/*!
 * \brief Managed reference class to FloatImmNode.
 *
 * \sa FloatImmNode
 */
class FloatImm : public PrimExpr {
public:
    /*!
   * \brief Constructor.
   * \param dtype The data type of the value.
   * \param value The internal value.
   * \param span The location in the source code.
   */
    // LITETVM_API FloatImm(DataType dtype, double value, Span span = Span());
    LITETVM_API FloatImm(DataType dtype, double value);

    TVM_DEFINE_OBJECT_REF_METHODS(FloatImm, PrimExpr, FloatImmNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(FloatImmNode);
};

/*!
 * \brief Boolean constant.
 *
 *  This reference type is useful to add additional compile-time
 *  type checks and helper functions for Integer equal comparisons.
 */
class Bool : public IntImm {
public:
    // explicit Bool(bool value, Span span = Span()) : IntImm(DataType::Bool(), value, span) {}
    explicit Bool(bool value) : IntImm(DataType::Bool(), value) {}
    Bool operator!() const { return Bool((*this)->value == 0); }
    operator bool() const { return (*this)->value != 0; }

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(Bool, IntImm, IntImmNode);
};

// Overload operators to make sure we have the most fine-grained types.
inline Bool operator||(const Bool& a, bool b) {
    return Bool(a.operator bool() || b);
}
inline Bool operator||(bool a, const Bool& b) {
    return Bool(a || b.operator bool());
}
inline Bool operator||(const Bool& a, const Bool& b) {
    return Bool(a.operator bool() || b.operator bool());
}

inline Bool operator&&(const Bool& a, bool b) {
    return Bool(a.operator bool() && b);
}
inline Bool operator&&(bool a, const Bool& b) {
    return Bool(a && b.operator bool());
}
inline Bool operator&&(const Bool& a, const Bool& b) {
    return Bool(a.operator bool() && b.operator bool());
}

inline bool operator==(const Bool& a, bool b) {
    return a.operator bool() == b;
}
inline bool operator==(bool a, const Bool& b) {
    return a == b.operator bool();
}
inline bool operator==(const Bool& a, const Bool& b) {
    return a.operator bool() == b.operator bool();
}

/*!
 * \brief Container of constant int that adds more constructors.
 *
 * This is used to store and automate type check
 * attributes that must be constant integer.
 *
 * \sa IntImm
 */
class Integer : public IntImm {
public:
    Integer() {}
    /*!
   * \brief constructor from node.
   */
    explicit Integer(ObjectPtr<Object> node) : IntImm(node) {}
    /*!
   * \brief Construct integer from int value.
   */
    // Integer(int value, Span span = Span()) : IntImm(DataType::Int(32), value, span) {}  // NOLINT(*)
    Integer(int value) : IntImm(DataType::Int(32), value) {}

    /*!
   * \brief Construct integer from int imm.
   * \param other The other value.
   */
    Integer(IntImm other) : IntImm(std::move(other)) {}// NOLINT(*)

    /*!
   * \brief Constructor from enum
   * \tparam Enum The enum type.
   * \param value The enum value.
   */
    template<typename Enum,
             typename = std::enable_if_t<std::is_enum_v<Enum>>>
    explicit Integer(Enum value) : Integer(static_cast<int>(value)) {
        static_assert(std::is_same_v<int, std::underlying_type_t<Enum>>, "declare enum to be enum int to use visitor");
    }

    /*!
   * \brief Assign an expression to integer.
   * \param other another expression.
   */
    Integer& operator=(const IntImm& other) {
        data_ = GetDataPtr<Object>(other);
        return *this;
    }

    /*!
   * \brief convert to int64_t
   */
    int64_t IntValue() const {
        CHECK(data_ != nullptr) << " Trying to reference a null Integer";
        return (*this)->value;
    }

    // comparators
    Bool operator==(int other) const {
        if (data_ == nullptr) return Bool(false);
        return Bool((*this)->value == other);
    }

    Bool operator!=(int other) const {
        return !(*this == other);
    }

    template<typename Enum,
             typename = std::enable_if_t<std::is_enum_v<Enum>>>
    Bool operator==(Enum other) const {
        return *this == static_cast<int>(other);
    }

    template<typename Enum,
             typename = std::enable_if_t<std::is_enum_v<Enum>>>
    Bool operator!=(Enum other) const {
        return *this != static_cast<int>(other);
    }
};

}// namespace litetvm

#endif//LITETVM_IR_EXPR_H
