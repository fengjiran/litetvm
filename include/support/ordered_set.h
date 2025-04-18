//
// Created by 赵丹 on 25-4-18.
//

#ifndef LITETVM_SUPPORT_ORDERED_SET_H
#define LITETVM_SUPPORT_ORDERED_SET_H

#include "runtime/object.h"

#include <list>
#include <unordered_map>

namespace litetvm {
namespace support {

namespace detail {
/* \brief Utility to allow use for standard and ObjectRef types
 *
 * \tparam T The type held by the OrderedSet
 */
template<typename T,
         typename = void>
struct OrderedSetLookupType {
    using MapType = std::unordered_map<T, typename std::list<T>::iterator>;
};

template<typename T>
struct OrderedSetLookupType<T, std::enable_if_t<std::is_base_of_v<runtime::ObjectRef, T>>> {
    using MapType = std::unordered_map<T, typename std::list<T>::iterator, runtime::ObjectPtrHash, runtime::ObjectPtrEqual>;
};
}// namespace detail

template<typename T>
class OrderedSet {
public:
    OrderedSet() = default;

    /* \brief Explicit copy constructor
   *
   * The default copy constructor would copy both `elements_` and
   * `elem_to_iter_`.  While this is the correct behavior for
   * `elements_`, the copy of `elem_to_iter_` would contain references
   * to the original's `element_`, rather than to its own
   */
    OrderedSet(const OrderedSet& other) : elements_(other.elements_) {
        InitElementToIter();
    }

    /* \brief Explicit copy assignment
   *
   * Implemented in terms of the copy constructor, and the default
   * move assignment.
   */
    OrderedSet& operator=(const OrderedSet& other) {
        OrderedSet tmp(other);
        swap(*this, tmp);
        return *this;
    }

    OrderedSet(OrderedSet&&) = default;
    OrderedSet& operator=(OrderedSet&&) = default;

    template<typename Iter>
    OrderedSet(Iter begin, Iter end) : elements_(begin, end) {
        InitElementToIter();
    }

    void push_back(const T& t) {
        if (!elem_to_iter_.count(t)) {
            elements_.push_back(t);
            elem_to_iter_[t] = std::prev(elements_.end());
        }
    }

    void insert(const T& t) {
        push_back(t);
    }

    void erase(const T& t) {
        if (auto it = elem_to_iter_.find(t); it != elem_to_iter_.end()) {
            elements_.erase(it->second);
            elem_to_iter_.erase(it);
        }
    }

    void clear() {
        elements_.clear();
        elem_to_iter_.clear();
    }

    size_t count(const T& t) const { return elem_to_iter_.count(t); }

    auto begin() const { return elements_.begin(); }
    auto end() const { return elements_.end(); }
    auto size() const { return elements_.size(); }
    auto empty() const { return elements_.empty(); }

private:
    void InitElementToIter() {
        for (auto it = elements_.begin(); it != elements_.end(); ++it) {
            elem_to_iter_[*it] = it;
        }
    }

    friend void swap(OrderedSet& lhs, OrderedSet& rhs) noexcept {
        using std::swap;
        swap(lhs.elements_, rhs.elements_);
        swap(lhs.elem_to_iter_, rhs.elem_to_iter_);
    }

    std::list<T> elements_;
    typename detail::OrderedSetLookupType<T>::MapType elem_to_iter_;
};

}// namespace support
}// namespace litetvm

#endif//LITETVM_SUPPORT_ORDERED_SET_H
