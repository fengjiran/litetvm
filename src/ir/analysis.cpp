//
// Created by 赵丹 on 25-4-18.
//

#include "ir/analysis.h"
#include "runtime/registry.h"
#include "support/ordered_set.h"

namespace litetvm {
namespace ir {

Map<GlobalVar, Array<GlobalVar>> CollectCallMap(const IRModule& mod) {
    struct CalleeCollectorImpl : CalleeCollector {
        void Mark(GlobalVar gvar) override { gvars.push_back(gvar); }
        support::OrderedSet<GlobalVar> gvars;
    };

    Map<GlobalVar, Array<GlobalVar>> call_map;
    for (const auto& [gvar, base_func]: mod->functions) {
        CalleeCollectorImpl collector;
        CalleeCollector::vtable()(base_func, &collector);
        call_map.Set(gvar, Array<GlobalVar>{collector.gvars.begin(), collector.gvars.end()});
    }
    return call_map;
}

TVM_REGISTER_GLOBAL("ir.analysis.CollectCallMap").set_body_typed(CollectCallMap);

}// namespace ir
}// namespace litetvm
