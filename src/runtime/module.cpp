//
// Created by richard on 2/4/25.
//

#include "runtime/module.h"
#include "runtime/registry.h"

#include <unordered_set>

namespace litetvm::runtime {

void ModuleNode::Import(Module other) {
    // specially handle rpc
    if (!std::strcmp(this->type_key(), "rpc")) {
        static const PackedFunc* fimport_ = nullptr;
        if (fimport_ == nullptr) {
            fimport_ = RegistryManager::Global().Get("rpc.ImportRemoteModule");
            CHECK(fimport_ != nullptr);
        }
        (*fimport_)(GetRef<Module>(this), other);
        return;
    }
    // cyclic detection.
    std::unordered_set<const ModuleNode*> visited{other.operator->()};
    std::vector<const ModuleNode*> stack{other.operator->()};
    while (!stack.empty()) {
        const ModuleNode* n = stack.back();
        stack.pop_back();
        for (const Module& m : n->imports_) {
            const ModuleNode* next = m.operator->();
            if (visited.count(next)) continue;
            visited.insert(next);
            stack.push_back(next);
        }
    }
    CHECK(!visited.count(this)) << "Cyclic dependency detected during import";
    this->imports_.emplace_back(std::move(other));
}


PackedFunc ModuleNode::GetFunction(const String& name, bool query_imports) {
    ModuleNode* self = this;
    PackedFunc pf = self->GetFunction(name, GetObjectPtr<Object>(this));
    if (pf != nullptr) return pf;
    if (query_imports) {
        for (Module& m : self->imports_) {
            pf = m.operator->()->GetFunction(name, query_imports);
            if (pf != nullptr) {
                return pf;
            }
        }
    }
    return pf;
}

// Module Module::LoadFromFile(const String& file_name, const String& format) {
//     std::string fmt = GetFileFormat(file_name, format);
//     CHECK(fmt.length() != 0) << "Cannot deduce format of file " << file_name;
//     if (fmt == "dll" || fmt == "dylib" || fmt == "dso") {
//         fmt = "so";
//     }
//     std::string load_f_name = "runtime.module.loadfile_" + fmt;
//     VLOG(1) << "Loading module from '" << file_name << "' of format '" << fmt << "'";
//     const PackedFunc* f = RegistryManager::Global().Get(load_f_name);
//     CHECK(f != nullptr) << "Loader for `." << format << "` files is not registered,"
//                          << " resolved to (" << load_f_name << ") in the global registry."
//                          << "Ensure that you have loaded the correct runtime code, and"
//                          << "that you are on the correct hardware architecture.";
//     Module m = (*f)(file_name, format);
//     return m;
// }

String ModuleNode::GetFormat() {
    LOG(FATAL) << "Module[" << type_key() << "] does not support GetFormat";
}

}
