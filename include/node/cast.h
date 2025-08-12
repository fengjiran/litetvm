//
// Created by 赵丹 on 2025/8/12.
//

#ifndef LITETVM_NODE_CAST_H
#define LITETVM_NODE_CAST_H

#include "ffi/any.h"
#include "ffi/cast.h"
#include "ffi/dtype.h"
#include "ffi/error.h"
#include "ffi/object.h"
#include "ffi/optional.h"

namespace litetvm {

/*!
 * \brief Downcast a base reference type to a more specific type.
 *
 * \param ref The input reference
 * \return The corresponding SubRef.
 * \tparam SubRef The target specific reference type.
 * \tparam BaseRef the current reference type.
 */
template<typename SubRef, typename BaseRef,
         typename = std::enable_if_t<std::is_base_of_v<ffi::ObjectRef, BaseRef>>>
SubRef Downcast(BaseRef ref) {
    if (ref.defined()) {
        if (!ref->template IsInstance<typename SubRef::ContainerType>()) {
            TVM_FFI_THROW(TypeError) << "Downcast from " << ref->GetTypeKey() << " to "
                                     << SubRef::ContainerType::_type_key << " failed.";
        }
        return SubRef(ffi::details::ObjectUnsafe::ObjectPtrFromObjectRef<ffi::Object>(std::move(ref)));
    }
    if constexpr (ffi::is_optional_type_v<SubRef> || SubRef::_type_is_nullable) {
        return SubRef(ffi::ObjectPtr<ffi::Object>(nullptr));
    }
    TVM_FFI_THROW(TypeError) << "Downcast from undefined(nullptr) to `"
                             << SubRef::ContainerType::_type_key
                             << "` is not allowed. Use Downcast<Optional<T>> instead.";
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
template<typename OptionalType, typename = std::enable_if_t<ffi::is_optional_type_v<OptionalType>>>
OptionalType Downcast(const std::optional<Any>& ref) {
    if (ref.has_value()) {
        if constexpr (std::is_same_v<OptionalType, ffi::Any>) {
            return *ref;
        } else {
            return (*ref).cast<OptionalType>();
        }
    }
    return OptionalType(std::nullopt);
}
}// namespace litetvm

#endif//LITETVM_NODE_CAST_H
