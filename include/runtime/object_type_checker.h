//
// Created by richard on 2/6/25.
//

#ifndef OBJECT_TYPE_CHECKER_H
#define OBJECT_TYPE_CHECKER_H

#include "runtime/array.h"
#include "runtime/map.h"
#include "runtime/object.h"
#include "runtime/string.h"
#include "runtime/variant.h"

namespace litetvm::runtime {

/*!
 * \brief Type traits for runtime type check during FFI conversion.
 * \tparam T the type to be checked.
 */
template<typename T>
struct ObjectTypeChecker {
    using ContainerType = typename T::ContainerType;
    /*!
     * \brief Check if an object matches the template type and return the
     *        mismatched type if it exists.
     * \param ptr The object to check the type of.
     * \return An Optional containing the actual type of the pointer if it does not match the
     *         template type. If the Optional does not contain a value, then the types match.
     */
    static Optional<String> CheckAndGetMismatch(const Object* ptr) {
        // using ContainerType = typename T::ContainerType;
        if (ptr == nullptr) {
            if (T::_type_is_nullable) {
                return NullOpt;
            }
            return String("nullptr");
        }

        if (ptr->IsInstance<ContainerType>()) {
            return NullOpt;
        }

        return String(ptr->GetTypeKey());
    }
    /*!
     * \brief Check if an object matches the template type.
     * \param ptr The object to check the type of.
     * \return Whether the template type matches the objects type.
     */
    static bool Check(const Object* ptr) {
        // using ContainerType = typename T::ContainerType;
        if (ptr == nullptr)
            return T::_type_is_nullable;
        return ptr->IsInstance<ContainerType>();
    }

    static std::string TypeName() {
        // using ContainerType = typename T::ContainerType;
        return ContainerType::_type_key;
    }
};

// Additional overloads for PackedFunc checking.
template<typename T>
struct ObjectTypeChecker<Array<T>> {
    static Optional<String> CheckAndGetMismatch(const Object* ptr) {
        if (ptr == nullptr) {
            return NullOpt;
        }

        if (!ptr->IsInstance<ArrayNode>()) {
            return String(ptr->GetTypeKey());
        }

        if constexpr (std::is_same_v<T, ObjectRef>) {
            return NullOpt;
        }

        const auto* n = static_cast<const ArrayNode*>(ptr);
        for (size_t i = 0; i < n->size(); i++) {
            const ObjectRef& p = (*n)[i];
            Optional<String> check_subtype = ObjectTypeChecker<T>::CheckAndGetMismatch(p.get());
            if (check_subtype.defined()) {
                return String("Array[index " + std::to_string(i) + ": " + check_subtype.value() + "]");
            }
        }
        return NullOpt;
    }

    static bool Check(const Object* ptr) {
        if (ptr == nullptr) {
            return true;
        }

        if (!ptr->IsInstance<ArrayNode>()) {
            return false;
        }

        if constexpr (std::is_same_v<T, ObjectRef>) {
            return true;
        }

        const auto* n = static_cast<const ArrayNode*>(ptr);
        for (const ObjectRef& p: *n) {
            if (!ObjectTypeChecker<T>::Check(p.get())) {
                return false;
            }
        }
        return true;
    }

    static std::string TypeName() {
        return "Array[" + ObjectTypeChecker<T>::TypeName() + "]";
    }
};

template<typename K, typename V>
struct ObjectTypeChecker<Map<K, V>> {
    static Optional<String> CheckAndGetMismatch(const Object* ptr) {
        if (ptr == nullptr) {
            return NullOpt;
        }

        if (!ptr->IsInstance<MapNode>()) {
            return String(ptr->GetTypeKey());
        }

        if constexpr (std::is_same_v<K, ObjectRef> && std::is_same_v<V, ObjectRef>) {
            return NullOpt;
        }

        const auto* n = static_cast<const MapNode*>(ptr);
        for (const auto& kv: *n) {
            Optional<String> key_type = NullOpt;
            if constexpr (!std::is_same_v<K, ObjectRef>) {
                key_type = ObjectTypeChecker<K>::CheckAndGetMismatch(kv.first.get());
            }

            Optional<String> value_type = NullOpt;
            if constexpr (!std::is_same_v<V, ObjectRef>) {
                value_type = ObjectTypeChecker<K>::CheckAndGetMismatch(kv.first.get());
            }

            if (key_type.defined() || value_type.defined()) {
                std::string key_name =
                        key_type.defined() ? std::string(key_type.value()) : ObjectTypeChecker<K>::TypeName();
                std::string value_name = value_type.defined() ? std::string(value_type.value())
                                                              : ObjectTypeChecker<V>::TypeName();
                return String("Map[" + key_name + ", " + value_name + "]");
            }
        }
        return NullOpt;
    }

    static bool Check(const Object* ptr) {
        if (ptr == nullptr) {
            return true;
        }

        if (!ptr->IsInstance<MapNode>()) {
            return false;
        }

        if constexpr (std::is_same_v<K, ObjectRef> && std::is_same_v<V, ObjectRef>) {
            return true;
        }

        const auto* n = static_cast<const MapNode*>(ptr);
        for (const auto& kv: *n) {
            if constexpr (!std::is_same_v<K, ObjectRef>) {
                if (!ObjectTypeChecker<K>::Check(kv.first.get())) return false;
            }

            if constexpr (!std::is_same_v<V, ObjectRef>) {
                if (!ObjectTypeChecker<V>::Check(kv.second.get())) return false;
            }
        }
        return true;
    }

    static std::string TypeName() {
        return "Map[" + ObjectTypeChecker<K>::TypeName() + ", " + ObjectTypeChecker<V>::TypeName() + ']';
    }
};

template<typename OnlyVariant>
struct ObjectTypeChecker<Variant<OnlyVariant>> {
    static Optional<String> CheckAndGetMismatch(const Object* ptr) {
        return ObjectTypeChecker<OnlyVariant>::CheckAndGetMismatch(ptr);
    }

    static bool Check(const Object* ptr) {
        return ObjectTypeChecker<OnlyVariant>::Check(ptr);
    }

    static std::string TypeName() {
        return "Variant[" + VariantNames() + "]";
    }

    static std::string VariantNames() {
        return ObjectTypeChecker<OnlyVariant>::TypeName();
    }
};

template<typename FirstVariant, typename... RemainingVariants>
struct ObjectTypeChecker<Variant<FirstVariant, RemainingVariants...>> {
    static Optional<String> CheckAndGetMismatch(const Object* ptr) {
        auto try_first = ObjectTypeChecker<FirstVariant>::CheckAndGetMismatch(ptr);
        if (!try_first.defined()) {
            return try_first;
        }

        return ObjectTypeChecker<Variant<RemainingVariants...>>::CheckAndGetMismatch(ptr);
    }

    static bool Check(const Object* ptr) {
        return ObjectTypeChecker<FirstVariant>::Check(ptr) ||
               ObjectTypeChecker<Variant<RemainingVariants...>>::Check(ptr);
    }

    static std::string TypeName() {
        return "Variant[" + VariantNames() + "]";
    }

    static std::string VariantNames() {
        return ObjectTypeChecker<FirstVariant>::TypeName() + ", " +
               ObjectTypeChecker<Variant<RemainingVariants...>>::VariantNames();
    }
};

}// namespace litetvm::runtime

#endif//OBJECT_TYPE_CHECKER_H
