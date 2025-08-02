//
// Created by 赵丹 on 25-4-11.
//

#include "tir/utils.h"
#include "ir/attrs.h"

namespace litetvm {
namespace tir {

ObjectRef NormalizeAttributeObject(ObjectRef obj) {
    if (const auto* runtime_int = obj.as<runtime::Int::ContainerType>()) {
        return Integer(runtime_int->value);
    }

    if (const auto* runtime_bool = obj.as<runtime::Bool::ContainerType>()) {
        return Bool(runtime_bool->value);
    }

    if (const auto* runtime_float = obj.as<runtime::Float::ContainerType>()) {
        return FloatImm(DataType::Float(32), runtime_float->value);
    }

    if (auto opt_array = obj.as<Array<ObjectRef>>()) {
        return opt_array.value().Map(NormalizeAttributeObject);
    }

    if (auto opt_map = obj.as<Map<ObjectRef, ObjectRef>>()) {
        Map<ObjectRef, ObjectRef> new_map;
        bool is_same = true;

        for (const auto& [key, obj]: opt_map.value()) {
            ObjectRef new_obj = NormalizeAttributeObject(obj);
            is_same = is_same && obj.same_as(new_obj);
            new_map.Set(key, new_obj);
        }

        if (is_same) {
            return obj;
        }
        return new_map;
    }

    if (auto dict_attrs = obj.as<DictAttrs::ContainerType>()) {
        auto new_attrs = Downcast<Map<String, ObjectRef>>(NormalizeAttributeObject(dict_attrs->dict));
        if (new_attrs.same_as(dict_attrs->dict)) {
            return GetRef<DictAttrs>(dict_attrs);
        }
        return DictAttrs(new_attrs);
    }
    return obj;
}

}// namespace tir
}// namespace litetvm
