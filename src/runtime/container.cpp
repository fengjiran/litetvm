//
// Created by 赵丹 on 25-2-19.
//

#include "runtime/adt.h"
#include "runtime/array.h"
#include "runtime/map.h"
#include "runtime/registry.h"
#include "runtime/shape_tuple.h"
#include "runtime/string.h"
#include "runtime/closure.h"

namespace litetvm::runtime {

// Array
TVM_REGISTER_OBJECT_TYPE(ArrayNode);
// String
TVM_REGISTER_OBJECT_TYPE(StringObj);
// Map
TVM_REGISTER_OBJECT_TYPE(MapNode);
// ShapeTuple
TVM_REGISTER_OBJECT_TYPE(ShapeTupleNode);
// ADT
TVM_REGISTER_OBJECT_TYPE(ADTObj);
// Closure
TVM_REGISTER_OBJECT_TYPE(ClosureObj);


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

TVM_REGISTER_GLOBAL("runtime.GetADTTag").set_body([](TVMArgs args, TVMRetValue* rv) {
    // ObjectRef obj = args[0];
    ObjectRef obj(args[0]);
    const auto& adt = Downcast<ADT>(obj);
    *rv = static_cast<int64_t>(adt.tag());
});

TVM_REGISTER_GLOBAL("runtime.GetADTSize").set_body([](TVMArgs args, TVMRetValue* rv) {
    // ObjectRef obj = args[0];
    ObjectRef obj(args[0]);
    const auto& adt = Downcast<ADT>(obj);
    *rv = static_cast<int64_t>(adt.size());
});

TVM_REGISTER_GLOBAL("runtime.GetADTFields").set_body([](TVMArgs args, TVMRetValue* rv) {
    // ObjectRef obj = args[0];
    ObjectRef obj(args[0]);
    int idx = static_cast<int>(args[1]);
    const auto& adt = Downcast<ADT>(obj);
    CHECK_LT(idx, adt.size());
    *rv = adt[idx];
});

TVM_REGISTER_GLOBAL("runtime.Tuple").set_body([](TVMArgs args, TVMRetValue* rv) {
    std::vector<ObjectRef> fields;
    for (auto i = 0; i < args.size(); ++i) {
        fields.push_back(ObjectRef(args[i]));
    }
    *rv = ADT::Tuple(fields);
});

TVM_REGISTER_GLOBAL("runtime.ADT").set_body([](TVMArgs args, TVMRetValue* rv) {
    int itag = (int) args[0];
    size_t tag = static_cast<size_t>(itag);
    std::vector<ObjectRef> fields;
    for (int i = 1; i < args.size(); i++) {
        fields.push_back(ObjectRef(args[i]));
    }
    *rv = ADT(tag, fields);
});

TVM_REGISTER_GLOBAL("runtime.String").set_body_typed([](std::string str) {
    return String(std::move(str));
});

TVM_REGISTER_GLOBAL("runtime.GetFFIString").set_body_typed([](String str) {
    return std::string(str);
});

TVM_REGISTER_GLOBAL("runtime.Map").set_body([](TVMArgs args, TVMRetValue* ret) {
    CHECK_EQ(args.size() % 2, 0);
    std::unordered_map<ObjectRef, ObjectRef, ObjectPtrHash, ObjectPtrEqual> data;
    for (int i = 0; i < args.num_args; i += 2) {
        ObjectRef k = String::CanConvertFrom(args[i]) ? args[i].operator String() : args[i].operator ObjectRef();
        ObjectRef v(args[i + 1]);
        data.emplace(std::move(k), std::move(v));
    }
    *ret = Map(std::move(data));
});

TVM_REGISTER_GLOBAL("runtime.MapSize").set_body([](TVMArgs args, TVMRetValue* ret) {
    CHECK_EQ(args[0].type_code(), static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    auto* ptr = static_cast<Object*>(args[0].value().v_handle);
    CHECK(ptr->IsInstance<MapNode>());
    auto* n = static_cast<const MapNode*>(ptr);
    *ret = static_cast<int64_t>(n->size());
});

TVM_REGISTER_GLOBAL("runtime.MapGetItem").set_body([](TVMArgs args, TVMRetValue* ret) {
    CHECK_EQ(args[0].type_code(), static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    auto* ptr = static_cast<Object*>(args[0].value().v_handle);
    CHECK(ptr->IsInstance<MapNode>());

    auto* n = static_cast<const MapNode*>(ptr);
    auto it = n->find(String::CanConvertFrom(args[1]) ? args[1].operator String()
                                                      : args[1].operator ObjectRef());
    CHECK(it != n->end()) << "cannot find the corresponding key in the Map";
    *ret = (*it).second;
});

TVM_REGISTER_GLOBAL("runtime.MapCount").set_body([](TVMArgs args, TVMRetValue* ret) {
    CHECK_EQ(args[0].type_code(), static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    auto* ptr = static_cast<Object*>(args[0].value().v_handle);
    CHECK(ptr->IsInstance<MapNode>());
    const auto* n = static_cast<const MapNode*>(ptr);
    int64_t cnt = n->count(String::CanConvertFrom(args[1]) ? args[1].operator String()
                                                           : args[1].operator ObjectRef());
    *ret = cnt;
});

TVM_REGISTER_GLOBAL("runtime.MapItems").set_body([](TVMArgs args, TVMRetValue* ret) {
    CHECK_EQ(args[0].type_code(), static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    auto* ptr = static_cast<Object*>(args[0].value().v_handle);
    auto* n = static_cast<const MapNode*>(ptr);
    Array<ObjectRef> rkvs;
    for (const auto& kv: *n) {
        if (kv.first->IsInstance<StringObj>()) {
            rkvs.push_back(Downcast<String>(kv.first));
        } else {
            rkvs.push_back(kv.first);
        }
        rkvs.push_back(kv.second);
    }
    *ret = std::move(rkvs);
});

TVM_REGISTER_GLOBAL("runtime.ShapeTuple").set_body([](TVMArgs args, TVMRetValue* rv) {
    std::vector<ShapeTuple::index_type> shape;
    for (int i = 0; i < args.size(); i++) {
        shape.push_back(static_cast<ShapeTuple::index_type>(args[i]));
    }
    *rv = ShapeTuple(shape);
});

TVM_REGISTER_GLOBAL("runtime.GetShapeTupleSize").set_body_typed([](ShapeTuple shape) {
    return static_cast<int64_t>(shape.size());
});

TVM_REGISTER_GLOBAL("runtime.GetShapeTupleElem").set_body_typed([](ShapeTuple shape, int idx) {
    CHECK_LT(idx, shape.size());
    return shape[idx];
});

}// namespace litetvm::runtime
