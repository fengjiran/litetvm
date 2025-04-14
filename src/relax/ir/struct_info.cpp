//
// Created by 赵丹 on 25-4-14.
//

#include "relax/struct_info.h"
#include "runtime/registry.h"

namespace litetvm {
namespace relax {

ObjectStructInfo::ObjectStructInfo() {
    ObjectPtr<ObjectStructInfoNode> n = make_object<ObjectStructInfoNode>();
    // n->span = span;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(ObjectStructInfoNode);

TVM_REGISTER_GLOBAL("relax.ObjectStructInfo").set_body_typed([]() {
    return ObjectStructInfo();
});

}// namespace relax
}// namespace litetvm
