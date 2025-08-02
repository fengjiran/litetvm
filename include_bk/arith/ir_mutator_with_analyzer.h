//
// Created by 赵丹 on 25-4-15.
//

#ifndef LITETVM_ARITH_IR_MUTATOR_WITH_ANALYZER_H
#define LITETVM_ARITH_IR_MUTATOR_WITH_ANALYZER_H

#include "arith/analyzer.h"
#include "tir/analysis.h"
#include "tir/stmt_functor.h"

namespace litetvm {
namespace arith {

/*!
 * \brief IRMutator with an analyzer context.
 *
 * This class can sub-classed by ir mutators that need an analyzer.
 * It will populates scope-related info such as bounds of loop-variables and constraints
 * for the analyzer, so that the child class can do accurate context-dependent analysis.
 *
 * \sa src/arithmetic/ir_mutator_with_analyzer.cc
 */
class IRMutatorWithAnalyzer : public tir::StmtExprMutator {
public:
    explicit IRMutatorWithAnalyzer(Analyzer* analyzer) : analyzer_(analyzer) {}

    using StmtExprMutator::VisitExpr_;
    using StmtExprMutator::VisitStmt_;

    // override functions that need to populate the context information.
    tir::Stmt VisitStmt_(const tir::ForNode* op) override;
    tir::Stmt VisitStmt_(const tir::BlockNode* op) override;
    tir::Stmt VisitStmt_(const tir::LetStmtNode* op) override;
    tir::Stmt VisitStmt_(const tir::IfThenElseNode* op) override;
    tir::Stmt VisitStmt_(const tir::AttrStmtNode* op) override;
    tir::Stmt VisitStmt_(const tir::AssertStmtNode* op) override;
    PrimExpr VisitExpr_(const tir::LetNode* op) override;
    PrimExpr VisitExpr_(const tir::SelectNode* op) override;
    PrimExpr VisitExpr_(const tir::CallNode* op) override;
    PrimExpr VisitExpr_(const tir::ReduceNode* op) override;

protected:
    /*!
   * \brief Mark the all the buffer shape values in the buffer map as positive value.
   *
   * \note call this function before Visit function's body to maximize
   *       simplification efficiency
   */
    void MarkBufferMapShapes(const tir::PrimFunc& func);

    /*!
   * \brief Use internal bound information to perform inter map simplification of indices.
   * \note Only do this during layout remapping
   */
    Array<PrimExpr> IterMapSimplifyWithContext(const Array<PrimExpr>& indices, bool non_trivial_only);

    /*! \brief internal analyzer field. */
    Analyzer* analyzer_;
    // the following two fields are useful in case we want
    // note however that iter map analysis are usually more
    // expensive and we only encourage doing them during
    // necessary cases like layout remapping
    /*! \brief Recorded loop iterators */
    Map<Var, Range> iter_vars_;
    /*! \brief iterator predicates */
    Array<PrimExpr> iter_predicates_;
    /*!
   * \brief Run callback while trying to record iter predicate
   * \param conditon Condition to be checked.
   * \param callback The callback to be called.
   */
    template<typename FLambda>
    void WithRecordIterPredicate(PrimExpr condition, FLambda callback) {
        auto f_use_itervar = [this](const tir::VarNode* v) {
            return iter_vars_.count(GetRef<tir::Var>(v));
        };
        // simple heuristics for detecting predicate
        if (tir::UsesVar(condition, f_use_itervar)) {
            iter_predicates_.push_back(condition);
            callback();
            iter_predicates_.pop_back();
        } else {
            callback();
        }
    }
};

}// namespace arith
}// namespace litetvm

#endif//LITETVM_ARITH_IR_MUTATOR_WITH_ANALYZER_H
