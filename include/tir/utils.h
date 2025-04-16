//
// Created by 赵丹 on 25-4-11.
//

#ifndef LITETVM_TIR_UTILS_H
#define LITETVM_TIR_UTILS_H

#include "tir/block_scope.h"
#include "tir/expr.h"
#include "tir/stmt.h"

#include <unordered_map>

namespace litetvm {
namespace tir {

/*!
 * \brief A helper macro to convert an sref to the statement it points to,
 * then check if the downcasting succeeded.
 * \param Result The result variable, used for checking
 * \param SRef The SRef to be cast
 * \param Type The type to be cast to, can be Block or For
 */
#define TVM_SREF_AS_OR_ERR(Result, SRef, Type) \
    SRef->StmtAs<Type>();                      \
    CHECK(Result)

/*!
 * \brief A helper macro to convert an sref to the block it points to,
 *
 * Throws an internal error if downcasting fails.  The variable name
 * in the parent scope is used for the error message.
 *
 * \param SRef The SRef to be cast
 */
#define TVM_SREF_TO_BLOCK(SRef)                                                                        \
    [&]() {                                                                                            \
        auto result = TVM_SREF_AS_OR_ERR(result, (SRef), ::litetvm::tir::BlockNode)                    \
                      << "TypeError: Expects StmtSRef `" << #SRef << "` points to `Block`, but gets: " \
                      << ((SRef)->stmt ? (SRef)->stmt->GetTypeKey() : "None");                         \
        return result;                                                                                 \
    }()

/*!
 * \brief A helper macro to convert an sref to the for-loop it points to
 *
 * Throws an internal error if downcasting fails.  The variable name
 * in the parent scope is used for the error message.
 *
 * \param SRef The SRef to be cast
 */
#define TVM_SREF_TO_FOR(SRef)                                                                         \
    [&]() {                                                                                           \
        auto result = TVM_SREF_AS_OR_ERR(result, (SRef), ::litetvm::tir::ForNode)                     \
                      << "TypeError: Expects StmtSRef `" << #SRef << "` points to `Loop`, but gets: " \
                      << ((SRef)->stmt ? (SRef)->stmt->GetTypeKey() : "None");                        \
        return result;                                                                                \
    }()

/*!
 * \brief Downcast a TVM ObjectRef to its corresponding container using `ObjectRef::as<Type>`,
 * then check if the downcasting succeeded.
 * \param Result The result variable, used for checking
 * \param From The ObjectRef to be downcast
 * \param Type The type to be downcast to
 */
#define TVM_TYPE_AS_OR_ERR(Result, From, Type) \
    From.as<Type>();                           \
    CHECK(Result)

/*!
 * \brief Downcast a TVM ObjectRef to its corresponding container using `ObjectRef::as<Type>`,
 * throwing an internal error if downcast fails.
 * \param From The ObjectRef to be downcast
 * \param Type The type to be downcast to
 */
#define TVM_TYPE_AS(From, Type)                                                                   \
    [&]() {                                                                                       \
        auto result = TVM_TYPE_AS_OR_ERR(result, (From), Type)                                    \
                      << "TypeError: Expects `" << #From << "` to have type `" << Type::_type_key \
                      << "`, but gets: " << ((From).defined() ? (From)->GetTypeKey() : "None");   \
        return result;                                                                            \
    }()

/*!
 * \brief Set the `StmtSRefNode::seq_index` field for stmt
 * \param stmt2ref The stmt2ref map to be updated with seq_index
 * \param stmt The statement, or the realize node of the statement whose sref to be set
 * \param seq_index The seq_index to be set
 * \param include_loops Ignore ForNodes if this value is false
 * \note The method is NOP for statements that are not schedulable, i.e. not For or Block
 */
inline void SetSeqIndex(std::unordered_map<const StmtNode*, StmtSRef>& stmt2ref,// NOLINT(*)
                        const Stmt& stmt, int seq_index, bool include_loops = true) {
    if (const auto* realize = stmt.as<BlockRealizeNode>()) {
        const BlockNode* block = realize->block.get();
        CHECK(stmt2ref.count(block));
        stmt2ref.at(block)->seq_index = seq_index;
    } else if (const auto* block = stmt.as<BlockNode>()) {
        CHECK(stmt2ref.count(block));
        stmt2ref.at(block)->seq_index = seq_index;
    } else if (const auto* loop = stmt.as<ForNode>()) {
        if (!include_loops) return;
        CHECK(stmt2ref.count(loop));
        stmt2ref.at(loop)->seq_index = seq_index;
    }
}

/*!
 * \brief Update seq_index of the children of a SeqStmt
 * \param stmt2ref The stmt2ref map to be updated with indices
 * \param seq_stmt The SeqStmt whose children need updating
 * \param include_loops Ignore ForNodes if this value is false
 */
inline void SetSeqIndexInChildren(
        std::unordered_map<const StmtNode*, StmtSRef>& stmt2ref,// NOLINT(*)
        const SeqStmtNode* seq_stmt, bool include_loops = true) {
    int i = 0;
    for (const Stmt& stmt: seq_stmt->seq) {
        SetSeqIndex(stmt2ref, stmt, i, include_loops);
        ++i;
    }
}

/* \brief Normalize an ObjectRef held
 *
 * Where possible, the IR should be normalized contain IR types.  For
 * example, holding a `tir::IntImm` instead of a `runtime::Int`.  In
 * attributes, this is not always possible, as attributes may refer to
 * non-IR objects.
 *
 * This function normalizes any `runtime::Int`, `runtime::Bool`,
 * `runtime::Float`, or containers of those types to the corresponding
 * IR type.
 *
 * \param obj The attribute object to be normalized
 *
 * \returns The normalized attribute
 */
ObjectRef NormalizeAttributeObject(ObjectRef obj);

}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_UTILS_H
