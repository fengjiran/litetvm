//
// Created by 赵丹 on 25-3-17.
//

#ifndef LITETVM_NODE_ATTR_REGISTRY_H
#define LITETVM_NODE_ATTR_REGISTRY_H

#include "ffi/function.h"
#include "node/attr_registry_map.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace litetvm {

/*!
 * \brief Implementation of registry with attributes.
 *
 * \tparam EntryType The type of the registry entry.
 * \tparam KeyType The actual key that is used to lookup the attributes.
 *                 each entry has a corresponding key by default.
 */
template<typename EntryType, typename KeyType>
class AttrRegistry {
public:
    using TSelf = AttrRegistry<EntryType, KeyType>;
    /*!
   * \brief Get an entry from the registry.
   * \param name The name of the item.
   * \return The corresponding entry.
   */
    const EntryType* Get(const String& name) const {
        auto it = entry_map_.find(name);
        if (it != entry_map_.end()) return it->second;
        return nullptr;
    }

    /*!
   * \brief Get an entry or register a new one.
   * \param name The name of the item.
   * \return The corresponding entry.
   */
    EntryType& RegisterOrGet(const String& name) {
        auto it = entry_map_.find(name);
        if (it != entry_map_.end()) return *it->second;
        uint32_t registry_index = static_cast<uint32_t>(entries_.size());
        auto entry = std::unique_ptr<EntryType>(new EntryType(registry_index));
        auto* eptr = entry.get();
        eptr->name = name;
        entry_map_[name] = eptr;
        entries_.emplace_back(std::move(entry));
        return *eptr;
    }

    /*!
   * \brief List all the entry names in the registry.
   * \return The entry names.
   */
    Array<String> ListAllNames() const {
        Array<String> names;
        for (const auto& kv: entry_map_) {
            names.push_back(kv.first);
        }
        return names;
    }

    /*!
   * \brief Update the attribute stable.
   * \param attr_name The name of the attribute.
   * \param key The key to the attribute table.
   * \param value The value to be set.
   * \param plevel The support level.
   */
    void UpdateAttr(const String& attr_name, const KeyType& key, Any value, int plevel) {
        using ffi::Any;
        auto& op_map = attrs_[attr_name];
        if (op_map == nullptr) {
            op_map.reset(new AttrRegistryMapContainerMap<KeyType>());
            op_map->attr_name_ = attr_name;
        }

        uint32_t index = key->AttrRegistryIndex();
        if (op_map->data_.size() <= index) {
            op_map->data_.resize(index + 1, std::make_pair(Any(), 0));
        }
        std::pair<ffi::Any, int>& p = op_map->data_[index];
        ICHECK(p.second != plevel) << "Attribute " << attr_name << " of " << key->AttrRegistryName()
                                   << " is already registered with same plevel=" << plevel;
        ICHECK(value != nullptr) << "Registered packed_func is Null for " << attr_name
                                 << " of operator " << key->AttrRegistryName();
        if (p.second < plevel && value != nullptr) {
            op_map->data_[index] = std::make_pair(value, plevel);
        }
    }

    /*!
   * \brief Reset an attribute table entry.
   * \param attr_name The name of the attribute.
   * \param key The key to the attribute table.
   */
    void ResetAttr(const String& attr_name, const KeyType& key) {
        auto& op_map = attrs_[attr_name];
        if (op_map == nullptr) {
            return;
        }
        uint32_t index = key->AttrRegistryIndex();
        if (op_map->data_.size() > index) {
            op_map->data_[index] = std::make_pair(ffi::Any(), 0);
        }
    }

    /*!
   * \brief Get an internal attribute map.
   * \param attr_name The name of the attribute.
   * \return The result attribute map.
   */
    const AttrRegistryMapContainerMap<KeyType>& GetAttrMap(const String& attr_name) {
        auto it = attrs_.find(attr_name);
        if (it == attrs_.end()) {
            LOG(FATAL) << "Attribute \'" << attr_name << "\' is not registered";
        }
        return *it->second.get();
    }

    /*!
   * \brief Check of attribute has been registered.
   * \param attr_name The name of the attribute.
   * \return The check result.
   */
    bool HasAttrMap(const String& attr_name) { return attrs_.count(attr_name); }

    /*!
   * \return a global singleton of the registry.
   */
    static TSelf* Global() {
        static TSelf* inst = new TSelf();
        return inst;
    }

private:
    // entries in the registry
    std::vector<std::unique_ptr<EntryType>> entries_;
    // map from name to entries.
    std::unordered_map<String, EntryType*> entry_map_;
    // storage of additional attribute table.
    std::unordered_map<String, std::unique_ptr<AttrRegistryMapContainerMap<KeyType>>> attrs_;
};

}// namespace litetvm


#endif//LITETVM_NODE_ATTR_REGISTRY_H
