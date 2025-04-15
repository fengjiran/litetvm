//
// Created by 赵丹 on 25-4-15.
//

#ifndef LITETVM_ARITH_PRODUCT_NORMAL_FORM_H
#define LITETVM_ARITH_PRODUCT_NORMAL_FORM_H

#include "tir/expr.h"
#include "tir/op.h"

namespace litetvm {
namespace arith {

/*!
 * \brief Unpack reduction by calling each leaf via fleaf
 *
 * \param value The expression value.
 * \tparam TNode the reduction node to match.
 * \tparam FLeaf The callback function at leaf.
 */
template<typename TNode, typename FLeaf>
void UnpackReduction(const PrimExpr& value, FLeaf fleaf) {
    if (const TNode* node = value.as<TNode>()) {
        UnpackReduction<TNode, FLeaf>(node->a, fleaf);
        UnpackReduction<TNode, FLeaf>(node->b, fleaf);
    } else {
        fleaf(value);
    }
}

/**
 * \brief Unpack chain of add sub by calling each leaf via fleaf
 * \param value The expression value.
 * \tparam FLeaf The callback function at leaf.
 */
template<typename FLeaf>
void UnpackSum(const PrimExpr& value, FLeaf fleaf, int sign = 1) {
    if (const tir::AddNode* node = value.as<tir::AddNode>()) {
        UnpackSum(node->a, fleaf, sign);
        UnpackSum(node->b, fleaf, sign);
    } else if (const tir::SubNode* node = value.as<tir::SubNode>()) {
        UnpackSum(node->a, fleaf, sign);
        UnpackSum(node->b, fleaf, -sign);
    } else {
        fleaf(value, sign);
    }
}

/*!
 * \brief Helper function to multiply extent and re-normalize.
 *
 * Multiply extent scale and re-normalize to form (x * y) * z
 *
 * NOTE on multiplication order: when have shape (s[0], s[1], s[2]),
 * we prefer to multiple in order of s[0] * s[1] * s[2]

 * \param lhs The lhs iterator
 * \param rhs The rhs iterator
 * \return the result.
 */
inline PrimExpr MulAndNormalize(const PrimExpr& lhs, const PrimExpr& rhs) {
    int64_t cscale = 1;
    PrimExpr res = tir::make_const(lhs.dtype(), 1);
    auto fcollect = [&](PrimExpr val) {
        if (const auto* intimm = val.as<IntImmNode>()) {
            cscale *= intimm->value;
        } else {
            res = res * val;
        }
    };
    UnpackReduction<tir::MulNode>(lhs, fcollect);
    UnpackReduction<tir::MulNode>(rhs, fcollect);
    if (cscale != 1) {
        res = res * tir::make_const(res.dtype(), cscale);
    }
    return res;
}

}// namespace arith
}// namespace litetvm


#endif//LITETVM_ARITH_PRODUCT_NORMAL_FORM_H
