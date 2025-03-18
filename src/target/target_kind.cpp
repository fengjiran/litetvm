//
// Created by 赵丹 on 25-3-17.
//

#include "target/target_kind.h"
#include "ir/expr.h"
#include "node/attr_registry.h"
#include "node/reflection.h"
#include "node/repr_printer.h"
#include "runtime/device_api.h"
#include "runtime/registry.h"
#include "support/utils.h"

#include <algorithm>

namespace litetvm {

// helper to get internal dev function in objectref.
struct TargetKind2ObjectPtr : public ObjectRef {
    static ObjectPtr<Object> Get(const TargetKind& kind) { return GetDataPtr<Object>(kind); }
};

TVM_REGISTER_NODE_TYPE(TargetKindNode)
        .set_creator([](const std::string& name) {
            auto kind = TargetKind::Get(name);
            CHECK(kind.defined()) << "Cannot find target kind \'" << name << '\'';
            return TargetKind2ObjectPtr::Get(kind.value());
        })
        .set_repr_bytes([](const Object* n) -> std::string {
            return static_cast<const TargetKindNode*>(n)->name;
        });

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<TargetKindNode>([](const ObjectRef& obj, ReprPrinter* p) {
            const TargetKind& kind = Downcast<TargetKind>(obj);
            p->stream << kind->name;
        });

/**********  Registry-related code  **********/

using TargetKindRegistry = AttrRegistry<TargetKindRegEntry, TargetKind>;

Array<String> TargetKindRegEntry::ListTargetKinds() {
    return TargetKindRegistry::Global()->ListAllNames();
}

Map<String, String> TargetKindRegEntry::ListTargetKindOptions(const TargetKind& target_kind) {
    Map<String, String> options;
    for (const auto& kv: target_kind->key2vtype_) {
        options.Set(kv.first, kv.second.type_key);
    }
    return options;
}

TargetKindRegEntry& TargetKindRegEntry::RegisterOrGet(const String& target_kind_name) {
    return TargetKindRegistry::Global()->RegisterOrGet(target_kind_name);
}

void TargetKindRegEntry::UpdateAttr(const String& key, TVMRetValue value, int plevel) const {
    TargetKindRegistry::Global()->UpdateAttr(key, kind_, value, plevel);
}

const AttrRegistryMapContainerMap<TargetKind>& TargetKind::GetAttrMapContainer(
        const String& attr_name) {
    return TargetKindRegistry::Global()->GetAttrMap(attr_name);
}

Optional<TargetKind> TargetKind::Get(const String& target_kind_name) {
    const TargetKindRegEntry* reg = TargetKindRegistry::Global()->Get(target_kind_name);
    if (reg == nullptr) {
        return NullOpt;
    }
    return reg->kind_;
}

}// namespace litetvm
