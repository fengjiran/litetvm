//
// Created by 赵丹 on 25-3-13.
//

#ifndef LITETVM_TIR_EXPR_H
#define LITETVM_TIR_EXPR_H

#include "tir/var.h"

namespace litetvm {
namespace tir {

/*! \brief String constants, only used in asserts. */
class StringImmNode : public PrimExprNode {
public:
    /*! \brief The constant value content. */
    String value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("value", &value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const StringImmNode* other, SEqualReducer equal) const {
        return equal(value, other->value);
    }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce(value); }

    static constexpr const char* _type_key = "tir.StringImm";
    TVM_DECLARE_FINAL_OBJECT_INFO(StringImmNode, PrimExprNode);
};

/*!
 * \brief Managed reference to StringImmNode.
 * \sa StringImmNode
 */
class StringImm : public PrimExpr {
public:
    // TVM_DLL StringImm(String value, Span span = Span());
    LITETVM_API StringImm(String value);
    TVM_DEFINE_OBJECT_REF_METHODS(StringImm, PrimExpr, StringImmNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(StringImmNode);
};

/*!
 * \brief Cast value from one data type to another.
 * \note The lanes of value should keep fixed.
 */
class CastNode : public PrimExprNode {
public:
    /*! \brief Original data type. */
    PrimExpr value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("value", &value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const CastNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(value, other->value);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(value);
    }

    static constexpr const char* _type_key = "tir.Cast";
    TVM_DECLARE_FINAL_OBJECT_INFO(CastNode, PrimExprNode);
};

/*!
 * \brief Managed reference to CastNode
 * \sa CastNode
 */
class Cast : public PrimExpr {
public:
    // TVM_DLL Cast(DataType dtype, PrimExpr value, Span span = Span());
    LITETVM_API Cast(DataType dtype, PrimExpr value);
    TVM_DEFINE_OBJECT_REF_METHODS(Cast, PrimExpr, CastNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(CastNode);
};

/*!
 * \brief Base template to implement binary ops, using CRTP.
 * \tparam T The type of the child class.
 */
template<typename T>
class BinaryOpNode : public PrimExprNode {
public:
    /*! \brief The left operand. */
    PrimExpr a;
    /*! \brief The right operand. */
    PrimExpr b;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &(this->dtype));
        v->Visit("a", &a);
        v->Visit("b", &b);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const T* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(a, other->a) && equal(b, other->b);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(a);
        hash_reduce(b);
    }

    TVM_DECLARE_FINAL_OBJECT_INFO(T, PrimExprNode);
};

/*! \brief a + b */
class AddNode : public BinaryOpNode<AddNode> {
public:
    static constexpr const char* _type_key = "tir.Add";
};

/*!
 * \brief Managed reference to AddNode
 * \sa AddNode
 */
class Add : public PrimExpr {
public:
    // TVM_DLL Add(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API Add(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(Add, PrimExpr, AddNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(AddNode);
};

/*! \brief a - b */
class SubNode : public BinaryOpNode<SubNode> {
public:
    static constexpr const char* _type_key = "tir.Sub";
};

/*!
 * \brief Managed reference to SubNode
 * \sa SubNode
 */
class Sub : public PrimExpr {
public:
    // TVM_DLL Sub(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API Sub(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(Sub, PrimExpr, SubNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(SubNode);
};


/*! \brief a * b */
class MulNode : public BinaryOpNode<MulNode> {
public:
    static constexpr const char* _type_key = "tir.Mul";
};

/*!
 * \brief Managed reference to MulNode
 * \sa MulNode
 */
class Mul : public PrimExpr {
public:
    // TVM_DLL Mul(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API Mul(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(Mul, PrimExpr, MulNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(MulNode);
};

/*!
 * \brief a / b in the C semnatics.
 * \note For integer division, C standard uses trunc div.
 */
class DivNode : public BinaryOpNode<DivNode> {
public:
    static constexpr const char* _type_key = "tir.Div";
};

/*!
 * \brief Managed reference to DivNode
 * \sa DivNode
 */
class Div : public PrimExpr {
public:
    // TVM_DLL Div(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API Div(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(Div, PrimExpr, DivNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(DivNode);
};

/*!
 * \brief a % b in the C semnatics.
 * \note For integer division, C standard uses trunc div.
 */
class ModNode : public BinaryOpNode<ModNode> {
public:
    static constexpr const char* _type_key = "tir.Mod";
};

/*!
 * \brief Managed reference to ModNode
 * \sa ModNode
 */
class Mod : public PrimExpr {
public:
    // TVM_DLL Mod(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API Mod(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(Mod, PrimExpr, ModNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(ModNode);
};


/*! \brief Floor division, floor(a/b) */
class FloorDivNode : public BinaryOpNode<FloorDivNode> {
public:
    static constexpr const char* _type_key = "tir.FloorDiv";
};

/*!
 * \brief Managed reference to FloorDivNode
 * \sa FloorDivNode
 */
class FloorDiv : public PrimExpr {
public:
    // TVM_DLL FloorDiv(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API FloorDiv(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(FloorDiv, PrimExpr, FloorDivNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(FloorDivNode);
};


/*! \brief The remainder of the floordiv */
class FloorModNode : public BinaryOpNode<FloorModNode> {
public:
    static constexpr const char* _type_key = "tir.FloorMod";
};

/*!
 * \brief Managed reference to FloorModNode
 * \sa FloorModNode
 */
class FloorMod : public PrimExpr {
public:
    // TVM_DLL FloorMod(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API FloorMod(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(FloorMod, PrimExpr, FloorModNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(FloorModNode);
};

/*! \brief min(a, b) */
class MinNode : public BinaryOpNode<MinNode> {
public:
    static constexpr const char* _type_key = "tir.Min";
};

/*!
 * \brief Managed reference to MinNode
 * \sa MinNode
 */
class Min : public PrimExpr {
public:
    // TVM_DLL Min(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API Min(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(Min, PrimExpr, MinNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(MinNode);
};

/*! \brief max(a, b) */
class MaxNode : public BinaryOpNode<MaxNode> {
public:
    static constexpr const char* _type_key = "tir.Max";
};

/*!
 * \brief Managed reference to MaxNode
 * \sa MaxNode
 */
class Max : public PrimExpr {
public:
    // TVM_DLL Max(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API Max(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(Max, PrimExpr, MaxNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(MaxNode);
};

/*!
 * \brief Base template to implement comparison ops.
 * \tparam T The type of the child class.
 */
template<typename T>
class CmpOpNode : public PrimExprNode {
public:
    /*! \brief The left operand. */
    PrimExpr a;
    /*! \brief The right operand. */
    PrimExpr b;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &(this->dtype));
        v->Visit("a", &a);
        v->Visit("b", &b);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const T* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(a, other->a) && equal(b, other->b);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(a);
        hash_reduce(b);
    }

    TVM_DECLARE_FINAL_OBJECT_INFO(T, PrimExprNode);
};

/*! \brief a == b */
class EQNode : public CmpOpNode<EQNode> {
public:
    static constexpr const char* _type_key = "tir.EQ";
};

/*!
 * \brief Managed reference to EQNode
 * \sa EQNode
 */
class EQ : public PrimExpr {
public:
    // TVM_DLL EQ(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API EQ(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(EQ, PrimExpr, EQNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(EQNode);
};

/*! \brief a != b */
class NENode : public CmpOpNode<NENode> {
public:
    static constexpr const char* _type_key = "tir.NE";
};

/*!
 * \brief Managed reference to NENode
 * \sa NENode
 */
class NE : public PrimExpr {
public:
    // TVM_DLL NE(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API NE(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(NE, PrimExpr, NENode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(NENode);
};

/*! \brief a < b */
class LTNode : public CmpOpNode<LTNode> {
public:
    static constexpr const char* _type_key = "tir.LT";
};

/*!
 * \brief Managed reference to LTNode
 * \sa LTNode
 */
class LT : public PrimExpr {
public:
    // TVM_DLL LT(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API LT(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(LT, PrimExpr, LTNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(LTNode);
};


/*! \brief a <= b */
struct LENode : public CmpOpNode<LENode> {
public:
    static constexpr const char* _type_key = "tir.LE";
};

/*!
 * \brief Managed reference to LENode
 * \sa LENode
 */
class LE : public PrimExpr {
public:
    // TVM_DLL LE(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API LE(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(LE, PrimExpr, LENode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(LENode);
};

/*! \brief a > b */
class GTNode : public CmpOpNode<GTNode> {
public:
    static constexpr const char* _type_key = "tir.GT";
};

/*!
 * \brief Managed reference to GTNode
 * \sa GTNode
 */
class GT : public PrimExpr {
public:
    // TVM_DLL GT(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API GT(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(GT, PrimExpr, GTNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(GTNode);
};

/*! \brief a >= b */
class GENode : public CmpOpNode<GENode> {
public:
    static constexpr const char* _type_key = "tir.GE";
};

/*!
 * \brief Managed reference to GENode
 * \sa GENode
 */
class GE : public PrimExpr {
public:
    // TVM_DLL GE(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API GE(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(GE, PrimExpr, GENode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(GENode);
};

/*! \brief a && b */
class AndNode : public PrimExprNode {
public:
    /*! \brief The left operand. */
    PrimExpr a;
    /*! \brief The right operand. */
    PrimExpr b;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &(this->dtype));
        v->Visit("a", &a);
        v->Visit("b", &b);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const AndNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(a, other->a) && equal(b, other->b);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(a);
        hash_reduce(b);
    }

    static constexpr const char* _type_key = "tir.And";
    TVM_DECLARE_FINAL_OBJECT_INFO(AndNode, PrimExprNode);
};

/*!
 * \brief Managed reference to AndNode
 * \sa AndNode
 */
class And : public PrimExpr {
public:
    // TVM_DLL And(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API And(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(And, PrimExpr, AndNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(AndNode);
};

/*! \brief a || b */
class OrNode : public PrimExprNode {
public:
    /*! \brief The left operand. */
    PrimExpr a;
    /*! \brief The right operand. */
    PrimExpr b;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("a", &a);
        v->Visit("b", &b);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const OrNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(a, other->a) && equal(b, other->b);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(a);
        hash_reduce(b);
    }

    static constexpr const char* _type_key = "tir.Or";
    TVM_DECLARE_FINAL_OBJECT_INFO(OrNode, PrimExprNode);
};

/*!
 * \brief Managed reference to OrNode
 * \sa OrNode
 */
class Or : public PrimExpr {
public:
    // TVM_DLL Or(PrimExpr a, PrimExpr b, Span span = Span());
    LITETVM_API Or(PrimExpr a, PrimExpr b);
    TVM_DEFINE_OBJECT_REF_METHODS(Or, PrimExpr, OrNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(OrNode);
};

/*! \brief !a */
class NotNode : public PrimExprNode {
public:
    /*! \brief The input operand. */
    PrimExpr a;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("a", &a);
        v->Visit("span", &span);
    }

    bool SEqualReduce(const NotNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(a, other->a);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(a);
    }

    static constexpr const char* _type_key = "tir.Not";
    TVM_DECLARE_FINAL_OBJECT_INFO(NotNode, PrimExprNode);
};

/*!
 * \brief Managed reference to NotNode
 * \sa NotNode
 */
class Not : public PrimExpr {
public:
    // TVM_DLL Not(PrimExpr a, Span span = Span());
    LITETVM_API Not(PrimExpr a);
    TVM_DEFINE_OBJECT_REF_METHODS(Not, PrimExpr, NotNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(NotNode);
};

/*!
 * \brief return true_value if condition is true, otherwise return false_value.
 * \note Both true_value and false_value could be evaluated
 *       regardless of the condition value.
 *       Do not use it to guard against outbound access,
 *       please use if_then_else instead.
 */
class SelectNode : public PrimExprNode {
public:
    /*! \brief The condition */
    PrimExpr condition;
    /*! \brief value to be returned when condition is true. */
    PrimExpr true_value;
    /*! \brief value to be returned when condition is false. */
    PrimExpr false_value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("condition", &condition);
        v->Visit("true_value", &true_value);
        v->Visit("false_value", &false_value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const SelectNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(condition, other->condition) &&
               equal(true_value, other->true_value) && equal(false_value, other->false_value);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(condition);
        hash_reduce(true_value);
        hash_reduce(false_value);
    }

    static constexpr const char* _type_key = "tir.Select";
    TVM_DECLARE_FINAL_OBJECT_INFO(SelectNode, PrimExprNode);
};

/*!
 * \brief Managed reference to SelectNode
 * \sa SelectNode
 */
class Select : public PrimExpr {
public:
    // TVM_DLL Select(PrimExpr condition, PrimExpr true_value, PrimExpr false_value, Span span = Span());
    LITETVM_API Select(PrimExpr condition, PrimExpr true_value, PrimExpr false_value);

    TVM_DEFINE_OBJECT_REF_METHODS(Select, PrimExpr, SelectNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(SelectNode);
};


/*!
 * \brief Call node.
 */
class CallNode : public PrimExprNode {
public:
    /*!
   * \brief The operator(function) being invoked
   *
   *  - It can be tvm::Op which corresponds to the primitive operators(intrinsics).
   *  - It can also be another function in the IRModule (GlobalVar).
   */
    RelaxExpr op;

    /*! \brief The arguments. */
    Array<PrimExpr> args;
    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("op", &op);
        v->Visit("args", &args);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const CallNode* other, SEqualReducer equal) const {
        return equal(dtype, other->dtype) && equal(op, other->op) && equal(args, other->args);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(dtype);
        hash_reduce(op);
        hash_reduce(args);
    }

    static constexpr const char* _type_key = "tir.Call";
    TVM_DECLARE_FINAL_OBJECT_INFO(CallNode, PrimExprNode);
};

/*!
 * \brief Managed reference to CallNode
 * \sa CallNode
 */
class Call : public PrimExpr {
public:
    // TVM_DLL Call(DataType dtype, RelaxExpr op, Array<PrimExpr> args, Span span = Span());
    LITETVM_API Call(DataType dtype, RelaxExpr op, Array<PrimExpr> args);
    TVM_DEFINE_OBJECT_REF_METHODS(Call, PrimExpr, CallNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(CallNode);
};

}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_EXPR_H
