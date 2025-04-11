//
// Created by 赵丹 on 25-4-11.
//

#ifndef LITETVM_TIR_UTILS_H
#define LITETVM_TIR_UTILS_H

#include "tir/expr.h"

namespace litetvm {
namespace tir {

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
