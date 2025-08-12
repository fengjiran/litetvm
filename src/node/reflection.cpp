//
// Created by richard on 8/2/25.
//

#include "node/reflection.h"
#include "ffi/reflection/accessor.h"
#include "ffi/reflection/registry.h"
#include "ir/attrs.h"
#include "node/node.h"

namespace litetvm {

using ffi::Any;
using ffi::Function;
using ffi::PackedArgs;

// API function to make node.
// args format:
//   key1, value1, ..., key_n, value_n
void MakeNode(const ffi::PackedArgs& args, ffi::Any* rv) {
    // TODO(tvm-team): consider further simplify by removing DictAttrsNode special handling
    String type_key = args[0].cast<String>();
    int32_t type_index;
    TVMFFIByteArray type_key_array = TVMFFIByteArray{type_key.data(), type_key.size()};
    TVM_FFI_CHECK_SAFE_CALL(TVMFFITypeKeyToIndex(&type_key_array, &type_index));
    if (type_index == DictAttrsNode::RuntimeTypeIndex()) {
        ObjectPtr<DictAttrsNode> attrs = make_object<DictAttrsNode>();
        attrs->InitByPackedArgs(args.Slice(1), false);
        *rv = ObjectRef(attrs);
    } else {
        auto fcreate_object = ffi::Function::GetGlobalRequired("ffi.MakeObjectFromPackedArgs");
        fcreate_object.CallPacked(args, rv);
    }
}

TVM_FFI_STATIC_INIT_BLOCK({
  namespace refl = litetvm::ffi::reflection;
  refl::GlobalDef().def_packed("node.MakeNode", MakeNode);
});

}  // namespace tvm
