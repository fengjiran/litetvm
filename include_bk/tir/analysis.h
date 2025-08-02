//
// Created by 赵丹 on 25-4-15.
//

#ifndef LITETVM_TIR_ANALYSIS_H
#define LITETVM_TIR_ANALYSIS_H

#include "arith/analyzer.h"
#include "ir/module.h"
#include "ir/transform.h"
#include "target/target.h"
#include "tir/function.h"
#include "tir/op_attr_types.h"
#include "tir/stmt.h"

#include <optional>
#include <string>

namespace litetvm {

namespace arith {
class Analyzer;
}

namespace tir {

/*!
 * \brief Compare two expressions recursively and check if they are equal
 *        to each other without var remapping.
 *
 *  This function does not remap variable bindings, it will not
 *  return true for (let x = 1 in x + 1) vs (let y = 1 in y + 1), unless x.same_as(y).
 *
 *  Use StructuralEqual for such cases.
 *
 *  Due to the restriction of not remapping variables, this function can run
 *  faster than StructuralEqual and can be used as a utility function during arithmetic
 *  simplifications.
 *
 * \sa StructuralEqual
 */
struct ExprDeepEqual {
    LITETVM_API bool operator()(const PrimExpr& lhs, const PrimExpr& rhs) const;
};

/*!
 * \brief Visit the PrimFuncs in the IRModule
 * \tparam FLambda The type of the PrimFunc visitor
 * \param mod The IRModule to be visited
 * \param fvisit The visitor to the PrimFuncs in the IRModule
 */
template<class FLambda>
void VisitPrimFuncs(const IRModule& mod, FLambda fvisit) {
    for (const auto& kv: mod->functions) {
        const BaseFunc& base_func = kv.second;
        if (const auto* prim_func = base_func.as<PrimFuncNode>()) {
            fvisit(prim_func);
        }
    }
}

/*!
 * \brief Estimate the FLOPs of a TIR fragment.
 * \param stmt The TIR fragment to be estimated.
 * \return The estimated FLOPs.
 */
 LITETVM_API double EstimateTIRFlops(const Stmt& stmt);

/*!
 * \brief Estimate the FLOPs of TIRs in an IRModule.
 * \param mod The IRModule to be estimated.
 * \return The estimated FLOPs.
 */
 LITETVM_API double EstimateTIRFlops(const IRModule& mod);

/*!
 * \brief Find undefined vars in the statement.
 * \param stmt The statement to be checked.
 * \param defs The vars that is defined.
 * \return Array of undefined vars.
 */
 LITETVM_API Array<Var> UndefinedVars(const Stmt& stmt, const Array<Var>& defs);

/*!
 * \brief Find undefined vars in the expression.
 * \param expr The expression to be checked.
 * \return Array of undefined vars.
 */
 LITETVM_API Array<Var> UndefinedVars(const PrimExpr& expr);

/*!
 * \brief Find undefined vars in the expression.
 * \param expr The expression to be checked.
 * \param defs The vars that is defined.
 * \return Array of undefined vars.
 */
 LITETVM_API Array<Var> UndefinedVars(const PrimExpr& expr, const Array<Var>& defs);

/*!
 * \brief Analyze the side effect of an expression
 * \param expr The expression to be checked.
 *
 * \return CallEffectKind, can be kPure, kReadState or kUpdateState
 */
 LITETVM_API CallEffectKind SideEffect(const PrimExpr& expr);

/*!
 * \brief Analyze the side effect of a function
 *
 * \param func The expression to be checked.
 *
 * \param assert_on_error If true, an error will be thrown for an
 *    impure function.  If false (default), the purity of the PrimFunc
 *    will be returned.
 *
 * \return The purity of the function
 */
 LITETVM_API bool IsPureFunction(const PrimFunc& func, bool assert_on_error = false);

/*!
 * \brief Whether the given Stmt uses any var in the given variable set.
 * \param stmt The Stmt to be checked.
 * \param vset_contains The check function to see if a var is in the variable set.
 * \return Whether `stmt` uses any var in the given variable set.
 */
 LITETVM_API bool UsesVar(const Stmt& stmt, std::function<bool(const VarNode*)> vset_contains);

/*!
 * \brief Whether the given PrimExpr uses any var in the given variable set.
 * \param expr The PrimExpr to be checked.
 * \param vset_contains The check function to see if var is in the variable set.
 * \return Whether `expr` uses any var in the given variable set.
 */
 LITETVM_API bool UsesVar(const PrimExpr& expr, std::function<bool(const VarNode*)> vset_contains);

/*!
 * \brief Verifies whether the IR stmt or Expr is in SSA form.
 *  That is: each Var is defined and assigned once(in Let/For)
 *
 * \param func The function to be verified.
 * \return Whether IR is in SSA form.
 *
 * \note All passes in TIR consume and produce SSA form.
 */
 LITETVM_API bool VerifySSA(const PrimFunc& func);

/*!
 * \brief Verify if memory accesses are legal for a specific target device type.
 *
 *  In the case that tgt is cuda, if not all workload is bound with
 *  threads, CPU code is generated that tries to access GPU memory,
 *  which is illegal. This pass performs verification for this case.
 *
 * \param func The function to be verified.
 * \return Success of memory verification.
 */
 LITETVM_API bool VerifyMemory(const PrimFunc& func);

/*!
 * \brief Verify the correctness of a GPU code
 *        It will check the whether the amount of memory usage or the number of threads
 *        in a block exceeds the limit
 * \param func The function to be checked
 * \param constraints The dict to specify constraints to check.
 *        Possible keys are
 *
 *        "max_local_memory_per_block": Total amount of local memory per block (in bytes).
 *        "max_shared_memory_per_block": Total amount of shared memory per block (in bytes).
 *        "max_threads_per_block": Maximum number of threads per block.
 *        "max_thread_x": Maximum length of threadIdx.x.
 *        "max_thread_y": Maximum length of threadIdx.y.
 *        "max_thread_z": Maximum length of threadIdx.z.
 *
 *        If one key is missing in this argument, the pass won't check for that item.
 * \return valid Whether it is a valid GPU code
 *
 */
 LITETVM_API bool VerifyGPUCode(const PrimFunc& func, Map<String, PrimExpr> constraints);

/**
 * @brief Utility function to get the list of lowering passes to be applied to calculate the
 * compacted VTCM allocation size
 *
 * @return returns list of passes
 */
 LITETVM_API Array<transform::Pass> GetVTCMCompactionPasses();

/*!
 * \brief Verifies that the VTCM usage for all prim_funcs in the given IRModule
 * \param mod The module to be checked
 * \param limit The limit to check.
 * \return true if the VTCM usage is within the provided limit.
 */
 LITETVM_API bool VerifyVTCMLimit(const IRModule& mod, Integer limit);

/*!
 * \brief Verifies that the VTCM usage of the given prim_func is within the provided limit.
 * \param func The function to be checked.
 * \param limit The limit to check.
 * \return true if the VTCM usage is within the provided limit.
 */
 LITETVM_API bool VerifyVTCMLimit(const PrimFunc& func, Integer limit);

/*!
 * \brief Auto detect the block access region according to its body stmt
 *        It will detect the access region as an array in order of appearance in AST
 * \param block The block to be detected
 * \param buffer_var_map The outside buffers which may be accessed the block.
 *                       It is a map from buffer var to the buffer.
 * \return Array of access regions.
 *         There are three arrays of BufferRegion:
 *           - first: read regions
 *           - second: write regions
 *           - third: opaque regions
 */
 LITETVM_API Array<Array<BufferRegion>> GetBlockAccessRegion(const Block& block,
                                                        const Map<Var, Buffer>& buffer_var_map);

/*!
 * \brief Auto detect the block read/write region according to its body stmt. An opaque access will
 *        be counted as both a read and a write access
 * \param block The block to be detected
 * \param buffer_var_map The outside buffers which may be accessed the block.
 *                       It is a map from buffer var to the buffer
 * \return An array only consisting of the read regions and write regions of the input block
 */
 LITETVM_API Array<Array<BufferRegion>> GetBlockReadWriteRegion(const Block& block,
                                                           const Map<Var, Buffer>& buffer_var_map);

/*! \brief Helper struct for return value of IdentifyMemCpy
 *
 * This helper struct is not strictly necessary, as `IdentifyMemCpy`
 * could instead return a `std::pair<BufferRegion, BufferRegion>`.
 * However, that would introduce ambiguity between the two unnamed
 * regions.
 */
struct MemCpyDetails {
    BufferRegion source;
    BufferRegion dest;
};

/*! \brief Identify whether a For loop is semantically equivalent to MemCpy
 *
 * \param loop The loop to be checked
 *
 * \param analyzer The analyzer with which to check any algebraic expressions
 *
 * \returns The source and destination regions being copied, if the
 * loop is equivalent to memcpy.  Otherwise, returns nullopt.
 */
 LITETVM_API std::optional<MemCpyDetails> IdentifyMemCpy(const For& loop, arith::Analyzer* analyzer);

/*!
 * \brief Calculate the expression complexity based on number of symbols it contains.
 * \param expr The expr to be calculated.
 */
 LITETVM_API size_t CalculateExprComplexity(const PrimExpr& expr);

/*!
 * \brief Calculate the constants size in bytes needed by the TIR allocates inside the TIR PrimFunc
 * \param func The TIR PrimFunc for which the constants size to be calculated
 * \param constant_byte_alignment The byte alignment required for each constant allocated
 */
 LITETVM_API size_t CalculateConstantBytes(const PrimFunc& func, const Integer& constant_byte_alignment);

/*!
 * \brief Calculate the workspace size in bytes needed by the TIR allocates inside the TIR PrimFunc
 * \param func The TIR PrimFunc for which the workspace size to be calculated
 * \param workspace_byte_alignment The byte alignment required for each tensor allocated in this
 * workspace
 */
 LITETVM_API size_t CalculateWorkspaceBytes(const PrimFunc& func,
                                       const Integer& workspace_byte_alignment);

/*!
 * \brief Calculate the allocated memory per scope in bytes needed inside the TIR PrimFunc
 * \param func The TIR PrimFunc for which the allocated memory size to be calculated
 * \return Allocated memory size per scope in bytes inside the PrimFunc returned as a Map with
 * key "main" and a Map of allocated sizes as values.
 */
 LITETVM_API Map<String, Map<String, Integer>> CalculateAllocatedBytes(const PrimFunc& func);

/*!
 * \brief Calculate the allocated memory per scope in bytes for each function inside the module
 * \param mod The IRModule for which the allocated memory size has to be calculated
 * \return Allocated memory size per scope in bytes for each function in the IRModule returned as a
           Map with function names as keys and a Map of allocated sizes as values.
 */
 LITETVM_API Map<String, Map<String, Integer>> CalculateAllocatedBytes(const IRModule& mod);

/*!
 * \brief Detect the lowest common ancestor(LCA) of buffer access, including both high-level
 *        access(BufferLoad, BufferStore) and low-level access(Load, Store and opaque access).
 *        The LCA may be a For loop or a Block.
 * \param func The PrimFunc to be detected.
 * \return The Map from buffer to the LCA of all access to it. The lca is function root if the
 *         return stmt is NullOpt.
 */
 LITETVM_API Map<Buffer, Optional<Stmt>> DetectBufferAccessLCA(const PrimFunc& func);

/*!
 * \brief Verify if the given TIR is well-formed. The verification includes:
 *
 * - All variables are defined prior to their point of use.
 *
 * - No variables are used outside of the scope of their definition.
 *
 * - Each variable has a single point of definition.
 *
 * - Expressions within a tir::Block may not reference variables
 *   defined outside the block.  For example, for a block with iter
 *   vars `vi, vj = T.axis.remap('SS', [i,j])`, the statement
 *   `B[i,j] = A[i,j]` would be ill-formed, because it uses the loop
 *   variables `i` and `j` instead of the block variables `vi` and
 *   `vj`.
 *
 * \param func The PrimFunc to be verified.
 * \param assert_mode The indicator if it raises an error when the function is not well-formed.
 * \return Whether it is a well-formed TIR function.
 */
 LITETVM_API bool VerifyWellFormed(const PrimFunc& func, bool assert_mode = true);

/*!
 * \brief Verify if the TIR in the given IRMOdule is well-formed.
 *
 * In addition to the checks performed for each PrimFunc (see above),
 * the following checks are performed:
 *
 * - The same TIR variable may not be defined in more than one function
 *
 * \param mod The IRModule to be verified.
 * \param assert_mode The indicator if it raises an error when the function is not well-formed.
 * \return Whether it is a well-formed TIR module.
 */
 LITETVM_API bool VerifyWellFormed(const IRModule& mod, bool assert_mode = true);

/*!
 * \brief Find the entry function of the given IRModule, i.e, functions marked by
 * `tir::attr::kIsEntryFunc`, whose name is `main` or being the only PrimeFunc.
 * \param mod The IRModule to find the entry function.
 * \param result_g_var The result GlobalVar of the entry function.
 * \return The entry function.
 */
const PrimFuncNode* FindEntryFunc(const IRModule& mod, GlobalVar* result_g_var);

/*!
 * \brief Find the "anchor block" of the given module.
 * We define the anchor block to be the block with (1) an init statement and (2) having
 * the biggest flops count. The latter condition is only used when there are multiple blocks
 * with an init statement.
 * For example, if the input module is conv2d + fused spatial blocks, conv2d is the anchor block.
 * The input module may not contain more than one such block. For example, a module having
 * two conv2d is not allowed as an input.
 * However, a module created from winograd convolution has multiple blocks with an init statement
 * (input transform, batched GEMM, and output transform). We use the second condition, the flops
 * count, to determine that the batched GEMM block is the anchor block.
 * \param mod The input TIR module.
 * \return The anchor block if found, nullptr otherwise.
 */
const tir::BlockNode* FindAnchorBlock(const IRModule& mod);

// Pass variants of verification analysis
// directly throws RuntimeError when verification fails.
namespace transform {

using litetvm::transform::Pass;
using litetvm::transform::PassContext;

/*!
 * \brief Pass variant of VerifySSA.
 *
 * \returns The pass.
 * \sa tvm::tir::VerifySSA
 */
 LITETVM_API Pass VerifySSA();

/*!
 * \brief Pass variant of VerifyMemory.
 *
 * \returns The pass.
 * \sa tvm::tir::VerifyMemory
 */
 LITETVM_API Pass VerifyMemory();

/*!
 * \brief Pass variant of VerifyGPUCode.
 *
 * \param constraints The dict to specify constraints to check.
 *
 * \returns The pass.
 * \sa tvm::tir::VerifyGPUCode
 */
 LITETVM_API Pass VerifyGPUCode(Map<String, PrimExpr> constraints);

/*!
 * \brief Pass to checks if the size of the allocated vtcm memory satisfies the limit
 *
 * \param target The target whose VTCM limit should be used for any
 * functions not already annotated with `tvm::attr::kTarget`.
 *
 * \returns The pass.
 * \sa tvm::tir::CalculateAllocatedBytes
 */
 LITETVM_API Pass VerifyVTCMLimit(Optional<Target> target = NullOpt);

/*!
 * \brief Statically check TIR code for out of bounds array access.
 *
 * This analysis is conservative: it will only raise errors if it can prove
 * that out of bounds access occurs. Cases that are uncertain do not raise
 * errors.
 *
 * \returns The pass.
 */
 LITETVM_API Pass OOBChecker();

}// namespace transform

}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_ANALYSIS_H
