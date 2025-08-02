//
// Created by 赵丹 on 25-2-26.
//

#ifndef LITETVM_IR_EXPR_H
#define LITETVM_IR_EXPR_H

#include "ir/type.h"
#include "node/repr_printer.h"
#include "node/script_printer.h"
#include "runtime/object.h"
#include "runtime/string.h"
#include "runtime/registry.h"

// #include <algorithm>
// #include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

namespace litetvm {

using runtime::DataType;
using runtime::make_object;
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
   * Dtype is sufficient to decide the Type of the PrimExpr
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
    LITETVM_API PrimExpr(int32_t value);// NOLINT

    /*!
   * \brief construct from float.
   * \param value The value to be constructed.
   */
    LITETVM_API PrimExpr(float value);// NOLINT

    /*! \return the data type of this expression. */
    NODISCARD DataType dtype() const {
        return get()->dtype;
    }

    TVM_DEFINE_OBJECT_REF_METHODS(PrimExpr, BaseExpr, PrimExprNode);

private:
    // Internal function for conversion.
    friend struct runtime::PackedFuncValueConverter<PrimExpr>;
    LITETVM_API static PrimExpr FromObject_(ObjectRef ref);
};

/*!
 * \brief Base node of all non-primitive expressions.
 *
 * RelaxExpr supports tensor and functions as first class citizen.
 * The life-cycle of the corresponding
 * objects are implicitly managed by the language.
 *
 * \sa RelaxExpr
 */
class RelaxExprNode : public BaseExprNode {
public:
    /*!
     * \brief Stores the result of type inference(type checking).
     *
     * \note This can be undefined before type inference.
     *       This value is discarded during serialization.
     */
    mutable Type checked_type_ = Type(nullptr);

    /*!
     * \brief Stores the result of structure information of the
     *        expression that encapsulate both static shape and
     *        runtime information such as shape.
     */
    mutable Optional<ObjectRef> struct_info_ = Optional<ObjectRef>();

    /*!
     * \return The checked_type
     */
    inline const Type& checked_type() const;

    /*!
     * \brief Check if the inferred(checked) type of the Expr
     *  is backed by a TTypeNode and return it.
     *
     * \note This function will throw an error if the node type
     *       of this Expr is not TTypeNode.
     *
     * \return The corresponding TTypeNode pointer.
     * \tparam The specific TypeNode we look for.
     */
    template<typename TTypeNode>
    const TTypeNode* type_as() const;

    static constexpr const char* _type_key = "RelaxExpr";
    static constexpr uint32_t _type_child_slots = 22;
    TVM_DECLARE_BASE_OBJECT_INFO(RelaxExprNode, BaseExprNode);
};

/*!
 * \brief Managed reference to RelaxExprNode.
 * \sa RelaxExprNode
 */
class RelaxExpr : public BaseExpr {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(RelaxExpr, BaseExpr, RelaxExprNode);
};

class GlobalVar;
/*!
 * \brief Global variable that lives in the top-level module.
 *
 * A GlobalVar only refers to function definitions.
 * This is used to enable recursive calls between functions.
 *
 * \sa GlobalVarNode
 */
class GlobalVarNode : public RelaxExprNode {
public:
    /*! \brief The name of the variable, this only acts as a hint. */
    String name_hint;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("name_hint", &name_hint);
        v->Visit("_checked_type_", &checked_type_);
        v->Visit("struct_info_", &struct_info_);
    }

    bool SEqualReduce(const GlobalVarNode* other, const SEqualReducer& equal) const {
        // name matters for global var.
        return equal(name_hint, other->name_hint) && equal.FreeVarEqualImpl(this, other);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(name_hint);
        hash_reduce.FreeVarHashImpl(this);
    }

    static constexpr const char* _type_key = "GlobalVar";
    TVM_DECLARE_FINAL_OBJECT_INFO(GlobalVarNode, RelaxExprNode);
};

/*!
 * \brief Managed reference to GlobalVarNode.
 * \sa GlobalVarNode
 */
class GlobalVar : public RelaxExpr {
public:
    // TVM_DLL explicit GlobalVar(String name_hint, Type type = {}, Span span = {});
    LITETVM_API explicit GlobalVar(String name_hint, Type type = {});

    TVM_DEFINE_OBJECT_REF_METHODS(GlobalVar, RelaxExpr, GlobalVarNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(GlobalVarNode);
};


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
    }

    bool SEqualReduce(const IntImmNode* other, const SEqualReducer& equal) const {
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

    bool SEqualReduce(const FloatImmNode* other, const SEqualReducer& equal) const {
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
    Integer() = default;
    /*!
   * \brief constructor from node.
   */
    explicit Integer(ObjectPtr<Object> node) : IntImm(std::move(node)) {}
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
        if (data_ == nullptr)
            return Bool(false);
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

/*! \brief range over one dimension */
class RangeNode : public Object {
public:
    /*! \brief beginning of the node */
    PrimExpr min;
    /*! \brief the extent of range */
    PrimExpr extent;
    /*! \brief the location of this range in the source */
    // mutable Span span;
    /*! \brief constructor */
    RangeNode() = default;

    // RangeNode(PrimExpr min, PrimExpr extent, Span span = Span())
    //     : min(min), extent(extent), span(span) {}
    RangeNode(PrimExpr min, PrimExpr extent) : min(std::move(min)), extent(std::move(extent)) {}

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("min", &min);
        v->Visit("extent", &extent);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const RangeNode* other, SEqualReducer equal) const {
        return equal(min, other->min) && equal(extent, other->extent);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(min);
        hash_reduce(extent);
    }

    static constexpr const char* _type_key = "Range";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    TVM_DECLARE_FINAL_OBJECT_INFO(RangeNode, Object);
};

/*! \brief Range container  */
class Range : public ObjectRef {
public:
    /*!
     * \brief constructor by begin and end
     * \param begin The begin of the range.
     * \param end The end of the range.
     * \param span The location of the Range in the source.
     */
    // TVM_DLL Range(PrimExpr begin, PrimExpr end, Span span = Span());
    LITETVM_API Range(PrimExpr begin, PrimExpr end);

    /*!
     * \brief construct a new range with min and extent
     *  The corresponding constructor is removed,
     *  because that is counter convention of tradition meaning
     *  of range(begin, end)
     *
     * \param min The minimum range.
     * \param extent The extent of the range.
     * \param span The location of the Range in the source.
     */
    // static Range FromMinExtent(PrimExpr min, PrimExpr extent, Span span = Span());
    static Range FromMinExtent(PrimExpr min, PrimExpr extent);

    // declare range.
    TVM_DEFINE_OBJECT_REF_METHODS(Range, ObjectRef, RangeNode);
};

// implementations
inline const Type& RelaxExprNode::checked_type() const {
    CHECK(checked_type_.defined()) << "internal error: the type checker has "
                                   << "not populated the checked_type "
                                   << "field for " << GetRef<RelaxExpr>(this);
    return this->checked_type_;
}

template<typename TTypeNode>
const TTypeNode* RelaxExprNode::type_as() const {
    static_assert(std::is_base_of_v<TypeNode, TTypeNode>, "TType must be a special case of type");
    CHECK(checked_type_.defined()) << "Type inference for this Expr has not completed. Try to call infer_type pass.";
    const TTypeNode* node = checked_type_.as<TTypeNode>();
    CHECK(node != nullptr) << "Expected type to be " << TTypeNode::_type_key << ", but get " << checked_type_->GetTypeKey();
    return node;
}

}// namespace litetvm

namespace litetvm {
namespace runtime {

// Automatic conversion into IntImm, Integer, and Bool, when called
// through the FFI.  Automatic conversions into PrimExpr are
// registered in "tvm/tir/expr.h", as it includes conversions to the
// TIR-only StringImm.
//
// While the FFI only requires the From() method, these
// implementations also define a TryFrom() method to avoid duplicate
// logic in the PrimExpr conversion.
template<>
struct PackedFuncValueConverter<IntImm> {
    template<typename PODSubclass>
    static Optional<IntImm> TryFrom(const PODSubclass& val) {
        if (auto opt = val.TryAsInt()) {
            int64_t value = opt.value();
            auto dtype =
                    (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min())
                            ? DataType::Int(64)
                            : DataType::Int(32);
            return IntImm(dtype, value);
        }
        if (auto opt = val.TryAsBool()) {
            return IntImm(DataType::Int(32), opt.value());
        }
        return NullOpt;
    }

    template<typename PODSubclass>
    static IntImm From(const PODSubclass& val) {
        if (auto opt = TryFrom(val)) {
            return opt.value();
        }
        return val.template AsObjectRef<IntImm>();
    }
};

template<>
struct PackedFuncValueConverter<Integer> {
    template<typename PODSubclass>
    static Integer From(const PODSubclass& val) {
        if (auto opt = PackedFuncValueConverter<IntImm>::TryFrom(val)) {
            return Integer(opt.value());
        }
        return val.template AsObjectRef<Integer>();
    }
};

template<>
struct PackedFuncValueConverter<Bool> {
    template<typename PODSubclass>
    static Optional<Bool> TryFrom(const PODSubclass& val) {
        if (auto opt = val.TryAsBool()) {
            return Bool(opt.value());
        }
        if (auto opt = val.TryAsInt()) {
            int value = opt.value();
            CHECK(value == 0 || value == 1)
                    << "ValueError: boolean value can only be 0 or 1, but get " << value;
            return Bool(static_cast<bool>(value));
        }
        return NullOpt;
    }

    template<typename PODSubclass>
    static Bool From(const PODSubclass& val) {
        if (auto opt = TryFrom(val)) {
            return opt.value();
        }
        return val.template AsObjectRef<Bool>();
    }
};

template<>
struct PackedFuncValueConverter<FloatImm> {
    static Optional<FloatImm> TryFrom(const TVMPODValue_& val) {
        if (auto opt = val.TryAsFloat()) {
            return FloatImm(DataType::Float(32), opt.value());
        }
        return NullOpt;
    }

    template<typename PODSubclass>
    static FloatImm From(const PODSubclass& val) {
        if (auto opt = TryFrom(val)) {
            return opt.value();
        }
        return val.template AsObjectRef<FloatImm>();
    }
};

/* \brief Backwards compatibility wrapper for IntImm arguments
 *
 * In previous versions of TVM, IntImm was the default FFI type for
 * integer arguments, instead of runtime::Int.  For backwards
 * compatibility where the callee has been updated to expected a
 * runtime::Int, the caller has not been updated to provide a
 * runtime::Int, and the auto-unboxing of
 * runtime::Int does not apply (e.g. making an `Array<runtime::Int>`),
 * allow the IntImm to be generated.
 */
template<>
struct PackedFuncValueConverter<Int> {
    template<typename PODSubclass>
    static Int From(const PODSubclass& val) {
        if (val.template IsObjectRef<IntImm>()) {
            return Int(val.template AsObjectRef<IntImm>()->value);
        }
        return val.template AsObjectRef<Int>();
    }
};

}// namespace runtime
}// namespace litetvm

/* \brief Allow tvm.GLobalVar as key in STL tables
 *
 * For most IR expressions, it would be ambiguous whether the
 * expression should follow reference equality or structural equality.
 * This is not the case for variables, which do not contain nested
 * internal structure, and are frequently used as keys in lookup
 * tables.
 *
 * Providing `std::hash` and `std::equal_to` specializations for
 * `tvm::GlobalVar` allows it to be used as a key in STL tables.  For
 * other IR expressions, the user must specify the type of equality
 * used (e.g. `std::unordered_set<T, StructuralHash, StructuralEqual>`
 * or `std::unordered_set<T, ObjectPtrHash, ObjectPtrEqual>`).
 */
template<>
struct std::hash<litetvm::GlobalVar> {
    std::size_t operator()(const litetvm::GlobalVar& var) const {
        return litetvm::runtime::ObjectPtrHash()(var);
    }
};

template<>
struct std::equal_to<litetvm::GlobalVar> {
    bool operator()(const litetvm::GlobalVar& var_a, const litetvm::GlobalVar& var_b) const {
        return litetvm::runtime::ObjectPtrEqual()(var_a, var_b);
    }
};

#endif//LITETVM_IR_EXPR_H
