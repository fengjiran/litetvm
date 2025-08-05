//
// Created by richard on 5/15/25.
//

#ifndef LITETVM_FFI_CAST_H
#define LITETVM_FFI_CAST_H

#include "ffi/any.h"
#include "ffi/dtype.h"
#include "ffi/error.h"
#include "ffi/object.h"
#include "ffi/optional.h"

#include <utility>

namespace litetvm {
namespace ffi {
/*!
 * \brief Get a reference type from a raw object ptr type
 *
 *  It is always important to get a reference type
 *  if we want to return a value as reference or keep
 *  the object alive beyond the scope of the function.
 *
 * \param ptr The object pointer
 * \tparam RefType The reference type
 * \tparam ObjectType The object type
 * \return The corresponding RefType
 */
template<typename RefType, typename ObjectType>
TVM_FFI_INLINE RefType GetRef(const ObjectType* ptr) {
    static_assert(std::is_base_of_v<typename RefType::ContainerType, ObjectType>, "Can only cast to the ref of same container type");

    if constexpr (is_optional_type_v<RefType> || RefType::_type_is_nullable) {
        if (ptr == nullptr) {
            return RefType(ObjectPtr<Object>(nullptr));
        }
    } else {
        TVM_FFI_ICHECK_NOTNULL(ptr);
    }
    return RefType(details::ObjectUnsafe::ObjectPtrFromUnowned<Object>(const_cast<Object*>(static_cast<const Object*>(ptr))));
}

/*!
 * \brief Get an object ptr type from a raw object ptr.
 *
 * \param ptr The object pointer
 * \tparam BaseType The reference type
 * \tparam ObjectType The object type
 * \return The corresponding RefType
 */
template<typename BaseType, typename ObjectType>
ObjectPtr<BaseType> GetObjectPtr(ObjectType* ptr) {
    static_assert(std::is_base_of_v<BaseType, ObjectType>, "Can only cast to the ref of same container type");
    return details::ObjectUnsafe::ObjectPtrFromUnowned<BaseType>(ptr);
}

/*!
 * \brief Downcast a base reference type to a more specific type.
 *
 * \param ref The input reference
 * \return The corresponding SubRef.
 * \tparam SubRef The target specific reference type.
 * \tparam BaseRef the current reference type.
 */
template<typename SubRef, typename BaseRef,
         typename = std::enable_if_t<std::is_base_of_v<ObjectRef, BaseRef>>>
SubRef Downcast(BaseRef ref) {
    if (ref.defined()) {
        if (!ref->template IsInstance<typename SubRef::ContainerType>()) {
            TVM_FFI_THROW(TypeError) << "Downcast from " << ref->GetTypeKey() << " to " << SubRef::ContainerType::_type_key << " failed.";
        }
        return SubRef(details::ObjectUnsafe::ObjectPtrFromObjectRef<Object>(std::move(ref)));
    }

    if constexpr (is_optional_type_v<SubRef> || SubRef::_type_is_nullable) {
        return SubRef(ObjectPtr<Object>(nullptr));
    }

    TVM_FFI_THROW(TypeError) << "Downcast from undefined(nullptr) to `" << SubRef::ContainerType::_type_key << "` is not allowed. Use Downcast<Optional<T>> instead.";
    TVM_FFI_UNREACHABLE();
}

/*!
 * \brief Downcast any to a specific type
 *
 * \param ref The input reference
 * \return The corresponding SubRef.
 * \tparam T The target specific reference type.
 */
template<typename T>
T Downcast(const Any& ref) {
    if constexpr (std::is_same_v<T, Any>) {
        return ref;
    } else {
        return ref.cast<T>();
    }
}

/*!
 * \brief Downcast any to a specific type
 *
 * \param ref The input reference
 * \return The corresponding SubRef.
 * \tparam T The target specific reference type.
 */
template<typename T>
T Downcast(Any&& ref) {
    if constexpr (std::is_same_v<T, Any>) {
        return std::move(ref);
    } else {
        return std::move(ref).cast<T>();
    }
}

/*!
 * \brief Downcast std::optional<Any> to std::optional<T>
 *
 * \param ref The input reference
 * \return The corresponding SubRef.
 * \tparam OptionalType The target optional type
 */
template<typename OptionalType,
         typename = std::enable_if_t<is_optional_type_v<OptionalType>>>
OptionalType Downcast(const std::optional<Any>& ref) {
    if (ref.has_value()) {
        if constexpr (std::is_same_v<OptionalType, Any>) {
            return *ref;
        } else {
            return (*ref).cast<OptionalType>();
        }
    }
    return OptionalType(std::nullopt);
}

}// namespace ffi

// Expose to the tvm namespace
// Rationale: convinience and no ambiguity
using ffi::Downcast;
using ffi::GetRef;
}// namespace litetvm

#endif//LITETVM_FFI_CAST_H
