//
// Created by 赵丹 on 25-3-4.
//

#ifndef STRUCTURAL_HASH_H
#define STRUCTURAL_HASH_H

#include "runtime/data_type.h"
#include "runtime/ndarray.h"

#include <cmath>
#include <functional>
#include <limits>
#include <string>

namespace litetvm {

using runtime::ObjectRef;

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
        }

        return Reinterpret<double, uint64_t>(key);
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
        return runtime::String::StableHashBytes(key.data(), key.length());
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
    LITETVM_API uint64_t operator()(const ObjectRef& key) const;
};

/*!
 * \brief A Reducer class to reduce the structural hash value.
 *
 *  The reducer will call the SEqualHash function of each objects recursively.
 *
 *  A SEqualHash function will make a sequence of calls to the reducer to
 *  indicate a sequence of child hash values that the reducer need to combine
 *  inorder to obtain the hash value of the hash value of the parent object.
 *
 *  Importantly, the reducer may not directly use recursive calls
 *  to compute the hash values of child objects directly.
 *
 *  Instead, it can store the necessary hash computing task into a stack
 *  and reduce the result later.
 */
class SHashReducer {
public:
    /*! \brief Internal handler that defines custom behaviors. */
    class Handler {
    public:
        /*!
     * \brief Append hashed_value to the current sequence of hashes.
     *
     * \param hashed_value The hashed value
     */
        virtual void SHashReduceHashedValue(uint64_t hashed_value) = 0;
        /*!
     * \brief Append hash value of key to the current sequence of hashes.
     *
     * \param key The object to compute hash from.
     * \param map_free_vars Whether to map free variables by their occurrence number.
     */
        virtual void SHashReduce(const ObjectRef& key, bool map_free_vars) = 0;
        /*!
     * \brief Append a hash value of free variable to the current sequence of hashes.
     *
     * \param var The var of interest.
     * \param map_free_vars Whether to map free variables by their occurrence number.
     *
     * \note If map_free_vars is set to be true,
     *       internally the handler can maintain a counter to encode free variables
     *       by their order of occurrence. This helps to resolve variable
     *       mapping of function parameters and let binding variables.
     *
     *       If map_free_vars is set to be false, the address of the variable will be used.
     */
        virtual void SHashReduceFreeVar(const runtime::Object* var, bool map_free_vars) = 0;
        /*!
     * \brief Lookup a hash value for key
     *
     * \param key The hash key.
     * \param hashed_value the result hash value
     *
     * \return Whether there is already a pre-computed hash value.
     */
        virtual bool LookupHashedValue(const ObjectRef& key, uint64_t* hashed_value) = 0;
        /*!
     * \brief Mark current comparison as graph node in hashing.
     *        Graph node hash will depends on the graph structure.
     */
        virtual void MarkGraphNode() = 0;
    };

    /*! \brief default constructor */
    SHashReducer() = default;
    /*!
   * \brief Constructor with a specific handler.
   * \param handler The equal handler for objects.
   * \param map_free_vars Whether to map free variables.
   */
    explicit SHashReducer(Handler* handler, bool map_free_vars)
        : handler_(handler), map_free_vars_(map_free_vars) {}
    /*!
   * \brief Push hash of key to the current sequence of hash values.
   * \param key The key to be hashed.
   */
    template<typename T,
             typename = std::enable_if_t<!std::is_base_of_v<ObjectRef, T>>>
    void operator()(const T& key) const {
        // handle normal values.
        handler_->SHashReduceHashedValue(BaseValueHash()(key));
    }
    /*!
   * \brief Push hash of key to the current sequence of hash values.
   * \param key The key to be hashed.
   */
    void operator()(const ObjectRef& key) const { return handler_->SHashReduce(key, map_free_vars_); }
    /*!
   * \brief Push hash of key to the current sequence of hash values.
   * \param key The key to be hashed.
   * \note This function indicate key could contain var defintions.
   */
    void DefHash(const ObjectRef& key) const { return handler_->SHashReduce(key, true); }
    /*!
   * \brief Implementation for hash for a free var.
   * \param var The variable.
   */
    void FreeVarHashImpl(const runtime::Object* var) const {
        handler_->SHashReduceFreeVar(var, map_free_vars_);
    }

    /*! \return Get the internal handler. */
    Handler* operator->() const { return handler_; }

private:
    /*! \brief Internal class pointer. */
    Handler* handler_;
    /*!
   * \brief Whether or not to map free variables by their occurrence
   *        If the flag is false, then free variables will be mapped
   *        by their in-memory address.
   */
    bool map_free_vars_;
};

/*! \brief The default handler for hash key computation
 *
 * Users can derive from this class and override the DispatchSHash method,
 * to customize hashing.
 */
class SHashHandlerDefault : public SHashReducer::Handler {
public:
    SHashHandlerDefault();
    virtual ~SHashHandlerDefault();

    void SHashReduceHashedValue(uint64_t hashed_value) override;
    void SHashReduce(const ObjectRef& key, bool map_free_vars) override;
    void SHashReduceFreeVar(const runtime::Object* var, bool map_free_vars) override;
    bool LookupHashedValue(const ObjectRef& key, uint64_t* hashed_value) override;
    void MarkGraphNode() override;

    /*!
     * \brief The entry point for hashing
     * \param object The object to be hashed.
     * \param map_free_vars Whether or not to remap variables if possible.
     * \return The hash result.
     */
    virtual uint64_t Hash(const ObjectRef& object, bool map_free_vars);

protected:
    /*!
     * \brief The dispatcher for hashing of intermediate objects
     * \param object An intermediate object to be hashed.
     * \param map_free_vars Whether or not to remap variables if possible.
     */
    virtual void DispatchSHash(const ObjectRef& object, bool map_free_vars);

private:
    class Impl;
    Impl* impl;
};

class SEqualReducer;
struct NDArrayContainerTrait {
    static constexpr std::nullptr_t VisitAttrs = nullptr;
    static void SHashReduce(const runtime::NDArray::Container* key, SHashReducer hash_reduce);
    static bool SEqualReduce(const runtime::NDArray::Container* lhs, const runtime::NDArray::Container* rhs, SEqualReducer equal);
};


}// namespace litetvm

#endif//STRUCTURAL_HASH_H
