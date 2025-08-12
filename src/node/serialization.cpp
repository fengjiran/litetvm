//
// Created by richard on 8/2/25.
//

#include "ffi/extra/serialization.h"
#include "ffi/extra/json.h"
#include "ffi/reflection/registry.h"
#include "runtime/base.h"

namespace litetvm {

std::string SaveJSON(Any n) {
    int indent = 2;
    ffi::json::Object metadata{{"tvm_version", TVM_VERSION}};
    ffi::json::Value jgraph = ffi::ToJSONGraph(n, metadata);
    return ffi::json::Stringify(jgraph, indent);
}

Any LoadJSON(std::string json_str) {
    ffi::json::Value jgraph = ffi::json::Parse(json_str);
    return ffi::FromJSONGraph(jgraph);
}

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef().def("node.SaveJSON", SaveJSON).def("node.LoadJSON", LoadJSON);
});
}// namespace litetvm