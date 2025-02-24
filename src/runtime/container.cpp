//
// Created by 赵丹 on 25-2-19.
//

#include "runtime/array.h"
#include "runtime/map.h"
#include "runtime/registry.h"
#include "runtime/shape_tuple.h"
#include "runtime/string.h"

namespace litetvm::runtime {

// Array
TVM_REGISTER_OBJECT_TYPE(ArrayNode);
// String
TVM_REGISTER_OBJECT_TYPE(StringObj);
// Map
TVM_REGISTER_OBJECT_TYPE(MapNode);
// ShapeTuple
TVM_REGISTER_OBJECT_TYPE(ShapeTupleNode);

TVM_REGISTER_GLOBAL("runtime.Array").set_body([](TVMArgs args, TVMRetValue* ret) {
    std::vector<ObjectRef> data;
    for (int i = 0; i < args.size(); ++i) {
        if (args[i].type_code() != static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {
            data.push_back(args[i].operator ObjectRef());
        } else {
            data.emplace_back(nullptr);
        }
    }
    *ret = Array(data);
});

TVM_REGISTER_GLOBAL("runtime.ArrayGetItem").set_body([](TVMArgs args, TVMRetValue* ret) {
    int64_t i = static_cast<int64_t>(args[1]);
    CHECK_EQ(args[0].type_code(), static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    Object* ptr = static_cast<Object*>(args[0].value().v_handle);
    CHECK(ptr->IsInstance<ArrayNode>());
    auto* n = static_cast<const ArrayNode*>(ptr);
    CHECK_LT(static_cast<size_t>(i), n->size()) << "out of bound of array";
    *ret = n->at(i);
});

TVM_REGISTER_GLOBAL("runtime.ArraySize").set_body([](TVMArgs args, TVMRetValue* ret) {
    CHECK_EQ(args[0].type_code(), static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    auto* ptr = static_cast<Object*>(args[0].value().v_handle);
    CHECK(ptr->IsInstance<ArrayNode>());
    *ret = static_cast<int64_t>(static_cast<const ArrayNode*>(ptr)->size());
});

}// namespace litetvm::runtime
