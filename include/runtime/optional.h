//
// Created by richard on 2/5/25.
//

#ifndef OPTIONAL_H
#define OPTIONAL_H

#include "runtime/base.h"

namespace litetvm {

namespace runtime {
/*! \brief Helper to represent nullptr for optional. */
struct NullOptType {};

/*!
 * \brief Optional container that to represent to a Nullable variant of T.
 * \tparam T The original ObjectRef.
 *
 * \code
 *
 *  Optional<String> opt0 = nullptr;
 *  Optional<String> opt1 = String("xyz");
 *  ICHECK(opt0 == nullptr);
 *  ICHECK(opt1 == "xyz");
 *
 * \endcode
 */
template<typename T>
class Optional : public ObjectRef {
public:
    using ContainerType = typename T::ContainerType;
    static_assert(std::is_base_of_v<ObjectRef, T>, "Optional is only defined for ObjectRef.");

    // default constructors.
    Optional() = default;
    Optional(const Optional&) = default;
    Optional(Optional&&) = default;
    Optional& operator=(const Optional&) = default;
    Optional& operator=(Optional&&) = default;

    /*!
   * \brief Construct from an ObjectPtr
   *        whose type already matches the ContainerType.
   * \param ptr
   */
    explicit Optional(const ObjectPtr<Object>& ptr) : ObjectRef(ptr) {}

    /*! \brief Nullopt handling */
    Optional(NullOptType) {}// NOLINT(*)

    // nullptr handling.
    // disallow implicit conversion as 0 can be implicitly converted to nullptr_t
    explicit Optional(std::nullptr_t) {}

    Optional& operator=(std::nullptr_t) {
        data_ = nullptr;
        return *this;
    }

    // normal value handling.
    Optional(T other)// NOLINT(*)
        : ObjectRef(std::move(other)) {}

    Optional& operator=(T other) {
        ObjectRef::operator=(std::move(other));
        return *this;
    }
    // delete the int constructor
    // since Optional<Integer>(0) is ambiguious
    // 0 can be implicitly casted to nullptr_t
    explicit Optional(int val) = delete;
    Optional& operator=(int val) = delete;

    /*!
   * \return A not-null container value in the optional.
   * \note This function performs not-null checking.
   */
    T value() const {
        CHECK(data_ != nullptr);
        return T(data_);
    }

    /*!
   * \return The internal object pointer with container type of T.
   * \note This function do not perform not-null checking.
   */
    const ContainerType* get() const {
        return static_cast<ContainerType*>(data_.get());
    }

    /*!
     * \return The contained value if the Optional is not null
     *         otherwise return the default_value.
     */
    T value_or(T default_value) const {
        return data_ != nullptr ? T(data_) : default_value;
    }

    /*! \return Whether the container is not nullptr.*/
    explicit operator bool() const {
        return *this != nullptr;
    }

    // operator overloadings
    bool operator==(std::nullptr_t) const {
        return data_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const {
        return data_ != nullptr;
    }

    auto operator==(const Optional& other) const {
        // support case where sub-class returns a symbolic ref type.
        using RetType = decltype(value() == other.value());
        if (same_as(other))
            return RetType(true);

        if (*this != nullptr && other != nullptr) {
            return value() == other.value();
        }

        // one of them is nullptr.
        return RetType(false);
    }

    auto operator!=(const Optional& other) const {
        // support case where sub-class returns a symbolic ref type.
        using RetType = decltype(value() != other.value());
        if (same_as(other))
            return RetType(false);

        if (*this != nullptr && other != nullptr) {
            return value() != other.value();
        }

        // one of them is nullptr.
        return RetType(true);
    }

    auto operator==(const T& other) const {
        using RetType = decltype(value() == other);
        if (same_as(other))
            return RetType(true);

        if (*this != nullptr)
            return value() == other;

        return RetType(false);
    }

    auto operator!=(const T& other) const {
        return !(*this == other);
    }

    template<typename U>
    auto operator==(const U& other) const {
        using RetType = decltype(value() == other);
        if (*this == nullptr)
            return RetType(false);

        return value() == other;
    }

    template<typename U>
    auto operator!=(const U& other) const {
        using RetType = decltype(value() != other);
        if (*this == nullptr)
            return RetType(true);

        return value() != other;
    }
    static constexpr bool _type_is_nullable = true;
};
}// namespace runtime

// expose the functions to the root namespace.
using runtime::Optional;
constexpr runtime::NullOptType NullOpt{};

}// namespace litetvm

#endif//OPTIONAL_H
