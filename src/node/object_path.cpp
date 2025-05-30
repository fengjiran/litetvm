//
// Created by richard on 2/26/25.
//

#include "node/object_path.h"
#include "node/repr_printer.h"
#include "runtime/memory.h"
#include "runtime/registry.h"

#include <algorithm>
#include <cstring>

namespace litetvm {

Optional<ObjectPath> ObjectPathNode::GetParent() const {
    if (parent_ == nullptr) {
        return NullOpt;
    }
    return Downcast<ObjectPath>(parent_);
}

TVM_REGISTER_GLOBAL("node.ObjectPathGetParent").set_body_method<ObjectPath>(&ObjectPathNode::GetParent);
TVM_REGISTER_GLOBAL("node.ObjectPathLength").set_body_method<ObjectPath>(&ObjectPathNode::Length);

// --- GetPrefix ---
ObjectPath ObjectPathNode::GetPrefix(int32_t length) const {
    CHECK_GE(length, 1) << "IndexError: Prefix length must be at least 1";
    CHECK_LE(length, Length()) << "IndexError: Attempted to get a prefix longer than the path itself";

    const ObjectPathNode* node = this;
    int32_t suffix_len = Length() - length;
    for (int32_t i = 0; i < suffix_len; ++i) {
        node = node->ParentNode();
    }

    return GetRef<ObjectPath>(node);
}
TVM_REGISTER_GLOBAL("node.ObjectPathGetPrefix").set_body_method<ObjectPath>(&ObjectPathNode::GetPrefix);

bool ObjectPathNode::IsPrefixOf(const ObjectPath& other) const {
    int32_t this_len = Length();
    if (this_len > other->Length()) {
        return false;
    }
    return this->PathsEqual(other->GetPrefix(this_len));
}
TVM_REGISTER_GLOBAL("node.ObjectPathIsPrefixOf").set_body_method<ObjectPath>(&ObjectPathNode::IsPrefixOf);

// --- Attr ---
ObjectPath ObjectPathNode::Attr(const char* attr_key) const {
    if (attr_key != nullptr) {
        return ObjectPath(make_object<AttributeAccessPathNode>(this, attr_key));
    }
    return ObjectPath(make_object<UnknownAttributeAccessPathNode>(this));
}

ObjectPath ObjectPathNode::Attr(const Optional<String>& attr_key) const {
    if (attr_key.defined()) {
        return ObjectPath(make_object<AttributeAccessPathNode>(this, attr_key.value()));
    }
    return ObjectPath(make_object<UnknownAttributeAccessPathNode>(this));
}

TVM_REGISTER_GLOBAL("node.ObjectPathAttr")
        .set_body_typed([](const ObjectPath& object_path, const Optional<String>& attr_key) {
            return object_path->Attr(attr_key);
        });


// --- ArrayIndex ---
ObjectPath ObjectPathNode::ArrayIndex(int32_t index) const {
    return ObjectPath(make_object<ArrayIndexPathNode>(this, index));
}
TVM_REGISTER_GLOBAL("node.ObjectPathArrayIndex").set_body_method<ObjectPath>(&ObjectPathNode::ArrayIndex);

// --- MissingArrayElement ---
ObjectPath ObjectPathNode::MissingArrayElement(int32_t index) const {
    return ObjectPath(make_object<MissingArrayElementPathNode>(this, index));
}
TVM_REGISTER_GLOBAL("node.ObjectPathMissingArrayElement").set_body_method<ObjectPath>(&ObjectPathNode::MissingArrayElement);

// --- MapValue ---
ObjectPath ObjectPathNode::MapValue(ObjectRef key) const {
    return ObjectPath(make_object<MapValuePathNode>(this, std::move(key)));
}

TVM_REGISTER_GLOBAL("node.ObjectPathMapValue").set_body_method<ObjectPath>(&ObjectPathNode::MapValue);

// --- MissingMapEntry ---
ObjectPath ObjectPathNode::MissingMapEntry() const {
    return ObjectPath(make_object<MissingMapEntryPathNode>(this));
}

TVM_REGISTER_GLOBAL("node.ObjectPathMissingMapEntry").set_body_method<ObjectPath>(&ObjectPathNode::MissingMapEntry);

// --- PathsEqual ----
bool ObjectPathNode::PathsEqual(const ObjectPath& other) const {
    if (!other.defined() || Length() != other->Length()) {
        return false;
    }

    const ObjectPathNode* lhs = this;
    const ObjectPathNode* rhs = other.get();

    while (lhs && rhs) {
        if (lhs->type_index() != rhs->type_index()) {
            return false;
        }

        if (!lhs->LastNodeEqual(rhs)) {
            return false;
        }

        lhs = lhs->ParentNode();
        rhs = rhs->ParentNode();
    }

    return lhs == nullptr && rhs == nullptr;
}
TVM_REGISTER_GLOBAL("node.ObjectPathEqual").set_body_method<ObjectPath>(&ObjectPathNode::PathsEqual);

// --- Repr ---
std::string GetObjectPathRepr(const ObjectPathNode* node) {
    std::string ret;
    while (node != nullptr) {
        std::string node_str = node->LastNodeString();
        ret.append(node_str.rbegin(), node_str.rend());
        node = node->GetParent().get();
    }
    std::reverse(ret.begin(), ret.end());
    return ret;
}

static void PrintObjectPathRepr(const ObjectRef& node, ReprPrinter* p) {
    p->stream << GetObjectPathRepr(static_cast<const ObjectPathNode*>(node.get()));
}
TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable).set_dispatch<ObjectPathNode>(PrintObjectPathRepr);

// --- Private/protected methods ---
const ObjectPathNode* ObjectPathNode::ParentNode() const {
    return static_cast<const ObjectPathNode*>(parent_.get());
}

// ============== ObjectPath ==============
ObjectPath ObjectPath::Root(const Optional<String>& name) {
    return ObjectPath(make_object<RootPathNode>(name));
}

TVM_REGISTER_GLOBAL("node.ObjectPathRoot").set_body_typed(ObjectPath::Root);

// ============== Individual path classes ==============

// ----- Root -----
RootPathNode::RootPathNode(const Optional<String>& name) : ObjectPathNode(nullptr), name(name) {}

bool RootPathNode::LastNodeEqual(const ObjectPathNode* other_path) const {
    const auto* other = static_cast<const RootPathNode*>(other_path);

    if (other->name.defined() != name.defined()) {
        return false;
    }

    if (name && other->name) {
        return name.value() == other->name.value();
    }

    return true;
}

std::string RootPathNode::LastNodeString() const {
    return name.value_or("<root>");
}

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable).set_dispatch<RootPathNode>(PrintObjectPathRepr);

// ----- AttributeAccess -----
AttributeAccessPathNode::AttributeAccessPathNode(const ObjectPathNode* parent, String attr_key)
    : ObjectPathNode(parent), attr_key(std::move(attr_key)) {}

bool AttributeAccessPathNode::LastNodeEqual(const ObjectPathNode* other) const {
    const auto* otherAttrAccess = static_cast<const AttributeAccessPathNode*>(other);
    return attr_key == otherAttrAccess->attr_key;
}

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable).set_dispatch<AttributeAccessPathNode>(PrintObjectPathRepr);

// ----- UnknownAttributeAccess -----
UnknownAttributeAccessPathNode::UnknownAttributeAccessPathNode(const ObjectPathNode* parent)
    : ObjectPathNode(parent) {}

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable).set_dispatch<UnknownAttributeAccessPathNode>(PrintObjectPathRepr);

// ----- ArrayIndexPath -----
ArrayIndexPathNode::ArrayIndexPathNode(const ObjectPathNode* parent, int32_t index)
    : ObjectPathNode(parent), index(index) {}

bool ArrayIndexPathNode::LastNodeEqual(const ObjectPathNode* other) const {
    const auto* otherArrayIndex = static_cast<const ArrayIndexPathNode*>(other);
    return index == otherArrayIndex->index;
}

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable).set_dispatch<ArrayIndexPathNode>(PrintObjectPathRepr);

// ----- MissingArrayElement -----
MissingArrayElementPathNode::MissingArrayElementPathNode(const ObjectPathNode* parent,
                                                         int32_t index)
    : ObjectPathNode(parent), index(index) {}

bool MissingArrayElementPathNode::LastNodeEqual(const ObjectPathNode* other) const {
    const auto* otherMissingElement = static_cast<const MissingArrayElementPathNode*>(other);
    return index == otherMissingElement->index;
}

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable).set_dispatch<MissingArrayElementPathNode>(PrintObjectPathRepr);

// ----- MapValue -----
MapValuePathNode::MapValuePathNode(const ObjectPathNode* parent, ObjectRef key)
    : ObjectPathNode(parent), key(std::move(key)) {}

bool MapValuePathNode::LastNodeEqual(const ObjectPathNode* other) const {
    const auto* otherMapValue = static_cast<const MapValuePathNode*>(other);
    return ObjectEqual()(key, otherMapValue->key);
}

std::string MapValuePathNode::LastNodeString() const {
    std::ostringstream s;
    s << "[" << key << "]";
    return s.str();
}

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable).set_dispatch<MapValuePathNode>(PrintObjectPathRepr);

// ----- MissingMapEntry -----
MissingMapEntryPathNode::MissingMapEntryPathNode(const ObjectPathNode* parent)
    : ObjectPathNode(parent) {}

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable).set_dispatch<MissingMapEntryPathNode>(PrintObjectPathRepr);


}// namespace litetvm
