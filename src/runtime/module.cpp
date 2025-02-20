//
// Created by richard on 2/4/25.
//

#include "runtime/module.h"
#include "runtime/registry.h"
#include "runtime/file_utils.h"

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
        for (const Module& m: n->imports_) {
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
        for (Module& m: self->imports_) {
            pf = m.operator->()->GetFunction(name, query_imports);
            if (pf != nullptr) {
                return pf;
            }
        }
    }
    return pf;
}

Module Module::LoadFromFile(const String& file_name, const String& format) {
    std::string fmt = GetFileFormat(file_name, format);
    CHECK(fmt.length() != 0) << "Cannot deduce format of file " << file_name;
    if (fmt == "dll" || fmt == "dylib" || fmt == "dso") {
        fmt = "so";
    }
    std::string load_f_name = "runtime.module.loadfile_" + fmt;
    VLOG(1) << "Loading module from '" << file_name << "' of format '" << fmt << "'";
    const PackedFunc* f = RegistryManager::Global().Get(load_f_name);
    CHECK(f != nullptr) << "Loader for `." << format << "` files is not registered,"
                        << " resolved to (" << load_f_name << ") in the global registry."
                        << "Ensure that you have loaded the correct runtime code, and"
                        << "that you are on the correct hardware architecture.";
    Module m = static_cast<Module>((*f)(file_name, format));
    return m;
}

void ModuleNode::SaveToFile(const String& file_name, const String& format) {
    LOG(FATAL) << "Module[" << type_key() << "] does not support SaveToFile";
}

void ModuleNode::SaveToBinary(dmlc::Stream* stream) {
    LOG(FATAL) << "Module[" << type_key() << "] does not support SaveToBinary";
}

String ModuleNode::GetSource(const String& format) {
    LOG(FATAL) << "Module[" << type_key() << "] does not support GetSource";
}

const PackedFunc* ModuleNode::GetFuncFromEnv(const String& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = import_cache_.find(name);
    if (it != import_cache_.end()) return it->second.get();
    PackedFunc pf;
    for (Module& m: this->imports_) {
        pf = m.GetFunction(name, true);
        if (pf != nullptr) break;
    }

    if (pf == nullptr) {
        const PackedFunc* f = RegistryManager::Global().Get(name);
        CHECK(f != nullptr) << "Cannot find function " << name
                            << " in the imported modules or global registry."
                            << " If this involves ops from a contrib library like"
                            << " cuDNN, ensure TVM was built with the relevant"
                            << " library.";
        return f;
    }

    import_cache_.insert(std::make_pair(name, std::make_shared<PackedFunc>(pf)));
    return import_cache_.at(name).get();
}

String ModuleNode::GetFormat() {
    LOG(FATAL) << "Module[" << type_key() << "] does not support GetFormat";
}

bool ModuleNode::ImplementsFunction(const String& name, bool query_imports) {
    return GetFunction(name, query_imports) != nullptr;
}

bool RuntimeEnabled(const String& target_str) {
    std::string target = target_str;
    std::string f_name;
    if (target == "cpu") {
        return true;
    } else if (target == "cuda" || target == "gpu") {
        f_name = "device_api.cuda";
    } else if (target == "cl" || target == "opencl" || target == "sdaccel") {
        f_name = "device_api.opencl";
    } else if (target == "mtl" || target == "metal") {
        f_name = "device_api.metal";
    } else if (target == "tflite") {
        f_name = "target.runtime.tflite";
    } else if (target == "vulkan") {
        f_name = "device_api.vulkan";
    } else if (target == "stackvm") {
        f_name = "target.build.stackvm";
    } else if (target == "rpc") {
        f_name = "device_api.rpc";
    } else if (target == "hexagon") {
        f_name = "device_api.hexagon";
    } else if (target.length() >= 5 && target.substr(0, 5) == "nvptx") {
        f_name = "device_api.cuda";
    } else if (target.length() >= 4 && target.substr(0, 4) == "rocm") {
        f_name = "device_api.rocm";
    } else if (target.length() >= 4 && target.substr(0, 4) == "llvm") {
        const PackedFunc* pf = runtime::RegistryManager::Global().Get("codegen.llvm_target_enabled");
        if (pf == nullptr) return false;
        return static_cast<bool>((*pf)(target));
    } else {
        LOG(FATAL) << "Unknown optional runtime " << target;
    }
    return runtime::RegistryManager::Global().Get(f_name) != nullptr;
}

TVM_REGISTER_GLOBAL("runtime.RuntimeEnabled").set_body_typed(RuntimeEnabled);

TVM_REGISTER_GLOBAL("runtime.ModuleGetSource").set_body_typed([](Module mod, std::string fmt) {
    return mod->GetSource(fmt);
});

TVM_REGISTER_GLOBAL("runtime.ModuleImportsSize").set_body_typed([](Module mod) {
    return static_cast<int64_t>(mod->imports().size());
});

TVM_REGISTER_GLOBAL("runtime.ModuleGetImport").set_body_typed([](Module mod, int index) {
    return mod->imports().at(index);
});

TVM_REGISTER_GLOBAL("runtime.ModuleClearImports").set_body_typed([](Module mod) {
    mod->ClearImports();
});

TVM_REGISTER_GLOBAL("runtime.ModuleGetTypeKey").set_body_typed([](Module mod) {
    return std::string(mod->type_key());
});

TVM_REGISTER_GLOBAL("runtime.ModuleGetFormat").set_body_typed([](Module mod) {
    return mod->GetFormat();
});

TVM_REGISTER_GLOBAL("runtime.ModuleLoadFromFile").set_body_typed(Module::LoadFromFile);
TVM_REGISTER_GLOBAL("runtime.ModuleGetFunction")
        .set_body_typed([](Module mod, String name, bool query_imports) {
            return mod->GetFunction(name, query_imports);
        });

TVM_REGISTER_GLOBAL("runtime.ModuleSaveToFile")
        .set_body_typed([](Module mod, String name, String fmt) { mod->SaveToFile(name, fmt); });

TVM_REGISTER_GLOBAL("runtime.ModuleGetPropertyMask").set_body_typed([](Module mod) {
    return mod->GetPropertyMask();
});

TVM_REGISTER_GLOBAL("runtime.ModuleImplementsFunction")
        .set_body_typed([](Module mod, String name, bool query_imports) {
            return mod->ImplementsFunction(std::move(name), query_imports);
        });

TVM_REGISTER_OBJECT_TYPE(ModuleNode);

}// namespace litetvm::runtime
