//
// Created by 赵丹 on 25-4-1.
//

#ifndef LITETVM_TIR_BUILTIN_H
#define LITETVM_TIR_BUILTIN_H

#include "ir/op.h"
#include "tir/expr.h"

namespace litetvm {
namespace tir {

/*! \brief Collection of builtin intrinsics as ops */
namespace builtin {

/*!
 * \brief Return value.
 */
LITETVM_API const Op& ret();


}// namespace builtin
}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_BUILTIN_H
