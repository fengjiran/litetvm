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
