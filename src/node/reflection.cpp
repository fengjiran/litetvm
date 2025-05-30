//
// Created by 赵丹 on 25-2-28.
//

#include "node/reflection.h"
#include "ir/attrs.h"
#include "runtime/registry.h"

namespace litetvm {

using runtime::NDArray;
using runtime::PackedFunc;
using runtime::TVMArgs;
using runtime::TVMArgValue;
using runtime::TVMRetValue;

// Attr getter.
class AttrGetter : public AttrVisitor {
public:
    const String& skey;
    TVMRetValue* ret;
    bool found_ref_object{false};

    AttrGetter(const String& key, TVMRetValue* ret_value) : skey(key), ret(ret_value) {}

    void Visit(const char* key, double* value) final {
        if (skey == key) {
            *ret = value[0];
        }
    }

    void Visit(const char* key, int64_t* value) final {
        if (skey == key) {
            *ret = value[0];
        }
    }

    void Visit(const char* key, uint64_t* value) final {
        CHECK_LE(value[0], static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) << "cannot return too big constant";
        if (skey == key) {
            *ret = static_cast<int64_t>(value[0]);
        }
    }

    void Visit(const char* key, int* value) final {
        if (skey == key) {
            *ret = static_cast<int64_t>(value[0]);
        }
    }

    void Visit(const char* key, bool* value) final {
        if (skey == key) {
            *ret = static_cast<int64_t>(value[0]);
        }
    }

    void Visit(const char* key, void** value) final {
        if (skey == key) {
            *ret = value[0];
        }
    }

    void Visit(const char* key, DataType* value) final {
        if (skey == key) {
            *ret = value[0];
        }
    }

    void Visit(const char* key, std::string* value) final {
        if (skey == key) {
            *ret = value[0];
        }
    }

    void Visit(const char* key, NDArray* value) final {
        if (skey == key) {
            *ret = value[0];
            found_ref_object = true;
        }
    }

    void Visit(const char* key, ObjectRef* value) final {
        if (skey == key) {
            *ret = value[0];
            found_ref_object = true;
        }
    }
};

// List names;
class AttrDir : public AttrVisitor {
public:
    std::vector<std::string>* names{nullptr};

    void Visit(const char* key, double* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, int64_t* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, uint64_t* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, bool* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, int* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, void** value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, DataType* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, std::string* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, NDArray* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }

    void Visit(const char* key, ObjectRef* value) final {
        names->emplace_back(key);
        UNUSED(value);
    }
};

class NodeAttrSetter : public AttrVisitor {
public:
    std::string type_key;
    std::unordered_map<std::string, TVMArgValue> attrs;

    void Visit(const char* key, double* value) final {
        *value = GetAttr(key).operator double();
    }

    void Visit(const char* key, int64_t* value) final {
        *value = GetAttr(key).operator int64_t();
    }

    void Visit(const char* key, uint64_t* value) final {
        *value = GetAttr(key).operator uint64_t();
    }

    void Visit(const char* key, int* value) final {
        *value = GetAttr(key).operator int();
    }

    void Visit(const char* key, bool* value) final {
        *value = GetAttr(key).operator bool();
    }

    void Visit(const char* key, std::string* value) final {
        *value = GetAttr(key).operator std::string();
    }

    void Visit(const char* key, void** value) final {
        *value = GetAttr(key).operator void*();
    }

    void Visit(const char* key, DataType* value) final {
        *value = GetAttr(key).operator DataType();
    }

    void Visit(const char* key, NDArray* value) final {
        *value = GetAttr(key).operator runtime::NDArray();
    }

    void Visit(const char* key, ObjectRef* value) final {
        *value = GetAttr(key).operator ObjectRef();
    }

private:
    TVMArgValue GetAttr(const char* key) {
        auto it = attrs.find(key);
        if (it == attrs.end()) {
            LOG(FATAL) << type_key << ": require field " << key;
        }
        TVMArgValue v = it->second;
        attrs.erase(it);
        return v;
    }
};

TVMRetValue ReflectionVTable::GetAttr(Object* self, const String& field_name) const {
    TVMRetValue ret;
    AttrGetter getter(field_name, &ret);

    bool success;
    if (getter.skey == "type_key") {
        ret = self->GetTypeKey();
        success = true;
    } else if (!self->IsInstance<DictAttrsNode>()) {
        VisitAttrs(self, &getter);
        success = getter.found_ref_object || ret.type_code() != static_cast<int>(TVMArgTypeCode::kTVMNullptr);
    } else {
        // specially handle dict attr
        auto* dnode = static_cast<DictAttrsNode*>(self);
        auto it = dnode->dict.find(getter.skey);
        if (it != dnode->dict.end()) {
            success = true;
            ret = (*it).second;
        } else {
            success = false;
        }
    }

    if (!success) {
        LOG(FATAL) << "AttributeError: " << self->GetTypeKey() << " object has no attributed " << getter.skey;
    }
    return ret;
}

std::vector<std::string> ReflectionVTable::ListAttrNames(Object* self) const {
    std::vector<std::string> names;
    AttrDir dir;
    dir.names = &names;

    if (!self->IsInstance<DictAttrsNode>()) {
        VisitAttrs(self, &dir);
    } else {
        // specially handle dict attr
        auto* dnode = static_cast<DictAttrsNode*>(self);
        for (const auto& kv: dnode->dict) {
            names.push_back(kv.first);
        }
    }
    return names;
}

ReflectionVTable* ReflectionVTable::Global() {
    static ReflectionVTable inst;
    return &inst;
}

ObjectPtr<Object> ReflectionVTable::CreateInitObject(const std::string& type_key,
                                                     const std::string& repr_bytes) const {
    uint32_t tindex = Object::TypeKey2Index(type_key);
    if (tindex >= fcreate_.size() || fcreate_[tindex] == nullptr) {
        LOG(FATAL) << "TypeError: " << type_key << " is not registered via TVM_REGISTER_NODE_TYPE";
    }
    return fcreate_[tindex](repr_bytes);
}

void InitNodeByPackedArgs(const ReflectionVTable* reflection, Object* n, const TVMArgs& args) {
    NodeAttrSetter setter;
    setter.type_key = n->GetTypeKey();
    CHECK_EQ(args.size() % 2, 0);
    for (int i = 0; i < args.size(); i += 2) {
        setter.attrs.emplace(args[i].operator std::string(), args[i + 1]);
    }
    reflection->VisitAttrs(n, &setter);

    if (!setter.attrs.empty()) {
        std::ostringstream os;
        os << setter.type_key << " does not contain field ";
        for (const auto& kv: setter.attrs) {
            os << " " << kv.first;
        }
        LOG(FATAL) << os.str();
    }
}

ObjectRef ReflectionVTable::CreateObject(const std::string& type_key, const TVMArgs& kwargs) const {
    ObjectPtr<Object> n = this->CreateInitObject(type_key);
    if (n->IsInstance<BaseAttrsNode>()) {
        static_cast<BaseAttrsNode*>(n.get())->InitByPackedArgs(kwargs);
    } else {
        InitNodeByPackedArgs(this, n.get(), kwargs);
    }
    return ObjectRef(n);
}

ObjectRef ReflectionVTable::CreateObject(const std::string& type_key,
                                         const Map<String, ObjectRef>& kwargs) const {
    // Redirect to the TVMArgs version
    // It is not the most efficient way, but CreateObject is not meant to be used
    // in a fast code-path and is mainly reserved as a flexible API for frontends.
    std::vector<TVMValue> values(kwargs.size() * 2);
    std::vector<int32_t> tcodes(kwargs.size() * 2);
    runtime::TVMArgsSetter setter(values.data(), tcodes.data());
    int index = 0;

    for (const auto& kv: *static_cast<const MapNode*>(kwargs.get())) {
        setter(index, Downcast<String>(kv.first).c_str());
        setter(index + 1, kv.second);
        index += 2;
    }

    return CreateObject(type_key, runtime::TVMArgs(values.data(), tcodes.data(), kwargs.size() * 2));
}


// Expose to FFI APIs.
void NodeGetAttr(TVMArgs args, TVMRetValue* ret) {
    CHECK_EQ(args[0].type_code(), static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    Object* self = static_cast<Object*>(args[0].value().v_handle);
    *ret = ReflectionVTable::Global()->GetAttr(self, String(args[1]));
}

void NodeListAttrNames(TVMArgs args, TVMRetValue* ret) {
    CHECK_EQ(args[0].type_code(), static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    auto* self = static_cast<Object*>(args[0].value().v_handle);
    auto names = std::make_shared<std::vector<std::string>>(ReflectionVTable::Global()->ListAttrNames(self));

    *ret = PackedFunc([names](TVMArgs args, TVMRetValue* rv) {
        int64_t i(args[0]);
        if (i == -1) {
            *rv = static_cast<int64_t>(names->size());
        } else {
            *rv = (*names)[i];
        }
    });
}

// API function to make node.
// args format:
//   key1, value1, ..., key_n, value_n
void MakeNode(const TVMArgs& args, TVMRetValue* rv) {
    std::string type_key = args[0];
    std::string empty_str;
    TVMArgs kwargs(args.values + 1, args.type_codes + 1, args.size() - 1);
    *rv = ReflectionVTable::Global()->CreateObject(type_key, kwargs);
}

TVM_REGISTER_GLOBAL("node.NodeGetAttr").set_body(NodeGetAttr);
TVM_REGISTER_GLOBAL("node.NodeListAttrNames").set_body(NodeListAttrNames);
TVM_REGISTER_GLOBAL("node.MakeNode").set_body(MakeNode);


namespace {
// Attribute visitor class for finding the attribute key by its address
class GetAttrKeyByAddressVisitor : public AttrVisitor {
public:
    explicit GetAttrKeyByAddressVisitor(const void* attr_address)
        : attr_address_(attr_address), key_(nullptr) {}

    void Visit(const char* key, double* value) final { DoVisit(key, value); }
    void Visit(const char* key, int64_t* value) final { DoVisit(key, value); }
    void Visit(const char* key, uint64_t* value) final { DoVisit(key, value); }
    void Visit(const char* key, int* value) final { DoVisit(key, value); }
    void Visit(const char* key, bool* value) final { DoVisit(key, value); }
    void Visit(const char* key, std::string* value) final { DoVisit(key, value); }
    void Visit(const char* key, void** value) final { DoVisit(key, value); }
    void Visit(const char* key, DataType* value) final { DoVisit(key, value); }
    void Visit(const char* key, NDArray* value) final { DoVisit(key, value); }
    void Visit(const char* key, ObjectRef* value) final { DoVisit(key, value); }

    const char* GetKey() const { return key_; }

private:
    const void* attr_address_;
    const char* key_;

    void DoVisit(const char* key, const void* candidate) {
        if (attr_address_ == candidate) {
            key_ = key;
        }
    }
};
}// anonymous namespace

Optional<String> GetAttrKeyByAddress(const Object* object, const void* attr_address) {
    GetAttrKeyByAddressVisitor visitor(attr_address);
    ReflectionVTable::Global()->VisitAttrs(const_cast<Object*>(object), &visitor);
    const char* key = visitor.GetKey();
    if (key == nullptr) {
        return NullOpt;
    }
    return String(key);
}

}// namespace litetvm
