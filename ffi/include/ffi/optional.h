//
// Created by richard on 5/15/25.
//

#ifndef LITETVM_FFI_OPTIONAL_H
#define LITETVM_FFI_OPTIONAL_H

#include "ffi/error.h"
#include "ffi/object.h"

#include <optional>
#include <string>
#include <utility>

namespace litetvm {
namespace ffi {

// Note: We place optional in tvm/ffi instead of tvm/ffi/container
// because optional itself is an inherent core component of the FFI system.

template<typename T>
inline constexpr bool is_optional_type_v = false;

template<typename T>
inline constexpr bool is_optional_type_v<Optional<T>> = true;

// we can safely use ptr based optional for ObjectRef types
// that do not have additional data members and virtual functions.
template<typename T>
inline constexpr bool use_ptr_based_optional_v = std::is_base_of_v<ObjectRef, T> && !is_optional_type_v<T>;

// Specialization for non-ObjectRef types.
// simply fallback to std::optional
template<typename T>
class Optional<T, std::enable_if_t<!use_ptr_based_optional_v<T>>> {
public:
    // default constructors.
    Optional() = default;
    Optional(const Optional<T>& other) : data_(other.data_) {}
    Optional(Optional<T>&& other) : data_(std::move(other.data_)) {}
    Optional(std::optional<T> other) : data_(std::move(other)) {}// NOLINT(*)
    Optional(std::nullopt_t) {}                                  // NOLINT(*)
    // normal value handling.
    Optional(T other) : data_(std::move(other)) {}

    TVM_FFI_INLINE Optional& operator=(const Optional& other) {
        data_ = other.data_;
        return *this;
    }

    TVM_FFI_INLINE Optional& operator=(Optional&& other) noexcept {
        data_ = std::move(other.data_);
        return *this;
    }

    TVM_FFI_INLINE Optional& operator=(T other) {
        data_ = std::move(other);
        return *this;
    }

    TVM_FFI_INLINE Optional& operator=(std::nullopt_t) {
        data_ = std::nullopt;
        return *this;
    }

    TVM_FFI_INLINE const T& value() const& {
        if (!data_.has_value()) {
            TVM_FFI_THROW(RuntimeError) << "Back optional access";
        }
        return *data_;
    }

    TVM_FFI_INLINE T&& value() && {
        if (!data_.has_value()) {
            TVM_FFI_THROW(RuntimeError) << "Back optional access";
        }
        return *std::move(data_);
    }

    template<typename U = std::remove_cv_t<T>>
    TVM_FFI_INLINE T value_or(U&& default_value) const {
        return data_.value_or(std::forward<U>(default_value));
    }

    TVM_FFI_INLINE explicit operator bool() const noexcept {
        return data_.has_value();
    }

    NODISCARD TVM_FFI_INLINE bool has_value() const noexcept {
        return data_.has_value();
    }

    TVM_FFI_INLINE bool operator==(const Optional<T>& other) const {
        return data_ == other.data_;
    }

    TVM_FFI_INLINE bool operator!=(const Optional<T>& other) const {
        return data_ != other.data_;
    }

    template<typename U>
    TVM_FFI_INLINE bool operator==(const U& other) const {
        return data_ == other;
    }

    template<typename U>
    TVM_FFI_INLINE bool operator!=(const U& other) const {
        return data_ != other;
    }

    /*!
   * \brief Direct access to the value.
   * \return the xvalue reference to the stored value.
   * \note only use this function after checking has_value()
   */
    TVM_FFI_INLINE T&& operator*() && noexcept {
        return *std::move(data_);
    }

    /*!
   * \brief Direct access to the value.
   * \return the const reference to the stored value.
   * \note only use this function  after checking has_value()
   */
    TVM_FFI_INLINE const T& operator*() const& noexcept {
        return *data_;
    }

private:
    std::optional<T> data_;
};

// Specialization for ObjectRef types.
// nullptr is treated as std::nullopt.
template<typename T>
class Optional<T, std::enable_if_t<use_ptr_based_optional_v<T>>> : public ObjectRef {
public:
    using ContainerType = typename T::ContainerType;
    Optional() = default;
    Optional(const Optional& other) : ObjectRef(other.data_) {}
    Optional(Optional&& other) noexcept : ObjectRef(std::move(other.data_)) {}
    explicit Optional(ObjectPtr<Object> ptr) : ObjectRef(std::move(ptr)) {}
    // nullopt hanlding
    Optional(std::nullopt_t) {}// NOLINT(*)

    // handle conversion from std::optional<T>
    Optional(std::optional<T> other) {// NOLINT(*)
        if (other.has_value()) {
            *this = *std::move(other);
        }
    }

    // normal value handling.
    Optional(T other) : ObjectRef(std::move(other)) {}

    TVM_FFI_INLINE Optional& operator=(T other) {
        ObjectRef::operator=(std::move(other));
        return *this;
    }

    TVM_FFI_INLINE Optional& operator=(const Optional& other) {
        data_ = other.data_;
        return *this;
    }

    TVM_FFI_INLINE Optional& operator=(std::nullptr_t) {
        data_ = nullptr;
        return *this;
    }

    TVM_FFI_INLINE Optional& operator=(Optional&& other) noexcept {
        data_ = std::move(other.data_);
        return *this;
    }

    TVM_FFI_INLINE T value() const& {
        if (data_ == nullptr) {
            TVM_FFI_THROW(RuntimeError) << "Back optional access";
        }
        return T(data_);
    }

    TVM_FFI_INLINE T value() && {
        if (data_ == nullptr) {
            TVM_FFI_THROW(RuntimeError) << "Back optional access";
        }
        return T(std::move(data_));
    }

    template<typename U = std::remove_cv_t<T>>
    TVM_FFI_INLINE T value_or(U&& default_value) const {
        return data_ != nullptr ? T(data_) : T(std::forward<U>(default_value));
    }

    TVM_FFI_INLINE explicit operator bool() const {
        return data_ != nullptr;
    }

    TVM_FFI_INLINE bool has_value() const {
        return data_ != nullptr;
    }

    /*!
   * \brief Direct access to the value.
   * \return the const reference to the stored value.
   * \note only use this function after checking has_value()
   */
    TVM_FFI_INLINE T operator*() const& noexcept {
        return T(data_);
    }

    /*!
   * \brief Direct access to the value.
   * \return the const reference to the stored value.
   * \note only use this function  after checking has_value()
   */
    TVM_FFI_INLINE T operator*() && noexcept {
        return T(std::move(data_));
    }

    TVM_FFI_INLINE bool operator==(std::nullptr_t) const noexcept {
        return !has_value();
    }

    TVM_FFI_INLINE bool operator!=(std::nullptr_t) const noexcept {
        return has_value();
    }

    // operator overloadings
    TVM_FFI_INLINE auto operator==(const Optional& other) const {
        // support case where sub-class returns a symbolic ref type.
        return EQToOptional(other);
    }
    TVM_FFI_INLINE auto operator!=(const Optional& other) const {
        return NEToOptional(other);
    }

    TVM_FFI_INLINE auto operator==(const std::optional<T>& other) const {
        // support case where sub-class returns a symbolic ref type.
        return EQToOptional(other);
    }

    TVM_FFI_INLINE auto operator!=(const std::optional<T>& other) const {
        return NEToOptional(other);
    }

    TVM_FFI_INLINE auto operator==(const T& other) const {
        using RetType = decltype(value() == other);
        if (same_as(other)) return RetType(true);
        if (has_value()) return operator*() == other;
        return RetType(false);
    }

    TVM_FFI_INLINE auto operator!=(const T& other) const {
        return !(*this == other);
    }

    template<typename U>
    TVM_FFI_INLINE auto operator==(const U& other) const {
        using RetType = decltype(value() == other);
        if (!has_value()) return RetType(false);
        return operator*() == other;
    }

    template<typename U>
    TVM_FFI_INLINE auto operator!=(const U& other) const {
        using RetType = decltype(value() != other);
        if (!has_value()) return RetType(true);
        return operator*() != other;
    }

    /*!
   * \return The internal object pointer with container type of T.
   * \note This function do not perform not-null checking.
   */
    TVM_FFI_INLINE const ContainerType* get() const {
        return static_cast<ContainerType*>(data_.get());
    }

private:
    template<typename U>
    TVM_FFI_INLINE auto EQToOptional(const U& other) const {
        // support case where sub-class returns a symbolic ref type.
        using RetType = decltype(operator*() == *other);
        if (same_as(other)) return RetType(true);
        if (has_value() && other.has_value()) {
            return operator*() == *other;
        }
        // one of them is nullptr.
        return RetType(false);
    }

    template<typename U>
    TVM_FFI_INLINE auto NEToOptional(const U& other) const {
        // support case where sub-class returns a symbolic ref type.
        using RetType = decltype(operator*() != *other);
        if (same_as(other)) return RetType(false);
        if (has_value() && other.has_value()) {
            return operator*() != *other;
        }
        // one of them is nullptr.
        return RetType(true);
    }
};
}// namespace ffi

// Expose to the tvm namespace
using ffi::Optional;
}// namespace litetvm

#endif//LITETVM_FFI_OPTIONAL_H
