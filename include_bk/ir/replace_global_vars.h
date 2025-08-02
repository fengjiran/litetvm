//
// Created by 赵丹 on 25-4-18.
//

#ifndef LITETVM_IR_REPLACE_GLOBAL_VARS_H
#define LITETVM_IR_REPLACE_GLOBAL_VARS_H

#include "ir/module.h"

namespace litetvm {
namespace transform {

/*!
 * \brief Replace GlobalVar instances across any IR type.
 *
 * \param mod The module to update
 *
 * \param replacements The map, where each entry maps from an old
 * `GlobalVar` to the new `GlobalVar` that should replace it.
 *
 * \return The updated IRModule
 */
LITETVM_API IRModule ReplaceGlobalVars(IRModule mod, Map<GlobalVar, GlobalVar> replacements);

struct GlobalVarReplacer {
    using FType = NodeFunctor<BaseFunc(const ObjectRef&, Map<GlobalVar, GlobalVar>)>;
    LITETVM_API static FType& vtable() {
        static FType inst;
        return inst;
    }
};

}// namespace transform
}// namespace litetvm

#endif//LITETVM_IR_REPLACE_GLOBAL_VARS_H
