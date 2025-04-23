//
// Created by 赵丹 on 25-2-28.
//

#ifndef LITETVM_NODE_STRUCTURAL_EQUAL_H
#define LITETVM_NODE_STRUCTURAL_EQUAL_H

#include "node/functor.h"
#include "node/object_path.h"
#include "runtime/array.h"
#include "runtime/data_type.h"

#include <cmath>
#include <limits>
// #include <numeric>
#include <string>

namespace litetvm {

using DataType = runtime::DataType;

/*!
 * \brief Equality definition of base value class.
 */
class BaseValueEqual {
public:
    bool operator()(const double& lhs, const double& rhs) const {
        if (std::isnan(lhs) && std::isnan(rhs)) {
            // IEEE floats do not compare as equivalent to each other.
            // However, for the purpose of comparing IR representation, two
            // NaN values are equivalent.
            return true;
        }

        if (std::isnan(lhs) || std::isnan(rhs)) {
            return false;
        }

        if (lhs == rhs) {
            return true;
        }

        // fuzzy float pt comparison
        return std::fabs(lhs - rhs) < std::numeric_limits<double>::epsilon();

        // constexpr double atol = 1e-9;
        // double diff = lhs - rhs;
        // return diff > -atol && diff < atol;
    }

    bool operator()(const int64_t& lhs, const int64_t& rhs) const { return lhs == rhs; }
    bool operator()(const uint64_t& lhs, const uint64_t& rhs) const { return lhs == rhs; }
    bool operator()(const int& lhs, const int& rhs) const { return lhs == rhs; }
    bool operator()(const bool& lhs, const bool& rhs) const { return lhs == rhs; }
    bool operator()(const std::string& lhs, const std::string& rhs) const { return lhs == rhs; }
    bool operator()(const DataType& lhs, const DataType& rhs) const { return lhs == rhs; }

    template<typename ENum, typename = std::enable_if_t<std::is_enum_v<ENum>>>
    bool operator()(const ENum& lhs, const ENum& rhs) const {
        return lhs == rhs;
    }
};


/*!
 * \brief Pair of `ObjectPath`s, one for each object being tested for structural equality.
 */
class ObjectPathPairNode : public Object {
public:
    ObjectPath lhs_path;
    ObjectPath rhs_path;

    ObjectPathPairNode(ObjectPath lhs_path, ObjectPath rhs_path)
        : lhs_path(std::move(lhs_path)), rhs_path(std::move(rhs_path)) {}

    static constexpr const char* _type_key = "ObjectPathPair";
    TVM_DECLARE_FINAL_OBJECT_INFO(ObjectPathPairNode, Object);
};

class ObjectPathPair : public ObjectRef {
public:
    ObjectPathPair(ObjectPath lhs_path, ObjectPath rhs_path) {
        data_ = runtime::make_object<ObjectPathPairNode>(std::move(lhs_path), std::move(rhs_path));
    }

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(ObjectPathPair, ObjectRef, ObjectPathPairNode);
};


/*!
 * \brief Content-aware structural equality comparator for objects.
 *
 *  The structural equality is recursively defined in the DAG of IR nodes via SEqual.
 *  There are two kinds of nodes:
 *
 *  - Graph node: a graph node in lhs can only be mapped as equal to
 *    one and only one graph node in rhs.
 *  - Normal node: equality is recursively defined without the restriction
 *    of graph nodes.
 *
 *  Vars(tir::Var, TypeVar) and non-constant relax expression nodes are graph nodes.
 *  For example, it means that `%1 = %x + %y; %1 + %1` is not structurally equal
 *  to `%1 = %x + %y; %2 = %x + %y; %1 + %2` in relax.
 *
 *  A var-type node(e.g. tir::Var, TypeVar) can be mapped as equal to another var
 *  with the same type if one of the following condition holds:
 *
 *  - They appear in the same definition point(e.g. function argument).
 *  - They point to the same VarNode via the same_as relation.
 *  - They appear in the same usage point, and map_free_vars is set to be True.
 */
class StructuralEqual : public BaseValueEqual {
public:
    // inheritate operator()
    using BaseValueEqual::operator();
    /*!
     * \brief Compare objects via strutural equal.
     * \param lhs The left operand.
     * \param rhs The right operand.
     * \param map_free_params Whether to map free variables.
     * \return The comparison result.
     */
    LITETVM_API bool operator()(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_params = false) const;
};

/*!
 * \brief A Reducer class to reduce the structural equality result of two objects.
 *
 * The reducer will call the SEqualReduce function of each object recursively.
 * Importantly, the reducer may not directly use recursive calls to resolve the
 * equality checking. Instead, it can store the necessary equality conditions
 * and check later via an internally managed stack.
 */
class SEqualReducer {
    struct PathTracingData;

public:
    /*! \brief Internal handler that defines custom behaviors. */
    class Handler {
    public:
        /*!
     * \brief Reduce condition to equality of lhs and rhs.
     *
     * \param lhs The left operand.
     * \param rhs The right operand.
     * \param map_free_vars Whether do we allow remap variables if possible.
     * \param current_paths Optional paths to `lhs` and `rhs` objects, for error traceability.
     *
     * \return false if there is an immediate failure, true otherwise.
     * \note This function may save the equality condition of (lhs == rhs) in an internal
     *       stack and try to resolve later.
     */
        virtual bool SEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars,
                                  const Optional<ObjectPathPair>& current_paths) = 0;

        /*!
     * \brief Mark the comparison as failed, but don't fail immediately.
     *
     * This is useful for producing better error messages when comparing containers.
     * For example, if two array sizes mismatch, it's better to mark the comparison as failed
     * but compare array elements anyway, so that we could find the true first mismatch.
     */
        virtual void DeferFail(const ObjectPathPair& mismatch_paths) = 0;

        /*!
     * \brief Check if fail defferal is enabled.
     *
     * \return false if the fail deferral is not enabled, true otherwise.
     */
        virtual bool IsFailDeferralEnabled() = 0;

        /*!
     * \brief Lookup the graph node equal map for vars that are already mapped.
     *
     *  This is an auxiliary method to check the Map<Var, Value> equality.
     * \param lhs an lhs value.
     *
     * \return The corresponding rhs value if any, nullptr if not available.
     */
        virtual ObjectRef MapLhsToRhs(const ObjectRef& lhs) = 0;

        /*!
         * \brief Mark current comparison as graph node equal comparison.
         */
        virtual void MarkGraphNode() = 0;

        virtual ~Handler() = default;

    protected:
        using PathTracingData = PathTracingData;
    };

    /*! \brief default constructor */
    SEqualReducer() = default;

    /*!
   * \brief Constructor with a specific handler.
   * \param handler The equal handler for objects.
   * \param tracing_data Optional pointer to the path tracing data.
   * \param map_free_vars Whether to map free variables.
   */
    explicit SEqualReducer(Handler* handler, const PathTracingData* tracing_data, bool map_free_vars)
        : handler_(handler), tracing_data_(tracing_data), map_free_vars_(map_free_vars) {}

    /*!
   * \brief Reduce condition to comparison of two attribute values.
   *
   * \param lhs The left operand.
   * \param rhs The right operand.
   * \param paths The paths to the LHS and RHS operands.
   * If unspecified, will attempt to identify the attribute's address
   * within the most recent ObjectRef.
   * In general, the paths only
   * require explicit handling for computed parameters
   * (e.g. `array.size()`)
   *
   * \return the immediate check result.
   */
    bool operator()(const double& lhs, const double& rhs, const Optional<ObjectPathPair>& paths = NullOpt) const;
    bool operator()(const int64_t& lhs, const int64_t& rhs, const Optional<ObjectPathPair>& paths = NullOpt) const;
    bool operator()(const uint64_t& lhs, const uint64_t& rhs, const Optional<ObjectPathPair>& paths = NullOpt) const;
    bool operator()(const int& lhs, const int& rhs, const Optional<ObjectPathPair>& paths = NullOpt) const;
    bool operator()(const bool& lhs, const bool& rhs, const Optional<ObjectPathPair>& paths = NullOpt) const;
    bool operator()(const std::string& lhs, const std::string& rhs,const Optional<ObjectPathPair>& paths = NullOpt) const;
    bool operator()(const DataType& lhs, const DataType& rhs,const Optional<ObjectPathPair>& paths = NullOpt) const;

    template<typename ENum, typename = std::enable_if_t<std::is_enum_v<ENum>>>
    bool operator()(const ENum& lhs, const ENum& rhs,Optional<ObjectPathPair> paths = NullOpt) const {
        using Underlying = std::underlying_type_t<ENum>;
        static_assert(std::is_same_v<Underlying, int>,"Enum must have `int` as the underlying type");
        return EnumAttrsEqual(static_cast<int>(lhs), static_cast<int>(rhs), &lhs, &rhs, paths);
    }

    template<typename T, typename Callable,
             typename = std::enable_if_t<
                     std::is_same_v<std::invoke_result_t<Callable, const ObjectPath&>, ObjectPath>>>
    bool operator()(const T& lhs, const T& rhs, const Callable& callable) {
        if (IsPathTracingEnabled()) {
            ObjectPathPair current_paths = GetCurrentObjectPaths();
            ObjectPathPair new_paths = {callable(current_paths->lhs_path),
                                        callable(current_paths->rhs_path)};
            return (*this)(lhs, rhs, new_paths);
        }
        return (*this)(lhs, rhs);
    }

    /*!
   * \brief Reduce condition to comparison of two objects.
   * \param lhs The left operand.
   * \param rhs The right operand.
   * \return the immediate check result.
   */
    bool operator()(const ObjectRef& lhs, const ObjectRef& rhs) const;

    /*!
   * \brief Reduce condition to comparison of two objects.
   *
   * Like `operator()`, but with an additional `paths` parameter that specifies explicit object
   * paths for `lhs` and `rhs`. This is useful for implementing SEqualReduce() methods for container
   * objects like Array and Map, or other custom objects that store nested objects that are not
   * simply attributes.
   *
   * Can only be called when `IsPathTracingEnabled()` is `true`.
   *
   * \param lhs The left operand.
   * \param rhs The right operand.
   * \param paths Object paths for `lhs` and `rhs`.
   * \return the immediate check result.
   */
    bool operator()(const ObjectRef& lhs, const ObjectRef& rhs, const ObjectPathPair& paths) const {
        CHECK(IsPathTracingEnabled()) << "Path tracing must be enabled when calling this function";
        return ObjectAttrsEqual(lhs, rhs, map_free_vars_, &paths);
    }

    /*!
   * \brief Reduce condition to comparison of two definitions,
   *        where free vars can be mapped.
   *
   *  Call this function to compare definition points such as function params
   *  and var in a let-binding.
   *
   * \param lhs The left operand.
   * \param rhs The right operand.
   * \return the immediate check result.
   */
    bool DefEqual(const ObjectRef& lhs, const ObjectRef& rhs) const;

    /*!
   * \brief Reduce condition to comparison of two arrays.
   * \param lhs The left operand.
   * \param rhs The right operand.
   * \return the immediate check result.
   */
    template<typename T>
    bool operator()(const Array<T>& lhs, const Array<T>& rhs) const {
        if (tracing_data_ == nullptr) {
            // quick specialization for Array to reduce amount of recursion
            // depth as array comparison is pretty common.
            if (lhs.size() != rhs.size()) return false;
            for (size_t i = 0; i < lhs.size(); ++i) {
                if (!(operator()(lhs[i], rhs[i]))) return false;
            }
            return true;
        }

        // If tracing is enabled, fall back to the regular path
        const ObjectRef& lhs_obj = lhs;
        const ObjectRef& rhs_obj = rhs;
        return (*this)(lhs_obj, rhs_obj);
    }

    /*!
   * \brief Implementation for equality rule of var type objects(e.g. TypeVar, tir::Var).
   * \param lhs The left operand.
   * \param rhs The right operand.
   * \return the result.
   */
    bool FreeVarEqualImpl(const runtime::Object* lhs, const runtime::Object* rhs) const {
        // var need to be remapped, so it belongs to graph node.
        handler_->MarkGraphNode();
        // We only map free vars if they correspond to the same address
        // or map free_var option is set to be true.
        return lhs == rhs || map_free_vars_;
    }

    /*! \return Get the internal handler. */
    Handler* operator->() const { return handler_; }

    /*! \brief Check if this reducer is tracing paths to the first mismatch. */
    NODISCARD bool IsPathTracingEnabled() const { return tracing_data_ != nullptr; }

    /*!
   * \brief Get the paths of the currently compared objects.
   *
   * Can only be called when `IsPathTracingEnabled()` is true.
   */
    NODISCARD const ObjectPathPair& GetCurrentObjectPaths() const;

    /*!
   * \brief Specify the object paths of a detected mismatch.
   *
   * Can only be called when `IsPathTracingEnabled()` is true.
   */
    void RecordMismatchPaths(const ObjectPathPair& paths) const;


private:
    bool EnumAttrsEqual(int lhs, int rhs, const void* lhs_address, const void* rhs_address,
                        const Optional<ObjectPathPair>& paths = NullOpt) const;

    bool ObjectAttrsEqual(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars,
                          const ObjectPathPair* paths) const;

    static void GetPathsFromAttrAddressesAndStoreMismatch(const void* lhs_address,
                                                          const void* rhs_address,
                                                          const PathTracingData* tracing_data);
    template<typename T>
    static bool CompareAttributeValues(const T& lhs, const T& rhs,
                                       const PathTracingData* tracing_data,
                                       const Optional<ObjectPathPair>& paths = NullOpt);

    /*! \brief Internal class pointer. */
    Handler* handler_ = nullptr;

    /*! \brief Pointer to the current path tracing context, or nullptr if path tracing is disabled. */
    const PathTracingData* tracing_data_ = nullptr;

    /*! \brief Whether to map free vars. */
    bool map_free_vars_ = false;
};

/*! \brief The default handler for equality testing.
 *
 * Users can derive from this class and override the DispatchSEqualReduce method,
 * to customize equality testing.
 */
class SEqualHandlerDefault : public SEqualReducer::Handler {
public:
    SEqualHandlerDefault(bool assert_mode, Optional<ObjectPathPair>* first_mismatch,bool defer_fails);
    ~SEqualHandlerDefault() override;

    bool SEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars,
                      const Optional<ObjectPathPair>& current_paths) override;
    void DeferFail(const ObjectPathPair& mismatch_paths) override;
    bool IsFailDeferralEnabled() override;
    ObjectRef MapLhsToRhs(const ObjectRef& lhs) override;
    void MarkGraphNode() override;

    /*!
     * \brief The entry point for equality testing
     * \param lhs The left operand.
     * \param rhs The right operand.
     * \param map_free_vars Whether to remap variables if possible.
     * \return The equality result.
     */
    virtual bool Equal(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars);

protected:
    /*!
     * \brief The dispatcher for equality testing of intermediate objects
     * \param lhs The left operand.
     * \param rhs The right operand.
     * \param map_free_vars Whether to remap variables if possible.
     * \param current_paths Optional paths to `lhs` and `rhs` objects, for error traceability.
     * \return The equality result.
     */
    virtual bool DispatchSEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars,
                                      const Optional<ObjectPathPair>& current_paths);

private:
    class Impl;
    Impl* impl;
};

}// namespace litetvm


#endif//LITETVM_NODE_STRUCTURAL_EQUAL_H
