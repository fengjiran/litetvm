//
// Created by 赵丹 on 25-4-10.
//

#ifndef LITETVM_TIR_STMT_H
#define LITETVM_TIR_STMT_H

#include "tir/expr.h"

#include <string>
#include <type_traits>
#include <vector>

namespace litetvm {
namespace tir {

/*! \brief Base node of all statements. */
class StmtNode : public Object {
public:
    /*!
   * \brief Span that points to the original source code.
   *        Reserved debug information.
   */
    // mutable Span span;

    StmtNode() = default;

    TVM_OBJECT_ENABLE_SCRIPT_PRINTER();

    static constexpr const char* _type_key = "tir.Stmt";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    static constexpr uint32_t _type_child_slots = 15;
    TVM_DECLARE_BASE_OBJECT_INFO(StmtNode, Object);
};

/*! \brief Container of all statements */
class Stmt : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(Stmt, ObjectRef, StmtNode);
};

/*!
 * \brief Let binding, bind var to value, then run body.
 */
class LetStmtNode : public StmtNode {
public:
    /*! \brief The variable. */
    Var var;
    /*! \brief The value to be bound. */
    PrimExpr value;
    /*! \brief The body block. */
    Stmt body;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("var", &var);
        v->Visit("value", &value);
        v->Visit("body", &body);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const LetStmtNode* other, SEqualReducer equal) const {
        return equal.DefEqual(var, other->var) && equal(value, other->value) &&
               equal(body, other->body);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce.DefHash(var);
        hash_reduce(value);
        hash_reduce(body);
    }

    static constexpr const char* _type_key = "tir.LetStmt";
    TVM_DECLARE_FINAL_OBJECT_INFO(LetStmtNode, StmtNode);
};

/*!
 * \brief Managed reference to LetStmtNode.
 * \sa LetStmtNode
 */
class LetStmt : public Stmt {
public:
    LITETVM_API LetStmt(Var var, PrimExpr value, Stmt body);

    TVM_DEFINE_OBJECT_REF_METHODS(LetStmt, Stmt, LetStmtNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(LetStmtNode);
};

/*!
 * \brief Define certain auxiliary attribute for the body to be a symbolic value.
 *  This provide auxiliary information for IR passes that transforms body.
 *
 *  In terms of effect, this is equivalent to Block(Evaluate(value), body).
 *
 *  Examples of possible usage:
 *    - Bound of function, variables.
 *    - Hint which block corresponds to a parallel region.
 */
class AttrStmtNode : public StmtNode {
public:
    /*! \brief this is attribute about certain node */
    ObjectRef node;
    /*! \brief the type key of the attribute */
    String attr_key;
    /*! \brief The attribute value, value is well defined at current scope. */
    PrimExpr value;
    /*! \brief The body statement to be executed */
    Stmt body;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("node", &node);
        v->Visit("attr_key", &attr_key);
        v->Visit("value", &value);
        v->Visit("body", &body);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const AttrStmtNode* other, SEqualReducer equal) const {
        return equal(node, other->node) && equal(attr_key, other->attr_key) &&
               equal(value, other->value) && equal(body, other->body);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(node);
        hash_reduce(attr_key);
        hash_reduce(value);
        hash_reduce(body);
    }

    static constexpr const char* _type_key = "tir.AttrStmt";
    TVM_DECLARE_FINAL_OBJECT_INFO(AttrStmtNode, StmtNode);
};

/*!
 * \brief Managed reference to AttrStmtNode.
 * \sa AttrStmtNode
 */
class AttrStmt : public Stmt {
public:
    LITETVM_API AttrStmt(ObjectRef node, String attr_key, PrimExpr value, Stmt body);

    TVM_DEFINE_OBJECT_REF_METHODS(AttrStmt, Stmt, AttrStmtNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(AttrStmtNode);
};

/*!
 * \brief Assert condition, if an error occurs, return the error message.
 */
class AssertStmtNode : public StmtNode {
public:
    /*! \brief Condition to be checked. */
    PrimExpr condition;
    /*! \brief Error message when assertion failed. */
    PrimExpr message;
    /*!
     * \brief Body which this assertion holds true.
     *  Will be executed after the assertion.
     */
    Stmt body;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("condition", &condition);
        v->Visit("message", &message);
        v->Visit("body", &body);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const AssertStmtNode* other, SEqualReducer equal) const {
        return equal(condition, other->condition) && equal(message, other->message) &&
               equal(body, other->body);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(condition);
        hash_reduce(message);
        hash_reduce(body);
    }

    static constexpr const char* _type_key = "tir.AssertStmt";
    TVM_DECLARE_FINAL_OBJECT_INFO(AssertStmtNode, StmtNode);
};

/*!
 * \brief Managed reference to AssertStmtNode.
 * \sa AssertStmtNode
 */
class AssertStmt : public Stmt {
public:
    LITETVM_API AssertStmt(PrimExpr condition, PrimExpr message, Stmt body);

    TVM_DEFINE_OBJECT_REF_METHODS(AssertStmt, Stmt, AssertStmtNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(AssertStmtNode);
};

/*!
 * \brief Store value to the high dimension buffer.
 *
 * \code
 *
 *  buffer[i, j] = value;
 *
 * \endcode
 * \sa BufferLoad
 */
class BufferStoreNode : public StmtNode {
public:
    /*! \brief The buffer variable. */
    Buffer buffer;
    /*! \brief The value to be stored. */
    PrimExpr value;
    /*! \brief The indices location to be stored. */
    Array<PrimExpr> indices;
    /*! \brief The predicate mask for storing values. */
    Optional<PrimExpr> predicate;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("buffer", &buffer);
        v->Visit("value", &value);
        v->Visit("indices", &indices);
        v->Visit("predicate", &predicate);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const BufferStoreNode* other, SEqualReducer equal) const {
        return equal(buffer, other->buffer) && equal(value, other->value) &&
               equal(indices, other->indices);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(buffer);
        hash_reduce(value);
        hash_reduce(indices);
        hash_reduce(predicate);
    }

    static constexpr const char* _type_key = "tir.BufferStore";
    TVM_DECLARE_FINAL_OBJECT_INFO(BufferStoreNode, StmtNode);
};

/*!
 * \brief Managed reference to BufferStoreNode.
 * \sa BufferStoreNode
 */
class BufferStore : public Stmt {
public:
    LITETVM_API explicit BufferStore(Buffer buffer, PrimExpr value, Array<PrimExpr> indices,
                                     Optional<PrimExpr> predicate = NullOpt);

    TVM_DEFINE_OBJECT_REF_METHODS(BufferStore, Stmt, BufferStoreNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(BufferStoreNode);
};

/*!
 * \brief Annotate the region where the buffer need to
 *  be read and write in the body.
 *  We only need to allocate the space for the corresponding region.
 *
 * \note There should be at most one BufferRealize for each buffer.
 *       BufferRealize is not necessary for external buffers,
 *       since they are assumed to be fully allocated.
 *
 * \sa BufferLoad, BufferStore
 */
class BufferRealizeNode : public StmtNode {
public:
    /*! \brief The buffer variable. */
    Buffer buffer;
    /*! \brief Bounds to be realized */
    Array<Range> bounds;
    /*! \brief Only realize if condition holds. */
    PrimExpr condition;
    /*! \brief The body of realization. */
    Stmt body;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("buffer", &buffer);
        v->Visit("bounds", &bounds);
        v->Visit("condition", &condition);
        v->Visit("body", &body);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const BufferRealizeNode* other, SEqualReducer equal) const {
        return equal(buffer, other->buffer) && equal(bounds, other->bounds) &&
               equal(condition, other->condition) && equal(body, other->body);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(buffer);
        hash_reduce(bounds);
        hash_reduce(condition);
        hash_reduce(body);
    }

    BufferRealizeNode() = default;
    BufferRealizeNode(Buffer buffer, Array<Range> bounds, PrimExpr condition, Stmt body)
        : StmtNode(), buffer(buffer), bounds(bounds), condition(condition), body(body) {}

    static constexpr const char* _type_key = "tir.BufferRealize";
    TVM_DECLARE_FINAL_OBJECT_INFO(BufferRealizeNode, StmtNode);
};

/*!
 * \brief Managed reference to BufferRealizeNode.
 * \sa BufferRealizeNode
 */
class BufferRealize : public Stmt {
public:
    LITETVM_API explicit BufferRealize(Buffer buffer, Array<Range> bounds, PrimExpr condition, Stmt body);

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(BufferRealize, Stmt, BufferRealizeNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(BufferRealizeNode);
};

/*!
 * \brief Store value into mult-dimensional array that will be read by the consumer
 *        of the producer.
 *
 * \note This node only appears in high-level DSLs that are built on top of the TIR.
 *       It should not appear in a valid TIR PrimFunc. A high-level DSL needs to lower
 *       this node before TIR transformations.
 *
 * \sa DataProducer
 */
class ProducerStoreNode : public StmtNode {
public:
    /*! \brief The producer to store the results into. */
    DataProducer producer;
    /*! \brief The value to be stored. */
    PrimExpr value;
    /*! \brief The index arguments of the function. */
    Array<PrimExpr> indices;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("producer", &producer);
        v->Visit("value", &value);
        v->Visit("indices", &indices);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const ProducerStoreNode* other, SEqualReducer equal) const {
        return equal(producer, other->producer) && equal(value, other->value) &&
               equal(indices, other->indices);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(producer);
        hash_reduce(value);
        hash_reduce(indices);
    }

    static constexpr const char* _type_key = "tir.ProducerStore";
    TVM_DECLARE_FINAL_OBJECT_INFO(ProducerStoreNode, StmtNode);
};

/*!
 * \brief Managed reference to ProducerStoreNode.
 * \sa ProducerStoreNode
 */
class ProducerStore : public Stmt {
public:
    LITETVM_API ProducerStore(DataProducer producer, PrimExpr value, Array<PrimExpr> indices);

    TVM_DEFINE_OBJECT_REF_METHODS(ProducerStore, Stmt, ProducerStoreNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(ProducerStoreNode);
};

/*!
 * \brief Annotate the bounds where the data produced by the producer
 *  need to be written and read in body.
 *  We will need to allocate space for the corresponding regions.
 *
 * \note This node only appears in high-level DSLs that are built on top of the TIR.
 *       It should not appear in a valid TIR PrimFunc. A high-level DSL needs to lower
 *       this node before TIR transformations.
 *
 * \sa DataProducer
 */
class ProducerRealizeNode : public StmtNode {
public:
    /*! \brief The producer that produces the data. */
    DataProducer producer;
    /*! \brief Bounds to be realized. */
    Region bounds;
    /*! \brief Only realize if condition holds. */
    PrimExpr condition;
    /*! \brief The body of realization. */
    Stmt body;
    /*! \brief The storage scope associated with this realization. */
    String storage_scope;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("producer", &producer);
        v->Visit("bounds", &bounds);
        v->Visit("condition", &condition);
        v->Visit("body", &body);
        v->Visit("storage_scope", &storage_scope);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const ProducerRealizeNode* other, SEqualReducer equal) const {
        return equal(producer, other->producer) && equal(bounds, other->bounds) &&
               equal(condition, other->condition) && equal(body, other->body) &&
               equal(storage_scope, other->storage_scope);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(producer);
        hash_reduce(bounds);
        hash_reduce(condition);
        hash_reduce(body);
        hash_reduce(storage_scope);
    }

    static constexpr const char* _type_key = "tir.ProducerRealize";
    TVM_DECLARE_FINAL_OBJECT_INFO(ProducerRealizeNode, StmtNode);
};

/*!
 * \brief Managed reference to ProducerRealizeNode.
 * \sa ProducerRealizeNode
 */
class ProducerRealize : public Stmt {
public:
    LITETVM_API ProducerRealize(DataProducer producer, Region bounds, PrimExpr condition, Stmt body,
                                String storage_scope = "");

    TVM_DEFINE_OBJECT_REF_METHODS(ProducerRealize, Stmt, ProducerRealizeNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(ProducerRealizeNode);
};

/*!
 * \brief Allocate a buffer that can be used in body.
 */
class AllocateNode : public StmtNode {
public:
    /*! \brief The buffer variable. */
    Var buffer_var;
    /*! \brief The type of the buffer. */
    DataType dtype;
    /*! \brief The extents of the buffer. */
    Array<PrimExpr> extents;
    /*! \brief Only allocate buffer when condition is satisfied. */
    PrimExpr condition;
    /*! \brief The body to be executed. */
    Stmt body;
    /*!
   * \brief Additional annotations about the allocation.
   *
   *  These annotations can be used as auxiliary hint
   *  to future transformations.
   */
    Map<String, ObjectRef> annotations;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("buffer_var", &buffer_var);
        v->Visit("dtype", &dtype);
        v->Visit("extents", &extents);
        v->Visit("condition", &condition);
        v->Visit("body", &body);
        v->Visit("annotations", &annotations);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const AllocateNode* other, SEqualReducer equal) const {
        return equal.DefEqual(buffer_var, other->buffer_var) && equal(dtype, other->dtype) &&
               equal(extents, other->extents) && equal(condition, other->condition) &&
               equal(body, other->body) && equal(annotations, other->annotations);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce.DefHash(buffer_var);
        hash_reduce(dtype);
        hash_reduce(extents);
        hash_reduce(condition);
        hash_reduce(body);
        hash_reduce(annotations);
    }

    /*!
   * \brief If the buffer size is constant, return the size.
   *        Otherwise return 0.
   * \return The result.
   */
    int64_t ConstantAllocationSize() const { return ConstantAllocationSize(extents); }
    /*!
   * \brief If the buffer size is constant, return the size.
   *        Otherwise return 0.
   * \param extents The extents of the buffer.
   * \return The result.
   */
    LITETVM_API static int64_t ConstantAllocationSize(const Array<PrimExpr>& extents);

    static constexpr const char* _type_key = "tir.Allocate";
    static constexpr const bool _type_has_method_sequal_reduce = true;
    static constexpr const bool _type_has_method_shash_reduce = true;
    TVM_DECLARE_FINAL_OBJECT_INFO(AllocateNode, StmtNode);
};

/*!
 * \brief Managed reference to AllocateNode.
 * \sa AllocateNode
 */
class Allocate : public Stmt {
public:
    LITETVM_API Allocate(Var buffer_var, DataType dtype, Array<PrimExpr> extents, PrimExpr condition,
                         Stmt body, Map<String, ObjectRef> annotations = Map<String, ObjectRef>());

    TVM_DEFINE_OBJECT_REF_METHODS(Allocate, Stmt, AllocateNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(AllocateNode);
};

/*!
 * \brief The container of seq statement.
 *        Represent a sequence of statements.
 */
class SeqStmtNode : public StmtNode {
public:
    /*! \brief internal sequence content. */
    Array<Stmt> seq;

    /*! \return get the size of the sequence */
    size_t size() const { return seq.size(); }
    /*!
     * \brief Get the index-th element in the sequence.
     */
    Stmt operator[](size_t index) const { return seq[index]; }

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("seq", &seq);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const SeqStmtNode* other, SEqualReducer equal) const {
        return equal(seq, other->seq);
    }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce(seq); }

    static constexpr const char* _type_key = "tir.SeqStmt";
    TVM_DECLARE_FINAL_OBJECT_INFO(SeqStmtNode, StmtNode);
};


/*!
 * \brief Evaluates an expression.
 *  This is mostly used for putting a Call node into Stmt.
 *
 *  If value do not have side-effect, this node can be safely removed.
 */
class EvaluateNode : public StmtNode {
public:
    /*! \brief The expression to be evaluated. */
    PrimExpr value;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("value", &value);
        // v->Visit("span", &span);
    }

    bool SEqualReduce(const EvaluateNode* other, SEqualReducer equal) const {
        return equal(value, other->value);
    }

    void SHashReduce(SHashReducer hash_reduce) const { hash_reduce(value); }

    static constexpr const char* _type_key = "tir.Evaluate";
    TVM_DECLARE_FINAL_OBJECT_INFO(EvaluateNode, StmtNode);
};

/*!
 * \brief Managed reference to EvaluateNode.
 * \sa EvaluateNode
 */
class Evaluate : public Stmt {
public:
    LITETVM_API explicit Evaluate(PrimExpr value);

    explicit Evaluate(int value) : Evaluate(PrimExpr(value)) {}

    TVM_DEFINE_OBJECT_REF_METHODS(Evaluate, Stmt, EvaluateNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(EvaluateNode);
};

/*! \brief Sequence statement. */
class SeqStmt : public Stmt {
public:
    /*!
   * \brief Construct SeqStmt.
   * \param seq The sequence.
   * \param span The location of this object in the source code.
   */
    LITETVM_API explicit SeqStmt(Array<Stmt> seq);

    /*! \return get the size of the sequence */
    size_t size() const { return operator->()->size(); }
    /*!
   * \brief Get the index-th element in the sequence.
   */
    Stmt operator[](size_t index) const { return (*(operator->()))[index]; }
    /*!
   * \brief Construct a sequence statement by flattening
   *        all the arrays and sequences in the arguments
   *        recursively.
   *
   * - When an argument is nullptr, it will be ignored.
   * - When an argument is an array or a SeqStmt, it will be flattened recursively.
   * - A normal Stmt will be appended to the end of the sequence.
   *
   * \note This function can directly return an element
   *       if it is the only element in the sequence.
   *
   * \note If the only argument to this function is a SeqStmt, and if
   *       no flattening of the SeqStmt is required, then the SeqStmt
   *       will be returned as-is.
   *
   * \param seq_args The list of arguments to be flattened.
   * \tparam Args arguments
   * \return The constructed statement
   */
    template<typename... Args>
    static Stmt Flatten(Args&&... seq_args) {
        Array<Stmt> seq;
        runtime::detail::for_each(Flattener(&seq), std::forward<Args>(seq_args)...);

        if (seq.empty()) {
            return Evaluate(0);
        } else if (seq.size() == 1) {
            return seq[0];
        }

        // If the argument is a single SeqStmt argument with no
        // flattening or unwrapping required, then we may
        // return the SeqStmt as-is.
        if constexpr (sizeof...(seq_args) == 1) {
            if (auto opt = Flattener::AsSeqStmt(std::forward<Args>(seq_args)...)) {
                SeqStmt original = opt.value();
                bool all_same = [&]() {
                    if (original->seq.size() != seq.size()) {
                        return false;
                    }
                    for (size_t i = 0; i < seq.size(); i++) {
                        if (!original->seq[i].same_as(seq[i])) {
                            return false;
                        }
                    }
                    return true;
                }();
                if (all_same) {
                    return original;
                }
            }
        }

        return SeqStmt(seq);
    }
    /*! \brief Helper class to flatten sequence of arguments into Array. */
    class Flattener {
    public:
        explicit Flattener(Array<Stmt>* seq) : seq_(seq) {}

        template<typename T>
        static Optional<SeqStmt> AsSeqStmt(const T& t) {
            if constexpr (std::is_same_v<T, SeqStmt>) {
                return t;
            } else if constexpr (!std::is_base_of_v<T, SeqStmt>) {
                return NullOpt;
            } else if (auto* ptr = t.template as<SeqStmtNode>()) {
                return GetRef<SeqStmt>(ptr);
            } else {
                return NullOpt;
            }
        }

        template<typename T>
        void operator()(size_t i, const T& stmt_or_seq) const {
            if constexpr (std::is_base_of_v<ObjectRef, T>) {
                // Early bail-out, applicable to any ObjectRef
                if (!stmt_or_seq.defined()) {
                    return;
                }
            }

            if constexpr (std::is_same_v<T, SeqStmt>) {
                // Static type-checking for a SeqStmt that could be flattened.
                (*this)(0, stmt_or_seq->seq);
                return;
            }

            if constexpr (std::is_base_of_v<T, SeqStmt>) {
                // Dynamic type-checking for a SeqStmt that could be
                // flattened.
                if (auto* op = stmt_or_seq.template as<SeqStmtNode>()) {
                    operator()(0, op->seq);
                    return;
                }
            }

            if constexpr (std::is_base_of_v<T, Evaluate>) {
                // Evaluate(0) is used to represent a no-op, and may be
                // generated by previous calls to SeqStmt::Flatten().  These
                // should be removed to ensure that Flatten(a+b) is equivalent
                // to Flatten(Flatten(a), Flatten(b)).
                if (auto* op = stmt_or_seq.template as<EvaluateNode>()) {
                    if (auto* as_int = op->value.template as<IntImmNode>(); as_int && as_int->value == 0) {
                        return;
                    }
                }
            }

            if constexpr (std::is_base_of_v<Stmt, T>) {
                // Any other Stmt type just gets appended.
                seq_->push_back(stmt_or_seq);
            } else {
                // Anything else is treated as an iterable of Stmt.
                for (auto v: stmt_or_seq) {
                    this->operator()(0, v);
                }
            }
        }

    private:
        Array<Stmt>* seq_;
    };

    TVM_DEFINE_OBJECT_REF_METHODS(SeqStmt, Stmt, SeqStmtNode);
    TVM_DEFINE_OBJECT_REF_COW_METHOD(SeqStmtNode);
};

}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_STMT_H
