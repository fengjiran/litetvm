//
// Created by 赵丹 on 25-3-17.
//

#ifndef LITETVM_TARGET_KIND_H
#define LITETVM_TARGET_KIND_H

#include "node/attr_registry_map.h"
#include "node/reflection.h"
#include "runtime/array.h"
#include "runtime/map.h"
#include "runtime/object.h"
#include "runtime/packed_func.h"
#include "runtime/string.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace litetvm {

using runtime::Object;
using runtime::ObjectRef;
using runtime::PackedFunc;
using runtime::String;
using runtime::TypedPackedFunc;

class Target;

/*!
 * \brief Map containing parsed features of a specific Target
 */
using TargetFeatures = Map<String, ObjectRef>;

/*!
 * \brief TargetParser to apply on instantiation of a given TargetKind
 *
 * \param target_json Target in JSON format to be transformed during parsing.
 *
 * \return The transformed Target JSON object.
 */
using TargetJSON = Map<String, ObjectRef>;
using FTVMTargetParser = TypedPackedFunc<TargetJSON(TargetJSON)>;

namespace detail {
template<typename, typename, typename>
struct ValueTypeInfoMaker;
}

class TargetInternal;

template<typename>
class TargetKindAttrMap;

/*! \brief Target kind, specifies the kind of the target */
class TargetKindNode : public Object {
public:
    /*! \brief Name of the target kind */
    String name;
    /*! \brief Device type of target kind */
    int default_device_type;
    /*! \brief Default keys of the target */
    Array<String> default_keys;
    /*! \brief Function used to preprocess on target creation */
    PackedFunc preprocessor;
    /*! \brief Function used to parse a JSON target during creation */
    FTVMTargetParser target_parser;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("name", &name);
        v->Visit("default_device_type", &default_device_type);
        v->Visit("default_keys", &default_keys);
    }

    static constexpr const char* _type_key = "TargetKind";
    TVM_DECLARE_FINAL_OBJECT_INFO(TargetKindNode, Object);

private:
    /*! \brief Return the index stored in attr registry */
    uint32_t AttrRegistryIndex() const {
        return index_;
    }

    /*! \brief Return the name stored in attr registry */
    String AttrRegistryName() const {
        return name;
    }

    /*! \brief Stores the required type_key and type_index of a specific attr of a target */
    struct ValueTypeInfo {
        String type_key;
        uint32_t type_index;
        std::unique_ptr<ValueTypeInfo> key;
        std::unique_ptr<ValueTypeInfo> val;
    };

    /*! \brief A hash table that stores the type information of each attr of the target key */
    std::unordered_map<String, ValueTypeInfo> key2vtype_;

    /*! \brief A hash table that stores the default value of each attr of the target key */
    std::unordered_map<String, ObjectRef> key2default_;

    /*! \brief Index used for internal lookup of attribute registry */
    uint32_t index_;

    template<typename, typename, typename>
    friend struct detail::ValueTypeInfoMaker;
    template<typename, typename>
    friend class AttrRegistry;
    template<typename>
    friend class AttrRegistryMapContainerMap;
    friend class TargetKindRegEntry;
    friend class TargetInternal;
};

/*!
 * \brief Managed reference class to TargetKindNode
 * \sa TargetKindNode
 */
class TargetKind : public ObjectRef {
public:
    TargetKind() = default;
    /*! \brief Get the attribute map given the attribute name */
    template<typename ValueType>
    static TargetKindAttrMap<ValueType> GetAttrMap(const String& attr_name);

    /*!
     * \brief Retrieve the TargetKind given its name
     * \param target_kind_name Name of the target kind
     * \return The TargetKind requested
     */
    LITETVM_API static Optional<TargetKind> Get(const String& target_kind_name);
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(TargetKind, ObjectRef, TargetKindNode);

    /*! \brief Mutable access to the container class  */
    TargetKindNode* operator->() {
        return static_cast<TargetKindNode*>(data_.get());
    }

private:
    LITETVM_API static const AttrRegistryMapContainerMap<TargetKind>& GetAttrMapContainer(const String& attr_name);
    friend class TargetKindRegEntry;
    friend class TargetInternal;
};

/*!
 * \brief Map<TargetKind, ValueType> used to store meta-information about TargetKind
 * \tparam ValueType The type of the value stored in map
 */
template<typename ValueType>
class TargetKindAttrMap : public AttrRegistryMap<TargetKind, ValueType> {
public:
    using TParent = AttrRegistryMap<TargetKind, ValueType>;
    using TParent::count;
    using TParent::get;
    using TParent::operator[];
    explicit TargetKindAttrMap(const AttrRegistryMapContainerMap<TargetKind>& map) : TParent(map) {}
};

/*! \brief Value used with --runtime in target specs to indicate the C++ runtime. */
static constexpr const char* kTvmRuntimeCpp = "c++";

/*! \brief Value used with --runtime in target specs to indicate the C runtime. */
static constexpr const char* kTvmRuntimeCrt = "c";


template<typename ValueType>
TargetKindAttrMap<ValueType> TargetKind::GetAttrMap(const String& attr_name) {
    return TargetKindAttrMap<ValueType>(GetAttrMapContainer(attr_name));
}

}// namespace litetvm

#endif//LITETVM_TARGET_KIND_H
