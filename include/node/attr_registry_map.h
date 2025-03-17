//
// Created by 赵丹 on 25-3-14.
//

#ifndef LITETVM_NODE_ATTR_REGISTRY_MAP_H
#define LITETVM_NODE_ATTR_REGISTRY_MAP_H

#include "runtime/string.h"
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
   * \return 1 if key is contained in a map, 0 otherwise.
   */
    int count(const KeyType& key) const {
        if (key.defined()) {
            const uint32_t idx = key->AttrRegistryIndex();
            return idx < data_.size() ? data_[idx].second != 0 : 0;
        }
        return 0;
    }

    /*!
   * \brief get the corresponding value element at a key.
   * \param key The key to the map
   * \return the const reference to the content value.
   */
    const TVMRetValue& operator[](const KeyType& key) const {
        CHECK(key.defined());
        const uint32_t idx = key->AttrRegistryIndex();
        CHECK(idx < data_.size() && data_[idx].second != 0)
                << "Attribute " << attr_name_ << " has not been registered for " << key->name;
        return data_[idx].first;
    }

    /*!
   * \brief get the corresponding value element at a key with default value.
   * \param key The key to the map
   * \param def_value The default value when the key does not exist.
   * \return the const reference to the content value.
   * \tparam ValueType The content value type.
   */
    template<typename ValueType>
    ValueType get(const KeyType& key, ValueType def_value) const {
        CHECK(key.defined());
        const uint32_t idx = key->AttrRegistryIndex();
        if (idx < data_.size() && data_[idx].second != 0) {
            return data_[idx].first;
        }
        return def_value;
    }

private:
    /*! \brief The name of the attr field */
    String attr_name_;
    /*! \brief The internal data. */
    std::vector<std::pair<TVMRetValue, int>> data_;
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
     * \brief Check if the map has op as a key.
     * \param key The key to the map
     * \return 1 if op is contained in a map, 0 otherwise.
     */
    int count(const KeyType& key) const {
        return map_.count(key);
    }

    /*!
     * \brief get the corresponding value element at a key.
     * \param key The key to the map
     * \return the const reference to the content value.
     */
    ValueType operator[](const KeyType& key) const {
        return map_[key];
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
