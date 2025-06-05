//
// Created by 赵丹 on 25-6-5.
//

#ifndef LITETVM_FFI_CONTAINER_TUPLE_H
#define LITETVM_FFI_CONTAINER_TUPLE_H

#include "ffi/container/array.h"

#include <string>
#include <tuple>
#include <utility>

namespace litetvm {
namespace ffi {

/*!
 * \brief Typed tuple like std::tuple backed by ArrayObj container.
 *
 * Tuple implements in-place copy-on-write semantics.
 *
 * \tparam Types The types of the tuple elements
 */
template<typename... Types>
class Tuple : public ObjectRef {
public:
    static_assert(details::all_storage_enabled_v<Types...>, "All types used in Tuple<...> must be compatible with Any");

    Tuple() : ObjectRef(MakeDefaultTupleNode()) {}
    Tuple(const Tuple& other) : ObjectRef(other) {}
    Tuple(Tuple&& other) noexcept : ObjectRef(std::move(other)) {}
    template<typename... UTypes,
             typename = std::enable_if_t<(details::type_contains_v<Types, UTypes> && ...), int>>
    Tuple(const Tuple<UTypes...>& other) : ObjectRef(other) {}
    template<typename... UTypes,
             typename = std::enable_if_t<(details::type_contains_v<Types, UTypes> && ...), int>>
    Tuple(Tuple<UTypes...>&& other) : ObjectRef(std::move(other)) {}

    template<typename... UTypes,
             typename = std::enable_if_t<sizeof...(Types) == sizeof...(UTypes) && !(sizeof...(Types) == 1 &&
                                                                                    (std::is_same_v<std::remove_cv_t<UTypes>, Tuple<Types>> && ...))>>
    explicit Tuple(UTypes&&... args) : ObjectRef(MakeTupleNode(std::forward<UTypes>(args)...)) {}

    TVM_FFI_INLINE Tuple& operator=(const Tuple& other) {
        data_ = other.data_;
        return *this;
    }

    TVM_FFI_INLINE Tuple& operator=(Tuple&& other) noexcept {
        data_ = std::move(other.data_);
        return *this;
    }

    template<typename... UTypes,
             typename = std::enable_if_t<(details::type_contains_v<Types, UTypes> && ...)>>
    TVM_FFI_INLINE Tuple& operator=(const Tuple<UTypes...>& other) {
        data_ = other.data_;
        return *this;
    }

    template<typename... UTypes,
             typename = std::enable_if_t<(details::type_contains_v<Types, UTypes> && ...)>>
    TVM_FFI_INLINE Tuple& operator=(Tuple<UTypes...>&& other) {
        data_ = std::move(other.data_);
        return *this;
    }

    explicit Tuple(ObjectPtr<Object> n) : ObjectRef(n) {}

    /*!
   * \brief Get I-th element of the tuple
   *
   * \tparam I The index of the element to get
   * \return The I-th element of the tuple
   * \note We use stl style since get usually is like a getter.
   */
    template<size_t I>
    auto get() const {
        static_assert(I < sizeof...(Types), "Tuple index out of bounds");
        using ReturnType = std::tuple_element_t<I, std::tuple<Types...>>;
        const Any* ptr = GetArrayObj()->begin() + I;
        return details::AnyUnsafe::CopyFromAnyViewAfterCheck<ReturnType>(*ptr);
    }

    /*!
   * \brief Set I-th element of the tuple
   *
   * \param item The item to set
   * \tparam I The index of the element to set
   * \tparam U The type of the item
   *
   * \note This function will perform copy on write if underlying
   *       container is not uniquely owned.
   *       We use CamelCase since Set can cause copy on write
   *       and is more complicated than simple field setter.
   */
    template<size_t I, typename U>
    void Set(U&& item) {
        static_assert(I < sizeof...(Types), "Tuple index out of bounds");
        using T = std::tuple_element_t<I, std::tuple<Types...>>;
        this->CopyIfNotUnique();
        Any* ptr = GetArrayObj()->MutableBegin() + I;
        *ptr = T(std::forward<U>(item));
    }

    /*! \brief specify container node */
    using ContainerType = ArrayObj;

private:
    static ObjectPtr<ArrayObj> MakeDefaultTupleNode() {
        ObjectPtr<ArrayObj> p = make_inplace_array_object<ArrayObj, Any>(sizeof...(Types));
        p->capacity_ = sizeof...(Types);
        // immeidate set size to 0, to ensure exception safety
        p->size_ = 0;
        Any* itr = p->MutableBegin();
        // increase size after each new to ensure exception safety
        ((new (itr++) Any(Types()), p->size_++), ...);
        return p;
    }

    template<typename... UTypes>
    static ObjectPtr<ArrayObj> MakeTupleNode(UTypes&&... args) {
        ObjectPtr<ArrayObj> p = make_inplace_array_object<ArrayObj, Any>(sizeof...(Types));
        p->capacity_ = sizeof...(Types);
        // immeidate set size to 0, to ensure exception safety
        p->size_ = 0;
        Any* itr = p->MutableBegin();
        // increase size after each new to ensure exception safety
        ((new (itr++) Any(Types(std::forward<UTypes>(args))), p->size_++), ...);
        return p;
    }

    /*! \brief Copy on write */
    void CopyIfNotUnique() {
        if (!data_.unique()) {
            ObjectPtr<ArrayObj> p = make_inplace_array_object<ArrayObj, Any>(sizeof...(Types));
            p->capacity_ = sizeof...(Types);
            // immeidate set size to 0, to ensure exception safety
            p->size_ = 0;
            Any* itr = p->MutableBegin();
            const Any* read = GetArrayObj()->begin();
            // increase size after each new to ensure exception safety
            for (size_t i = 0; i < sizeof...(Types); ++i) {
                new (itr++) Any(*read++);
                p->size_++;
            }
            data_ = std::move(p);
        }
    }

    /*! \return The underlying ArrayObj */
    NODISCARD ArrayObj* GetArrayObj() const {
        return static_cast<ArrayObj*>(data_.get());
    }

    template<typename... UTypes>
    friend class Tuple;
};

}// namespace ffi
}// namespace litetvm

#endif//LITETVM_FFI_CONTAINER_TUPLE_H
