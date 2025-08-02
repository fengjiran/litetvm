//
// Created by richard on 8/2/25.
//

#ifndef LITETVM_NODE_ATTR_REGISTRY_MAP_H
#define LITETVM_NODE_ATTR_REGISTRY_MAP_H

#include "ffi/string.h"

#include <utility>
#include <vector>

namespace litetvm {

/*!
 * \brief Generic attribute map.
 * \tparam KeyType the type of the key.
 */
template<typename KeyType>
class AttrRegistryMapContainerMap {
public:
    /*!
   * \brief Check if the map has key.
   * \param key The key to the map
   * \return 1 if key is contained in map, 0 otherwise.
   */
    int count(const KeyType& key) const {
        if (key.defined()) {
            const uint32_t idx = key->AttrRegistryIndex();
            return idx < data_.size() ? (data_[idx].second != 0) : 0;
        } else {
            return 0;
        }
    }
    /*!
   * \brief get the corresponding value element at key.
   * \param key The key to the map
   * \return the const reference to the content value.
   */
    const ffi::Any& operator[](const KeyType& key) const {
        ICHECK(key.defined());
        const uint32_t idx = key->AttrRegistryIndex();
        ICHECK(idx < data_.size() && data_[idx].second != 0)
                << "Attribute " << attr_name_ << " has not been registered for " << key->name;
        return data_[idx].first;
    }
    /*!
   * \brief get the corresponding value element at key with default value.
   * \param key The key to the map
   * \param def_value The default value when the key does not exist.
   * \return the const reference to the content value.
   * \tparam ValueType The content value type.
   */
    template<typename ValueType>
    ValueType get(const KeyType& key, ValueType def_value) const {
        ICHECK(key.defined());
        const uint32_t idx = key->AttrRegistryIndex();
        if (idx < data_.size() && data_[idx].second != 0) {
            if constexpr (std::is_same_v<ValueType, ffi::Any>) {
                return data_[idx].first;
            } else {
                return data_[idx].first.template cast<ValueType>();
            }
        } else {
            return def_value;
        }
    }

private:
    /*! \brief The name of the attr field */
    String attr_name_;
    /*! \brief The internal data. */
    std::vector<std::pair<ffi::Any, int>> data_;
    /*! \brief The constructor */
    AttrRegistryMapContainerMap() = default;
    template<typename, typename>
    friend class AttrRegistry;
    friend class OpRegEntry;
};

/*!
 * \brief Map<Key, ValueType> used to store meta-data.
 * \tparam KeyType The type of the key
 * \tparam ValueType The type of the value stored in map.
 */
template<typename KeyType, typename ValueType>
class AttrRegistryMap {
public:
    /*!
   * \brief constructor
   * \param map The internal map.
   */
    explicit AttrRegistryMap(const AttrRegistryMapContainerMap<KeyType>& map) : map_(map) {}
    /*!
   * \brief Check if the map has op as key.
   * \param key The key to the map
   * \return 1 if op is contained in map, 0 otherwise.
   */
    int count(const KeyType& key) const { return map_.count(key); }
    /*!
   * \brief get the corresponding value element at key.
   * \param key The key to the map
   * \return the const reference to the content value.
   */
    ValueType operator[](const KeyType& key) const {
        if constexpr (std::is_same_v<ValueType, ffi::Any>) {
            return map_[key];
        } else {
            return map_[key].template cast<ValueType>();
        }
    }
    /*!
   * \brief get the corresponding value element at key with default value.
   * \param key The key to the map
   * \param def_value The default value when the key does not exist.
   * \return the const reference to the content value.
   */
    ValueType get(const KeyType& key, ValueType def_value) const { return map_.get(key, def_value); }

protected:
    /*! \brief The internal map field */
    const AttrRegistryMapContainerMap<KeyType>& map_;
};

}// namespace litetvm

#endif//LITETVM_NODE_ATTR_REGISTRY_MAP_H
