//
// Created by 赵丹 on 25-4-15.
//

#include "ir/global_var_supply.h"
#include "runtime/registry.h"

namespace litetvm {

GlobalVarSupply::GlobalVarSupply(const NameSupply& name_supply,
                                 std::unordered_map<std::string, GlobalVar> name_to_var_map) {
    auto n = make_object<GlobalVarSupplyNode>(name_supply, name_to_var_map);
    data_ = std::move(n);
}

std::string GetModuleName(const IRModule& module) {
    return module->GetAttr<String>(attr::kModuleName).value_or("tvmgen_default");
}

GlobalVarSupply::GlobalVarSupply(const Array<IRModule>& modules) : GlobalVarSupply() {
    if (!modules.empty()) {
        IRModule first_mod = modules.front();
        this->operator->()->name_supply_->prefix_ = GetModuleName(first_mod);
    }
    for (auto& mod: modules) {
        for (auto kv: mod->functions) {
            this->operator->()->ReserveGlobalVar(kv.first);
        }
    }
}

GlobalVarSupply::GlobalVarSupply(const IRModule module)
    : GlobalVarSupply(Array<IRModule>{module}) {}

void GlobalVarSupplyNode::ReserveGlobalVar(const GlobalVar& var, bool allow_conflict) {
    name_supply_->ReserveName(var->name_hint, false);
    if (!allow_conflict) {
        CHECK(name_to_var_map_.count(var->name_hint) == 0)
                << "GlobalVar " << var << " conflicts by name in this supply.";
    }
    name_to_var_map_[var->name_hint] = var;
}

GlobalVarSupplyNode::GlobalVarSupplyNode(NameSupply name_supply,
                                         std::unordered_map<std::string, GlobalVar> name_to_var_map)
    : name_supply_(std::move(name_supply)), name_to_var_map_(std::move(name_to_var_map)) {}

GlobalVar GlobalVarSupplyNode::UniqueGlobalFor(const String& name, bool add_prefix) {
    String final_name = name_supply_->ReserveName(name, add_prefix);

    auto it = name_to_var_map_.find(final_name);
    if (it != name_to_var_map_.end()) {
        return it->second;
    } else {
        GlobalVar var = GlobalVar(final_name);
        name_to_var_map_.emplace(final_name, var);
        return var;
    }
}

GlobalVar GlobalVarSupplyNode::FreshGlobal(String name, bool add_prefix) {
    String final_name = name_supply_->FreshName(name, add_prefix);
    CHECK(name_to_var_map_.find(final_name) == name_to_var_map_.end())
            << "GlobalVar already exists for name " << final_name;
    GlobalVar var = GlobalVar(final_name);
    name_to_var_map_.emplace(final_name, var);
    return var;
}

TVM_REGISTER_NODE_TYPE(GlobalVarSupplyNode);

TVM_REGISTER_GLOBAL("ir.GlobalVarSupply_NameSupply")
        .set_body_typed([](const NameSupply& name_supply) { return GlobalVarSupply(name_supply); });

TVM_REGISTER_GLOBAL("ir.GlobalVarSupply_IRModule").set_body_typed([](IRModule mod) {
    return GlobalVarSupply(std::move(mod));
});

TVM_REGISTER_GLOBAL("ir.GlobalVarSupply_IRModules").set_body_typed([](const Array<IRModule>& mods) {
    return GlobalVarSupply(mods);
});

TVM_REGISTER_GLOBAL("ir.GlobalVarSupply_FreshGlobal")
        .set_body_method<GlobalVarSupply>(&GlobalVarSupplyNode::FreshGlobal);

TVM_REGISTER_GLOBAL("ir.GlobalVarSupply_UniqueGlobalFor")
        .set_body_method<GlobalVarSupply>(&GlobalVarSupplyNode::UniqueGlobalFor);

TVM_REGISTER_GLOBAL("ir.GlobalVarSupply_ReserveGlobalVar")
        .set_body_method<GlobalVarSupply>(&GlobalVarSupplyNode::ReserveGlobalVar);


}// namespace litetvm