//
// Created by 赵丹 on 25-3-20.
//

#include "ir/op.h"
#include "ir/type.h"
#include "node/attr_registry.h"
#include "runtime/module.h"
#include "runtime/packed_func.h"

#include <memory>
#include <utility>

namespace litetvm {

using runtime::PackedFunc;
using runtime::TVMArgs;
using runtime::TVMRetValue;

using OpRegistry = AttrRegistry<OpRegEntry, Op>;

// find operator by name
const Op& Op::Get(const String& name) {
    const OpRegEntry* reg = OpRegistry::Global()->Get(name);
    CHECK(reg != nullptr) << "AttributeError: Operator " << name << " is not registered";
    return reg->op();
}

// Get an attribute map by key
const AttrRegistryMapContainerMap<Op>& Op::GetAttrMapContainer(const String& key) {
    return OpRegistry::Global()->GetAttrMap(key);
}

// Check if a key is present in the registry.
bool Op::HasAttrMap(const String& attr_name) {
    return OpRegistry::Global()->HasAttrMap(attr_name);
}

OpRegEntry::OpRegEntry(uint32_t reg_index) {
    auto n = make_object<OpNode>();
    n->index_ = reg_index;
    op_ = Op(n);
}

OpRegEntry& OpRegEntry::RegisterOrGet(const String& name) {
    return OpRegistry::Global()->RegisterOrGet(name);
}

void OpRegEntry::reset_attr(const std::string& attr_name) const {
    OpRegistry::Global()->ResetAttr(attr_name, op_);
}

void OpRegEntry::UpdateAttr(const String& key, TVMRetValue value, int plevel) const {
    OpRegistry::Global()->UpdateAttr(key, op_, std::move(value), plevel);
}

// Frontend APIs
TVM_REGISTER_GLOBAL("ir.ListOpNames").set_body_typed([]() {
    return OpRegistry::Global()->ListAllNames();
});

TVM_REGISTER_GLOBAL("ir.GetOp").set_body_typed([](String name) -> Op { return Op::Get(name); });

TVM_REGISTER_GLOBAL("ir.OpGetAttr").set_body_typed([](Op op, String attr_name) -> TVMRetValue {
    auto op_map = Op::GetAttrMap<TVMRetValue>(attr_name);
    TVMRetValue rv;
    if (op_map.count(op)) {
        rv = op_map[op];
    }
    return rv;
});

TVM_REGISTER_GLOBAL("ir.OpHasAttr").set_body_typed([](Op op, String attr_name) -> bool {
    return Op::HasAttrMap(attr_name);
});

TVM_REGISTER_GLOBAL("ir.OpSetAttr")
        .set_body_typed([](Op op, String attr_name, TVMArgValue value, int plevel) {
            auto& reg = OpRegistry::Global()->RegisterOrGet(op->name).set_name();
            reg.set_attr(attr_name, value, plevel);
        });

TVM_REGISTER_GLOBAL("ir.OpResetAttr").set_body_typed([](Op op, String attr_name) {
    auto& reg = OpRegistry::Global()->RegisterOrGet(op->name);
    reg.reset_attr(attr_name);
});

TVM_REGISTER_GLOBAL("ir.RegisterOp").set_body_typed([](String op_name, String descr) {
    const OpRegEntry* reg = OpRegistry::Global()->Get(op_name);
    CHECK(reg == nullptr) << "AttributeError: Operator " << op_name << " is registered before";
    auto& op = OpRegistry::Global()->RegisterOrGet(op_name).set_name();
    op.describe(descr);
});

TVM_REGISTER_GLOBAL("ir.OpAddArgument")
        .set_body_typed([](Op op, String name, String type, String description) {
            auto& reg = OpRegistry::Global()->RegisterOrGet(op->name).set_name();
            reg.add_argument(name, type, description);
        });

TVM_REGISTER_GLOBAL("ir.OpSetSupportLevel").set_body_typed([](Op op, int level) {
    auto& reg = OpRegistry::Global()->RegisterOrGet(op->name).set_name();
    reg.set_support_level(level);
});

TVM_REGISTER_GLOBAL("ir.OpSetNumInputs").set_body_typed([](Op op, int n) {
    auto& reg = OpRegistry::Global()->RegisterOrGet(op->name).set_name();
    reg.set_num_inputs(n);
});

TVM_REGISTER_GLOBAL("ir.OpSetAttrsTypeKey").set_body_typed([](Op op, String key) {
    auto& reg = OpRegistry::Global()->RegisterOrGet(op->name).set_name();
    reg.set_attrs_type_key(key);
});

TVM_REGISTER_GLOBAL("ir.RegisterOpAttr")
        .set_body_typed([](String op_name, String attr_key, TVMArgValue value, int plevel) {
            auto& reg = OpRegistry::Global()->RegisterOrGet(op_name).set_name();
            // enable resgiteration and override of certain properties
            if (attr_key == "num_inputs" && plevel > 128) {
                reg.set_num_inputs(static_cast<int32_t>(value));
            } else if (attr_key == "attrs_type_key" && plevel > 128) {
                LOG(FATAL) << "attrs type key no longer supported";
            } else {
                // normal attr table override.
                if (value.type_code() == static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle)) {
                    // do an eager copy of the PackedFunc
                    PackedFunc f = value;
                    reg.set_attr(attr_key, f, plevel);
                } else {
                    reg.set_attr(attr_key, value, plevel);
                }
            }
        });

// TVM_REGISTER_GLOBAL("ir.RegisterOpLowerIntrinsic")
//         .set_body_typed([](String name, PackedFunc f, String target, int plevel) {
//             OpRegEntry::RegisterOrGet(name).set_attr<FLowerIntrinsic>(target + ".FLowerIntrinsic", f, plevel);
//         });


}// namespace litetvm