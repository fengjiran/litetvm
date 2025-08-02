//
// Created by 赵丹 on 25-4-18.
//

#include "ir/replace_global_vars.h"
#include "runtime/registry.h"
#include <vector>

namespace litetvm {
namespace transform {

IRModule ReplaceGlobalVars(IRModule mod, Map<GlobalVar, GlobalVar> replacements) {
    if (replacements.empty()) {
        return mod;
    }

    std::vector<GlobalVar> to_remove;
    IRModule updates;

    const auto& vtable = GlobalVarReplacer::vtable();

    for (const auto& [old_gvar, old_func]: mod->functions) {
        auto new_gvar = replacements.Get(old_gvar).value_or(old_gvar);
        auto new_func = vtable(old_func, replacements);

        if (!new_gvar.same_as(old_gvar)) {
            to_remove.push_back(old_gvar);
        }
        if (!old_gvar.same_as(new_gvar) || !old_func.same_as(new_func)) {
            updates->Add(new_gvar, new_func);
        }
    }

    if (to_remove.size() || updates->functions.size()) {
        auto write_ptr = mod.CopyOnWrite();
        for (const auto& old_gvar: to_remove) {
            write_ptr->Remove(old_gvar);
        }
        write_ptr->Update(updates);
    }
    return mod;
}

TVM_REGISTER_GLOBAL("transform.ReplaceGlobalVars").set_body_typed(ReplaceGlobalVars);

IRModule ModuleReplaceGlobalVars(
        IRModule mod, Map<Variant<String, GlobalVar>, Variant<String, GlobalVar>> replacements) {
    Map<GlobalVar, GlobalVar> gvar_replacements;
    for (const auto& [before, after]: replacements) {
        GlobalVar gvar_before;
        if (auto gvar = before.as<GlobalVar>()) {
            gvar_before = gvar.value();
        } else if (auto str = before.as<String>()) {
            gvar_before = mod->GetGlobalVar(str.value());
        } else {
            LOG(FATAL) << "Variant<String,GlobalVar> must contain either String or GlobalVar";
        }

        GlobalVar gvar_after;
        if (auto gvar = after.as<GlobalVar>()) {
            gvar_after = gvar.value();
        } else if (auto str = after.as<String>()) {
            gvar_after = gvar_before;
            gvar_after.CopyOnWrite()->name_hint = str.value();
        } else {
            LOG(FATAL) << "Variant<String,GlobalVar> must contain either String or GlobalVar";
        }

        gvar_replacements.Set(gvar_before, gvar_after);
    }

    return ReplaceGlobalVars(mod, gvar_replacements);
}

TVM_REGISTER_GLOBAL("ir.Module_ReplaceGlobalVars").set_body_typed(ModuleReplaceGlobalVars);

}// namespace transform
}// namespace litetvm