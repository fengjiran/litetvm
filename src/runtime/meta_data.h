//
// Created by 赵丹 on 25-8-1.
//

#ifndef LITETVM_RUNTIME_META_DATA_H
#define LITETVM_RUNTIME_META_DATA_H

#include "runtime/ndarray.h"

#include <dmlc/io.h>
#include <dmlc/json.h>
#include <string>
#include <utility>
#include <vector>

namespace litetvm {
namespace runtime {

inline String get_name_mangled(const String& module_name, const String& name) {
    std::stringstream ss;
    ss << module_name << "_" << name;
    return ss.str();
}

namespace launch_param {

/*! \brief A tag to specify whether or not dynamic shared memory is used */
constexpr const char* kUseDynamicSharedMemoryTag = "tir.use_dyn_shared_memory";

}// namespace launch_param

/*! \brief function information needed by device */
struct FunctionInfo {
    std::string name;
    std::vector<DLDataType> arg_types;
    std::vector<std::string> launch_param_tags;

    enum class ArgExtraTags : int { kNone = 0,
                                    kTensorMap = 1 };
    std::vector<ArgExtraTags> arg_extra_tags;

    void Save(dmlc::JSONWriter* writer) const;
    void Load(dmlc::JSONReader* reader);
    void Save(dmlc::Stream* writer) const;
    bool Load(dmlc::Stream* reader);
};
}// namespace runtime
}// namespace litetvm

namespace dmlc {
DMLC_DECLARE_TRAITS(has_saveload, ::litetvm::runtime::FunctionInfo, true);
}  // namespace dmlc

#endif//LITETVM_RUNTIME_META_DATA_H
