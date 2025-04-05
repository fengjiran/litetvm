//
// Created by richard on 4/5/25.
//

#ifndef LITETVM_ARITH_INT_SET_H
#define LITETVM_ARITH_INT_SET_H

#include "ir/expr.h"
#include "tir/expr.h"

#include <unordered_map>

namespace litetvm {
namespace arith {

using tir::IterVar;
using tir::Var;
using tir::VarNode;

class Analyzer;

//-----------------------------------------------
// Integer set data structure.
//
// This is a API build on top of the base
// integer analysis API to provide set analysis.
//------------------------------------------------
/*!
 * \brief Sign type of an integer expression.
 */
enum class SignType {
    kPositive,
    kNegative,
    kZero,
    kUnknown
};

/*!
 * \brief Base class of all Integer set containers.
 *        represent a set of integers in one dimension.
 * \sa IntSet
 */
class IntSetNode : public Object {
public:
    static constexpr const char* _type_key = "IntSet";
    static constexpr bool _type_has_method_sequal_reduce = false;
    TVM_DECLARE_BASE_OBJECT_INFO(IntSetNode, Object);
};

}// namespace arith
}// namespace litetvm


#endif//LITETVM_ARITH_INT_SET_H
