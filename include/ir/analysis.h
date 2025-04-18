//
// Created by 赵丹 on 25-4-18.
//

#ifndef LITETVM_IR_ANALYSIS_H
#define LITETVM_IR_ANALYSIS_H

#include "ir/expr.h"
#include "ir/module.h"
#include "node/functor.h"
#include "runtime/array.h"

namespace litetvm {
namespace ir {

class CalleeCollector {
public:
    /* \brief Functor to be registered for IR types
     *
     * Should be implemented for each `BaseFunc` subclass.
     * Implementation should call `CalleeCollector::Mark` for each
     * `GlobalVar` in the function.
     */
    using FType = NodeFunctor<void(const ObjectRef&, CalleeCollector*)>;
    LITETVM_API static FType& vtable() {
        static FType inst;
        return inst;
    }

    virtual ~CalleeCollector() = default;

    /* \brief Collect the GlobalVar in a function */
    virtual void Mark(GlobalVar gvar) = 0;
};

Map<GlobalVar, Array<GlobalVar>> CollectCallMap(const IRModule& mod);

}// namespace ir
}// namespace litetvm

#endif//LITETVM_IR_ANALYSIS_H
