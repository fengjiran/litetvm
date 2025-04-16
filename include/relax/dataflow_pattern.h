//
// Created by 赵丹 on 25-4-16.
//

#ifndef LITETVM_RELAX_DATAFLOW_PATTERN_H
#define LITETVM_RELAX_DATAFLOW_PATTERN_H

#include "ir/expr.h"
#include "relax/expr.h"
#include "relax/type.h"
#include "runtime/array.h"
#include "runtime/optional.h"
#include "support/with.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>


namespace litetvm {

namespace arith {
class Analyzer;
}

namespace relax {

class PatternSeq;
class CallPattern;
class OrPattern;
class AndPattern;
class NotPattern;
class ShapePattern;
class StructInfoPattern;
class TypePattern;
class DataTypePattern;
class AttrPattern;
class SameShapeConstraint;

/*!
 * \brief Create used-by relationship between lhs[-1] and rhs[0], with [*lhs, *rhs] returned.
 *
 * \param lhs Left hand side of the used-by relationship.
 * \param rhs Right hand side of the used-by relationship.
 * \param index lhs[-1] is used as the index'th argument of rhs[0].
 * \return PatternSeq The concatenated sequence of [*lhs, *rhs].
 */
LITETVM_API PatternSeq UsedBy(const PatternSeq& lhs, const PatternSeq& rhs, int index = -1);
/*! \brief Syntax sugar of UsedBy(lhs, rhs, -1). */
LITETVM_API PatternSeq operator^(const PatternSeq& lhs, const PatternSeq& rhs);

/*!
 * \brief Create only-used-by relationship between lhs[-1] and rhs[0], with [*lhs, *rhs] returned.
 *
 * \param lhs Left hand side of the used-by relationship.
 * \param rhs Right hand side of the used-by relationship.
 * \param index lhs[-1] is used as the index'th argument of rhs[0].
 * \return PatternSeq The concatenated sequence of [*lhs, *rhs].
 */
LITETVM_API PatternSeq OnlyUsedBy(const PatternSeq& lhs, const PatternSeq& rhs, int index = -1);
/*! \brief Syntax sugar of OnlyUsedBy(lhs, rhs, -1). */
LITETVM_API PatternSeq operator>>(const PatternSeq& lhs, const PatternSeq& rhs);

/*!
 * \brief Base type of all dataflow patterns.
 * \sa DFPattern
 */
class DFPatternNode : public Object {
public:
    static constexpr const char* _type_key = "DFPatternNode";
    static constexpr const uint32_t _type_child_slots = 21;
    TVM_DECLARE_BASE_OBJECT_INFO(DFPatternNode, Object);
};

/*!
 * \brief Managed reference to dataflow patterns.
 * \sa DFPatternNode
 */
class DFPattern : public ObjectRef {
public:
    /*! \brief Syntatic Sugar for creating a CallPattern */
    template<typename... Args>
    CallPattern operator()(Args&&... args) const;
    /*! \brief Syntatic Sugar for creating a CallPattern */
    LITETVM_API CallPattern operator()(const std::vector<DFPattern>& args) const;
    /*! \brief Syntatic Sugar for creating an OrPattern */
    LITETVM_API OrPattern operator|(const DFPattern& other) const;
    /*! \brief Syntatic Sugar for creating an AndPattern */
    LITETVM_API AndPattern operator&(const DFPattern& other) const;
    /*! \brief Syntatic Sugar for creating a NotPattern */
    LITETVM_API NotPattern operator~() const;
    /*! \brief Syntatic Sugar for creating an AttrPattern */
    LITETVM_API AttrPattern HasAttr(const Map<String, ObjectRef>& attrs) const;
    /*! \brief Syntatic Sugar for creating a StructInfoPattern */
    LITETVM_API StructInfoPattern HasStructInfo(const StructInfo& struct_info) const;
    /*! \brief Syntatic Sugar for creating a TypePattern */
    LITETVM_API TypePattern HasType(const Type& type) const;
    /*! \brief Syntatic Sugar for creating a DataTypePattern with a DataType */
    LITETVM_API DataTypePattern HasDtype(const DataType& dtype) const;
    /*! \brief Syntatic Sugar for creating a DataTypePattern with a data type's name */
    LITETVM_API DataTypePattern HasDtype(const std::string& dtype) const;
    /*! \brief Syntatic Sugar for creating a ShapePattern */
    LITETVM_API ShapePattern HasShape(const Array<PrimExpr>& shape) const;
    /*! \brief Syntatic Sugar for creating a ShapePattern */
    LITETVM_API SameShapeConstraint HasSameShapeAs(const DFPattern& other) const;
    /*! \brief Syntatic Sugar for duplicating the current pattern */
    LITETVM_API DFPattern dup() const;

    /*! \brief Implicit conversion from DFPattern to PatternSeq */
    LITETVM_API operator PatternSeq() const;

    TVM_DEFINE_OBJECT_REF_METHODS(DFPattern, ObjectRef, DFPatternNode);
};

/*! \brief Constraint of a DFPattern edge (producer -> consumer) in graph-level matching */
struct PairCons {
    /*! \brief Constraint types of the edge */
    enum Type {
        kUsedBy,     /*!< producer ^ consumer */
        kOnlyUsedBy, /*!< producer >> consumer */
    } type = kUsedBy;
    int index = -1; /*!< The argument index of the producer in the consumer caller site */

    /*!
   * \brief Construct a new PairCons object
   *
   * \param t The constraint type
   * \param index The producer is called as the index'th argument of the consumer function.
   */
    LITETVM_API explicit PairCons(Type t, int index = -1) : type(t), index(index) {}

    bool operator==(const PairCons& other) const {
        return type == other.type && index == other.index;
    }
};

/*! \brief Additional constraints on the graph
 *
 * Unlike PairCons, these may relate nodes that are not directly
 * connected by a DFPattern edge from producer to consumer.  For
 * example, constraining the two branches of an elementwise operation
 * to have the same shape.
 */
class DFConstraintNode : public Object {
public:
    /*! \brief Return the patterns on which the constraint depends */
    virtual Array<DFPattern> GetDependentPatterns() const = 0;

    /*! \brief Convert the constraint to a PrimExpr
   *
   * If the returned boolean parameter is true, then the returned
   * expression is a necessary-and-sufficient condition for evaluating
   * the constraint.  In this case, the matcher may either mark the
   * constraint as satisfied (no need to re-check later), or as failed
   * (need to back-track).
   *
   * If the returned boolean parameter is false, then the returned
   * expression is a necessary-but-not-sufficient condition for
   * evaluating the constraint.  In this case, the matcher may start
   * backtracking as a result of a failed condition, but may not mark
   * the constraint as satisfied.  This typically occurs when the
   * constraint involves a parameter that the matcher has not yet
   * filled.
   *
   * \param match_state A function that can be called to check the
   *    current state of the match.  The function takes as argument a
   *    pattern on which the constraint depends, and returns the relax
   *    variable matched by that pattern, or NullOpt if the pattern
   *    has not yet been matched.
   *
   * \return A tuple of `PrimExpr` and `bool`.  The first element is a
   *    necessary condition for the constraint to be satisfied.  The
   *    second tuple element indicates whether the condition is also
   *    sufficient for the constraint to be satisfied.
   */
    virtual std::tuple<PrimExpr, bool> AsPrimExpr(
            std::function<Optional<Var>(const DFPatternNode*)> match_state) const = 0;

    static constexpr const char* _type_key = "DFConstraintNode";
    static constexpr const uint32_t _type_child_slots = 1;
    TVM_DECLARE_BASE_OBJECT_INFO(DFConstraintNode, Object);
};

class DFConstraint : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(DFConstraint, ObjectRef, DFConstraintNode);
};

/*!
 * \brief A sequence of DFPatterns that the previous DFPattern is connected to the next one.
 * \sa PatternSeq
 */
class PatternSeqNode final : public Object {
public:
    Array<DFPattern> patterns;              /*!< The sequence of DFPatterns */
    std::vector<PairCons> pair_constraints; /*!< Constraints between the previous and next patterns */

    void VisitAttrs(AttrVisitor* v) { v->Visit("patterns", &patterns); }
    static constexpr const char* _type_key = "relax.dpl.PatternSeq";
    TVM_DECLARE_BASE_OBJECT_INFO(PatternSeqNode, Object);
};

/*!
 * \brief Managed reference to pattern sequences.
 * \sa PatternSeqNode
 */
class PatternSeq final : public ObjectRef {
public:
    LITETVM_API explicit PatternSeq(DFPattern init_pattern);
    LITETVM_API explicit PatternSeq(Array<DFPattern> patterns, bool only_used_by = false);

    PatternSeq UsedBy(PatternSeq other, int index = -1) const;
    PatternSeq OnlyUsedBy(PatternSeq other, int index = -1) const;

    /*! \brief Syntatic Sugar for duplicating the current pattern sequence */
    PatternSeq dup() const;

    // friend functions
    friend PatternSeq UsedBy(const PatternSeq& lhs, const PatternSeq& rhs, int index);
    friend PatternSeq OnlyUsedBy(const PatternSeq& lhs, const PatternSeq& rhs, int index);

    TVM_DEFINE_OBJECT_REF_METHODS(PatternSeq, ObjectRef, PatternSeqNode);
};

/*!
 * \brief A context to manage the graph-level pattern matching.
 * \sa PatternContext
 */
class PatternContextNode : public Object {
public:
    /*! \brief Constrainting matched graph with assertion to external uses */
    enum ExternUse {
        kMay,     /*!< No constraints */
        kMustNot, /*!< All nodes except outputs only have internal depedencies in the matched graph. */
    } allow_extern_use = kMay;

    // src node -> <dst node, constraint type> constraints.
    // Dst nodes are kept in a vector to keep them ordered.
    std::map<DFPattern, std::vector<std::pair<DFPattern, std::vector<PairCons>>>> edge_constraints;

    // Underlying DFPattern nodes which the edge constraints may reference
    // Kept as a separate vector of patterns to process constraints in a fixed order.
    std::vector<DFPattern> src_ordered;

    // Non-edge constraints
    std::vector<DFConstraint> validation_constraints;

    static constexpr const char* _type_key = "relax.dpl.PatternContext";
    TVM_DECLARE_FINAL_OBJECT_INFO(PatternContextNode, Object);
};

/*!
 * \brief Managed reference to a pattern context.
 * \sa PatternContextNode
 */
class PatternContext : public ObjectRef {
public:
    LITETVM_API explicit PatternContext(ObjectPtr<Object> n) : ObjectRef(n) {}
    LITETVM_API explicit PatternContext(bool incremental = false);

    const PatternContextNode* operator->() const {
        CHECK(get() != nullptr);
        return static_cast<const PatternContextNode*>(get());
    }

    PatternContextNode* operator->() {
        CHECK(get() != nullptr);
        return static_cast<PatternContextNode*>(get_mutable());
    }

    /*!
   * \brief Build an edge constraint between two patterns (producer and consumer).
   *
   * \param producer The pattern corresponding to the producer node.
   * \param consumer The pattern corresponding to the consumer node.
   * \param cons The constraint type. \sa PairCons
   */
    void add_constraint(DFPattern producer, DFPattern consumer, PairCons cons) {
        auto& pairs = (*this)->edge_constraints[producer];
        auto it = std::find_if(pairs.begin(), pairs.end(),
                               [consumer](auto p) { return p.first == consumer; });
        if (it == pairs.end()) {
            pairs.emplace_back(consumer, std::vector{cons});
        } else {
            auto& vec = it->second;
            CHECK(std::find(vec.cbegin(), vec.cend(), cons) == vec.cend())
                    << "Constraint already exists";
            vec.push_back(cons);
        }

        auto& patterns = (*this)->src_ordered;
        if (std::find(patterns.begin(), patterns.end(), producer) == patterns.end()) {
            patterns.push_back(producer);
        }
    }

    /*!
   * \brief Add a validation constraint
   *
   * \param constraint The new constraint
   */
    void add_constraint(DFConstraint constraint) {
        (*this)->validation_constraints.push_back(constraint);
    }

    /*! \brief Get the constraint context object on the top of the stack */
    LITETVM_API static Optional<PatternContext> Current();

    /*! \brief The RAII-like entry of a constraint context scope */
    LITETVM_API void EnterWithScope() const;
    /*! \brief The RAII-like exit of a constraint context scope */
    LITETVM_API void ExitWithScope() const;

private:
    friend class With<PatternContext>;
};

/*!
 * \brief Pattern for Relax Expression.
 * \sa ExprPattern
 */
class ExprPatternNode : public DFPatternNode {
public:
    Expr expr; /*!< The expression to match */

    void VisitAttrs(AttrVisitor* v) { v->Visit("expr", &expr); }

    static constexpr const char* _type_key = "relax.dpl.ExprPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(ExprPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to an ExprPattern.
 * \sa ExprPatternNode
 */
class ExprPattern : public DFPattern {
public:
    LITETVM_API explicit ExprPattern(Expr expr);
    TVM_DEFINE_OBJECT_REF_METHODS(ExprPattern, DFPattern, ExprPatternNode);
};

/*!
 * \brief A Pattern to Match a Relax Variable.
 * \note The name field matches any string if it is empty.
 * \sa VarPattern
 */
class VarPatternNode : public DFPatternNode {
public:
    String name;
    const String& name_hint() const { return name; }
    void VisitAttrs(AttrVisitor* v) { v->Visit("name", &name); }

    static constexpr const char* _type_key = "relax.dpl.VarPattern";
    static constexpr const uint32_t _type_child_slots = 1;
    TVM_DECLARE_BASE_OBJECT_INFO(VarPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to a VarPattern.
 * \sa VarPatternNode
 */
class VarPattern : public DFPattern {
public:
    /*!
   * \brief Create a pattern matching by variable name.
   *
   * \param name_hint Variable name to match. Any if empty ("").
   */
    LITETVM_API VarPattern(String name_hint);
    TVM_DEFINE_OBJECT_REF_METHODS(VarPattern, DFPattern, VarPatternNode);
};

/*!
 * \brief A Pattern to Match a Relax Dataflow Variable
 * \sa DataflowVarPattern
 */
class DataflowVarPatternNode : public VarPatternNode {
public:
    static constexpr const char* _type_key = "relax.dpl.DataflowVarPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(DataflowVarPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to a DataflowVarPattern.
 * \sa DataflowVarPatternNode
 */
class DataflowVarPattern : public DFPattern {
public:
    /*! \sa VarPattern::VarPattern */
    LITETVM_API DataflowVarPattern(String name_hint);
    TVM_DEFINE_OBJECT_REF_METHODS(DataflowVarPattern, DFPattern, DataflowVarPatternNode);
};

/*!
 * \brief A Pattern to Match a Relax Global Variable
 * \sa GlobalVarPattern
 */
class GlobalVarPatternNode : public VarPatternNode {
public:
    static constexpr const char* _type_key = "relax.dpl.GlobalVarPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(GlobalVarPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to a GlobalVarPattern.
 * \sa GlobalVarPatternNode
 */
class GlobalVarPattern : public DFPattern {
public:
    LITETVM_API GlobalVarPattern(String name_hint);
    TVM_DEFINE_OBJECT_REF_METHODS(GlobalVarPattern, DFPattern, GlobalVarPatternNode);
};

/*!
 * \brief A Pattern to Match a Relax Constant.
 * \sa ConstantPattern
 */
class ConstantPatternNode : public DFPatternNode {
public:
    void VisitAttrs(AttrVisitor* v) {}

    static constexpr const char* _type_key = "relax.dpl.ConstantPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(ConstantPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to a ConstantPattern.
 * \sa ConstantPatternNode
 */
class ConstantPattern : public DFPattern {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(ConstantPattern, DFPattern, ConstantPatternNode);
};

/*!
 * \brief A pattern to match a callable node in Relax.
 * \sa CallPattern
 */
class CallPatternNode : public DFPatternNode {
public:
    /*!
   * \note The op field can be:
   *  - relax::Op which corresponds to the primitive operators.
   *  - user defined functions (Function, GlobalVar, Var).
   */
    DFPattern op;          /*!< The operator (function) being invoked */
    Array<DFPattern> args; /*!< The arguments of the function call */
    /*!
   * \note If varg_default_wildcard is true. Given args of [pA, pB], when matching a call whose
   * arguments are [A, B, ...], the pattern will still match despite N(args) < N(call.args). That
   * said, with varg_default_wildcard set to true, we match the args in the order we have, and
   * regard the rest of the arguments as wildcards.
   */
    bool varg_default_wildcard; /*!< N(args) can be < N(real args) by the padding of Wildcard */

    // Todo(relax-team): Dataflow pattern for StructInfo, and match sinfo_args

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("op", &op);
        v->Visit("args", &args);
    }

    static constexpr const char* _type_key = "relax.dpl.CallPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(CallPatternNode, DFPatternNode);
};

class CallPattern : public DFPattern {
public:
    LITETVM_API CallPattern(DFPattern op, Array<DFPattern> args, bool varg_default_wildcard = false);
    TVM_DEFINE_OBJECT_REF_METHODS(CallPattern, DFPattern, CallPatternNode);
};

/*!
 * \brief A pattern to match an array of PrimExpr.
 * \sa PrimArrPattern
 * \note This is often used to match shapes specified as arguments to a function.
 */
class PrimArrPatternNode : public DFPatternNode {
public:
    Array<PrimExpr> fields; /*!< The array to match */
    void VisitAttrs(AttrVisitor* v) { v->Visit("fields", &fields); }
    static constexpr const char* _type_key = "relax.dpl.PrimArrPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(PrimArrPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to a PrimArrPattern.
 * \sa PrimArrPatternNode
 */
class PrimArrPattern : public DFPattern {
public:
    LITETVM_API PrimArrPattern(Array<PrimExpr> arr);
    TVM_DEFINE_OBJECT_REF_METHODS(PrimArrPattern, DFPattern, PrimArrPatternNode);
};

/*!
 * \brief A pattern to match a Relax Function
 * \sa Function
 * \sa FunctionPattern
 */
class FunctionPatternNode : public DFPatternNode {
public:
    Array<DFPattern> params; /*!< The parameters of the function */
    /*!
   * \note Note that in Relax, the function body is a SeqExpr which contains
   * 1) SeqExprNode::blocks, which is a list of blocks of statements; and 2)
   * SeqExprNode::body, which is an Expr that can be anything. FunctionPattern
   * only matches the body of the function (writing patterns to statements is tricky).
   */
    DFPattern body; /*!< The body of the function */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("params", &params);
        v->Visit("body", &body);
    }

    static constexpr const char* _type_key = "relax.dpl.FunctionPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(FunctionPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to FunctionPatternNode.
 * \sa FunctionPatternNode
 */
class FunctionPattern : public DFPattern {
public:
    /*!
   * \brief Constructor
   * \param params The parameters of the function.
   * \param body The body of the function.
   */
    LITETVM_API FunctionPattern(Array<DFPattern> params, DFPattern body);

    TVM_DEFINE_OBJECT_REF_METHODS(FunctionPattern, DFPattern, FunctionPatternNode);
};

/*!
 * \brief Pattern to match a tuple of ordered expressions.
 * \sa TuplePattern
 */
class TuplePatternNode : public DFPatternNode {
public:
    Array<DFPattern> fields; /*!< The fields of the tuple */

    void VisitAttrs(AttrVisitor* v) { v->Visit("fields", &fields); }

    static constexpr const char* _type_key = "relax.dpl.TuplePattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(TuplePatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to TuplePatternNode.
 * \sa TuplePatternNode
 */
class TuplePattern : public DFPattern {
public:
    LITETVM_API explicit TuplePattern(Array<DFPattern> fields);
    TVM_DEFINE_OBJECT_REF_METHODS(TuplePattern, DFPattern, TuplePatternNode);
};

/*!
 * \brief A pattern to match multiple expressions unorderedly.
 * \sa UnorderedTuplePattern
 */
class UnorderedTuplePatternNode : public DFPatternNode {
public:
    Array<DFPattern> fields; /*!< The fields of the tuple */

    void VisitAttrs(AttrVisitor* v) { v->Visit("fields", &fields); }

    static constexpr const char* _type_key = "relax.dpl.UnorderedTuplePattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(UnorderedTuplePatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to UnorderedTuplePatternNode.
 * \sa UnorderedTuplePatternNode
 */
class UnorderedTuplePattern : public DFPattern {
public:
    LITETVM_API explicit UnorderedTuplePattern(Array<DFPattern> fields);
    TVM_DEFINE_OBJECT_REF_METHODS(UnorderedTuplePattern, DFPattern, UnorderedTuplePatternNode);
};

/*!
 * \brief A pattern to match n'th indexing to a tuple.
 * \sa TupleGetItem
 * \sa TupleGetItemPattern
 */
class TupleGetItemPatternNode : public DFPatternNode {
public:
    DFPattern tuple; /*!< The tuple Expression */
    int index;       /*!< The index of the tuple with -1 meaning arbitrary */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("tuple", &tuple);
        v->Visit("index", &index);
    }

    static constexpr const char* _type_key = "relax.dpl.TupleGetItemPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(TupleGetItemPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to TupleGetItemPatternNode.
 * \sa TupleGetItemPatternNode
 */
class TupleGetItemPattern : public DFPattern {
public:
    LITETVM_API TupleGetItemPattern(DFPattern tuple, int index);
    TVM_DEFINE_OBJECT_REF_METHODS(TupleGetItemPattern, DFPattern, TupleGetItemPatternNode);
};

/*!
 * \brief Match a conjunction of other patterns.
 * \sa AndPattern
 */
class AndPatternNode : public DFPatternNode {
public:
    DFPattern left;  /*!< The left hand side of the conjunction */
    DFPattern right; /*!< The right hand side of the conjunction */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("left", &left);
        v->Visit("right", &right);
    }

    static constexpr const char* _type_key = "relax.dpl.AndPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(AndPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to AndPatternNode.
 * \sa AndPatternNode
 */
class AndPattern : public DFPattern {
public:
    LITETVM_API AndPattern(DFPattern lhs, DFPattern rhs);
    TVM_DEFINE_OBJECT_REF_METHODS(AndPattern, DFPattern, AndPatternNode);
};

/*!
 * \brief Match a disjunction of other patterns.
 * \sa OrPattern
 */
class OrPatternNode : public DFPatternNode {
public:
    DFPattern left;  /*!< The left hand side of the disjunction */
    DFPattern right; /*!< The right hand side of the disjunction */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("left", &left);
        v->Visit("right", &right);
    }

    static constexpr const char* _type_key = "relax.dpl.OrPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(OrPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to OrPatternNode.
 * \sa OrPatternNode
 */
class OrPattern : public DFPattern {
public:
    LITETVM_API OrPattern(DFPattern left, DFPattern right);
    TVM_DEFINE_OBJECT_REF_METHODS(OrPattern, DFPattern, OrPatternNode);
};

/*!
 * \brief Pattern for rejecting a certain pattern.
 * \sa NotPattern
 */
class NotPatternNode : public DFPatternNode {
public:
    DFPattern reject; /*!< The pattern to reject */

    void VisitAttrs(AttrVisitor* v) { v->Visit("reject", &reject); }

    static constexpr const char* _type_key = "relax.dpl.NotPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(NotPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to NotPatternNode.
 * \sa NotPatternNode
 */
class NotPattern : public DFPattern {
public:
    LITETVM_API NotPattern(DFPattern reject);
    TVM_DEFINE_OBJECT_REF_METHODS(NotPattern, DFPattern, NotPatternNode);
};

/*!
 * \brief Wildcard Pattern is a pattern that can match anything.
 * \sa WildcardPattern
 */
class WildcardPatternNode : public DFPatternNode {
public:
    void VisitAttrs(AttrVisitor* v) {}

    static constexpr const char* _type_key = "relax.dpl.WildcardPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(WildcardPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to WildcardPatternNode.
 * \sa WildcardPatternNode
 */
class WildcardPattern : public DFPattern {
public:
    WildcardPattern();

    // Declaring WildcardPattern declared as non-nullable avoids the
    // default zero-parameter constructor for ObjectRef with `data_ =
    // nullptr`.  This allows a zero-parameter constructor to be
    // declared here, to create a valid wildcard instance.

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(WildcardPattern, DFPattern, WildcardPatternNode);
};

/*!
 * \brief Pattern for matching a certain type.
 * \sa TypePattern
 */
class TypePatternNode : public DFPatternNode {
public:
    DFPattern pattern; /*!< The pattern to match */
    Type type;         /*!< The type to match */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("pattern", &pattern);
        v->Visit("type", &type);
    }

    static constexpr const char* _type_key = "relax.dpl.TypePattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(TypePatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to TypePatternNode.
 * \sa TypePatternNode
 */
class TypePattern : public DFPattern {
public:
    LITETVM_API TypePattern(DFPattern pattern, Type type);
    TVM_DEFINE_OBJECT_REF_METHODS(TypePattern, DFPattern, TypePatternNode);
};

/*!
 * \brief Pattern for matching a certain struct info.
 * \sa StructInfoPattern
 */
class StructInfoPatternNode : public DFPatternNode {
public:
    DFPattern pattern;      /*!< The pattern to match */
    StructInfo struct_info; /*!< The type to match */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("pattern", &pattern);
        v->Visit("struct_info", &struct_info);
    }

    static constexpr const char* _type_key = "relax.dpl.StructInfoPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(StructInfoPatternNode, DFPatternNode);
};

class StructInfoPattern : public DFPattern {
public:
    LITETVM_API StructInfoPattern(DFPattern pattern, StructInfo struct_info);
    TVM_DEFINE_OBJECT_REF_METHODS(StructInfoPattern, DFPattern, StructInfoPatternNode);
};

/*!
 * \brief A pattern that asserting a root pattern has a certain shape.
 * \sa ShapePattern
 */
class ShapePatternNode : public DFPatternNode {
public:
    DFPattern pattern;     /*!< The root pattern to match */
    Array<PrimExpr> shape; /*!< The shape to match */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("pattern", &pattern);
        v->Visit("shape", &shape);
    }

    static constexpr const char* _type_key = "relax.dpl.ShapePattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(ShapePatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to ShapePatternNode.
 * \sa ShapePatternNode
 */
class ShapePattern : public DFPattern {
public:
    LITETVM_API ShapePattern(DFPattern pattern, Array<PrimExpr> type);
    TVM_DEFINE_OBJECT_REF_METHODS(ShapePattern, DFPattern, ShapePatternNode);
};

/*!
 * \brief A pattern that asserting multiple root patterns have the same shape
 * \sa SameShapePattern
 */
class SameShapeConstraintNode : public DFConstraintNode {
public:
    Array<DFPattern> args; /*!< The patterns with matching shapes */

    Array<DFPattern> GetDependentPatterns() const override { return args; }

    std::tuple<PrimExpr, bool> AsPrimExpr(
            std::function<Optional<Var>(const DFPatternNode*)> match_state) const override;

    void VisitAttrs(AttrVisitor* v) { v->Visit("args", &args); }

    static constexpr const char* _type_key = "relax.dpl.SameShapeConstraint";
    TVM_DECLARE_FINAL_OBJECT_INFO(SameShapeConstraintNode, DFConstraintNode);
};

/*!
 * \brief Managed reference to SameShapePatternNode.
 * \sa SameShapePatternNode
 */
class SameShapeConstraint : public DFConstraint {
public:
    LITETVM_API SameShapeConstraint(Array<DFPattern> args);
    TVM_DEFINE_OBJECT_REF_METHODS(SameShapeConstraint, DFConstraint, SameShapeConstraintNode);
};

/*!
 * \brief A pattern that asserting a root pattern has a certain data type.
 * \sa DataTypePattern
 */
class DataTypePatternNode : public DFPatternNode {
public:
    DFPattern pattern; /*!< The root pattern to match */
    DataType dtype;    /*!< The data type to match */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("pattern", &pattern);
        v->Visit("dtype", &dtype);
    }

    static constexpr const char* _type_key = "relax.dpl.DataTypePattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(DataTypePatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to DataTypePatternNode.
 * \sa DataTypePatternNode
 */
class DataTypePattern : public DFPattern {
public:
    LITETVM_API DataTypePattern(DFPattern pattern, DataType dtype);
    TVM_DEFINE_OBJECT_REF_METHODS(DataTypePattern, DFPattern, DataTypePatternNode);
};

/*!
 * \brief A pattern that asserting a root pattern has certain attributes.
 * \sa AttrPattern
 */
class AttrPatternNode : public DFPatternNode {
public:
    DFPattern pattern; /*!< The root pattern to match */
    DictAttrs attrs;   /*!< The attributes (a map/dictionary) to match */

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("pattern", &pattern);
        v->Visit("attrs", &attrs);
    }

    static constexpr const char* _type_key = "relax.dpl.AttrPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(AttrPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to AttrPatternNode.
 * \sa AttrPatternNode
 */
class AttrPattern : public DFPattern {
public:
    LITETVM_API AttrPattern(DFPattern pattern, DictAttrs attrs);
    TVM_DEFINE_OBJECT_REF_METHODS(AttrPattern, DFPattern, AttrPatternNode);
};

/*!
 * \brief A pattern of external function.
 * \sa ExternFunc
 * \sa ExternFuncPattern
 */
class ExternFuncPatternNode : public DFPatternNode {
public:
    String global_symbol_; /*!< The global symbol name of the external function */

    /*! \brief The external function name */
    const String& global_symbol() const { return global_symbol_; }
    void VisitAttrs(AttrVisitor* v) { v->Visit("global_symbol", &global_symbol_); }

    static constexpr const char* _type_key = "relax.dpl.ExternFuncPattern";
    TVM_DECLARE_FINAL_OBJECT_INFO(ExternFuncPatternNode, DFPatternNode);
};

/*!
 * \brief Managed reference to ExternFuncPatternNode.
 * \sa ExternFuncPatternNode
 */
class ExternFuncPattern : public DFPattern {
public:
    LITETVM_API ExternFuncPattern(String global_symbol);
    TVM_DEFINE_OBJECT_REF_METHODS(ExternFuncPattern, DFPattern, ExternFuncPatternNode);
};

/*! \brief Syntatic Sugar for creating a VarPattern with a name */
VarPattern IsVar(const String& name);
/*! \brief Syntatic Sugar for creating a ConstantPattern */
ConstantPattern IsConst();
/*! \brief Syntatic Sugar for creating a WildcardPattern */
WildcardPattern Wildcard();
/*! \brief Syntatic Sugar for creating a ExprPattern */
ExprPattern IsExpr(const Expr& expr);
/*! \brief Syntatic Sugar for creating a ExprPattern base on an Op */
ExprPattern IsOp(const String& op_name);
/*! \brief Syntatic Sugar for call_tir (return a tensor) */
// Todo(relax-team): Dataflow pattern for StructInfo, and match out_sinfo
CallPattern IsCallTIR(const String& name, Optional<TuplePattern> args = NullOpt);
/*! \brief Syntatic Sugar for call_tir (return a tuple of tensor) */
CallPattern IsCallTIR(const String& name, TuplePattern var_args);
/*! \brief Syntatic Sugar for call_dps_packed (return a tensor) */
CallPattern IsCallDPSPacked(const String& name, Optional<TuplePattern> args = NullOpt);
/*! \brief Syntatic Sugar for call_dps_packed (return a tuple of tensor) */
CallPattern IsCallDPSPacked(const String& name, TuplePattern var_args);
/*! \brief Syntatic Sugar for creating TuplePattern or UnorderedTuplePattern (unordered=true) */
DFPattern IsTuple(const Array<DFPattern>& fields, bool unordered = false);
/*! \brief Syntatic Sugar for creating a TupleGetItemPattern */
TupleGetItemPattern IsTupleGetItem(const DFPattern tuple, int index = -1);

/*! \brief Implementation of the templated CallPattern syntax sugar */
template<typename... Args>
CallPattern DFPattern::operator()(Args&&... args) const {
    return CallPattern(GetRef<DFPattern>(this->get()),
                       Array<DFPattern>({std::forward<Args>(args)...}));
}

}// namespace relax

}// namespace litetvm

#endif//LITETVM_RELAX_DATAFLOW_PATTERN_H
