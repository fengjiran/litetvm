//
// Created by richard on 8/2/25.
//

#ifndef LITETVM_NODE_STRUCTURAL_HASH_H
#define LITETVM_NODE_STRUCTURAL_HASH_H

#include "runtime/data_type.h"
#include "runtime/ndarray.h"

#include <cmath>
#include <functional>
#include <limits>
#include <string>

namespace litetvm {

/*!
 * \brief Hash definition of base value classes.
 */
class BaseValueHash {
protected:
    template<typename T, typename U>
    uint64_t Reinterpret(T value) const {
        union Union {
            T a;
            U b;
        } u;
        static_assert(sizeof(Union) == sizeof(T), "sizeof(Union) != sizeof(T)");
        static_assert(sizeof(Union) == sizeof(U), "sizeof(Union) != sizeof(U)");
        u.b = 0;
        u.a = value;
        return u.b;
    }

public:
    uint64_t operator()(const float& key) const { return Reinterpret<float, uint32_t>(key); }
    uint64_t operator()(const double& key) const {
        if (std::isnan(key)) {
            // The IEEE format defines more than one bit-pattern that
            // represents NaN.  For the purpose of comparing IR
            // representations, all NaN values are considered equivalent.
            return Reinterpret<double, uint64_t>(std::numeric_limits<double>::quiet_NaN());
        } else {
            return Reinterpret<double, uint64_t>(key);
        }
    }
    uint64_t operator()(const int64_t& key) const { return Reinterpret<int64_t, uint64_t>(key); }
    uint64_t operator()(const uint64_t& key) const { return key; }
    uint64_t operator()(const int& key) const { return Reinterpret<int, uint32_t>(key); }
    uint64_t operator()(const bool& key) const { return key; }
    uint64_t operator()(const runtime::DataType& key) const {
        return Reinterpret<DLDataType, uint32_t>(key);
    }
    template<typename ENum, typename = typename std::enable_if<std::is_enum<ENum>::value>::type>
    uint64_t operator()(const ENum& key) const {
        return Reinterpret<int64_t, uint64_t>(static_cast<int64_t>(key));
    }
    uint64_t operator()(const std::string& key) const {
        return litetvm::ffi::details::StableHashBytes(key.data(), key.length());
    }
    uint64_t operator()(const Optional<int64_t>& key) const {
        if (key.has_value()) {
            return Reinterpret<int64_t, uint64_t>(*key);
        } else {
            return 0;
        }
    }
    uint64_t operator()(const Optional<double>& key) const {
        if (key.has_value()) {
            return Reinterpret<double, uint64_t>(*key);
        } else {
            return 0;
        }
    }
    /*!
   * \brief Compute structural hash value for a POD value in Any.
   * \param key The Any object.
   * \return The hash value.
   */
    TVM_FFI_INLINE uint64_t HashPODValueInAny(const ffi::Any& key) const {
        return ffi::details::AnyUnsafe::TVMFFIAnyPtrFromAny(key)->v_uint64;
    }
};

/*!
 * \brief Content-aware structural hashing.
 *
 *  The structural hash value is recursively defined in the DAG of IRNodes.
 *  There are two kinds of nodes:
 *
 *  - Normal node: the hash value is defined by its content and type only.
 *  - Graph node: each graph node will be assigned a unique index ordered by the
 *    first occurrence during the visit. The hash value of a graph node is
 *    combined from the hash values of its contents and the index.
 */
class StructuralHash : public BaseValueHash {
public:
    // inherit operator()
    using BaseValueHash::operator();
    /*!
   * \brief Compute structural hashing value for an object.
   * \param key The left operand.
   * \return The hash value.
   */
    TVM_DLL uint64_t operator()(const ffi::Any& key) const;
};

}// namespace litetvm

#endif//LITETVM_NODE_STRUCTURAL_HASH_H
