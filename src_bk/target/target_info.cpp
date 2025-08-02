//
// Created by 赵丹 on 25-3-14.
//

#include "target/target_info.h"
#include "node/repr_printer.h"
#include "runtime/registry.h"
#include "node/attr_registry.h"

namespace litetvm {

TVM_REGISTER_NODE_TYPE(MemoryInfoNode);

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<MemoryInfoNode>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const MemoryInfoNode*>(node.get());
            p->stream << "mem-info("
                      << "unit_bits=" << op->unit_bits << ", "
                      << "max_num_bits=" << op->max_num_bits << ", "
                      << "max_simd_bits=" << op->max_simd_bits << ", "
                      << "head_address=" << op->head_address << ")";
        });

MemoryInfo GetMemoryInfo(const std::string& scope) {
    std::string fname = "tvm.info.mem." + scope;
    const runtime::PackedFunc* f = runtime::RegistryManager::Global().Get(fname);
    if (f == nullptr) {
        LOG(WARNING) << "MemoryInfo for scope = " << scope << " is undefined";
        return {};
    }
    return (*f)();
}

}// namespace litetvm
