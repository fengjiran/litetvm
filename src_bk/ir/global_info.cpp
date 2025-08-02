//
// Created by 赵丹 on 25-4-15.
//

#include "ir/global_info.h"
#include "runtime/registry.h"

namespace litetvm {

TVM_REGISTER_NODE_TYPE(DummyGlobalInfoNode);
TVM_REGISTER_GLOBAL("ir.DummyGlobalInfo").set_body_typed([]() {
    auto n = DummyGlobalInfo(make_object<DummyGlobalInfoNode>());
    return n;
});

VDevice::VDevice(Target tgt, int dev_id, MemoryScope mem_scope) {
    ObjectPtr<VDeviceNode> n = make_object<VDeviceNode>();
    n->target = std::move(tgt);
    n->vdevice_id = std::move(dev_id);
    n->memory_scope = std::move(mem_scope);
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(VDeviceNode);
TVM_REGISTER_GLOBAL("ir.VDevice").set_body_typed([](Target tgt, int dev_id, MemoryScope mem_scope) {
    return VDevice(tgt, dev_id, mem_scope);
});

}// namespace litetvm
