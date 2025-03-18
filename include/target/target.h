//
// Created by 赵丹 on 25-3-18.
//

#ifndef LITETVM_TARGET_TARGET_H
#define LITETVM_TARGET_TARGET_H

#include "target/target_kind.h"
#include "support/with.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace litetvm {

class TargetInternal;
class Target;

/*!
 * \brief Compilation target.
 * \sa Target
 */
class TargetNode : public Object {
public:
    /*! \brief The kind of the target device */
    TargetKind kind;
    /*! \brief Target host information, must be Target type */
    Optional<ObjectRef> host;
    /*! \brief Tag of the target, can be empty */
    String tag;
    /*! \brief Keys for this target */
    Array<String> keys;
    /*! \brief Collection of attributes */
    Map<String, ObjectRef> attrs;
    /*! \brief Target features */
    Map<String, ObjectRef> features;

    /*!
   * \brief The raw string representation of the target
   * \return the full device string to pass to codegen::Build
   * \note It will be deprecated after the Target RFC is fully landed.
   */
    LITETVM_API const std::string& str() const;
    /*! \return Export target to JSON-like configuration */
    LITETVM_API Map<String, ObjectRef> Export() const;
    /*! \return The Optional<Target> typed target host of the TargetNode */
    LITETVM_API Optional<Target> GetHost() const;
    /*! \return The device type for this target */
    LITETVM_API int GetTargetDeviceType() const;

    /*!
   * \brief Check if the target contains a key
   *
   * \param query_key The string name of the key to be checked
   *
   * \return True if the target's `TargetNode::keys` contains the
   * specified key, False otherwise.
   */
    LITETVM_API bool HasKey(const std::string& query_key) const;

    /*!
   * \brief Returns a human readable representation of \p Target which includes all fields,
   * especially the host. Useful for diagnostic messages and debugging.
   *
   * TODO(mbs): The ReprPrinter version should perhaps switch to this form, however currently
   * code depends on str() and << being the same.
   */
    String ToDebugString() const;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("kind", &kind);
        v->Visit("tag", &tag);
        v->Visit("keys", &keys);
        v->Visit("attrs", &attrs);
        v->Visit("features", &features);
        v->Visit("host", &host);
    }

    /*!
   * \brief Get an entry from attrs of the target
   * \tparam TObjectRef Type of the attribute
   * \param attr_key The name of the attribute key
   * \param default_value The value returned if the key is not present
   * \return An optional, NullOpt if not found, otherwise the value found
   */
    template<typename TObjectRef>
    Optional<TObjectRef> GetAttr(
            const std::string& attr_key,
            Optional<TObjectRef> default_value = Optional<TObjectRef>(nullptr)) const {
        static_assert(std::is_base_of<ObjectRef, TObjectRef>::value,
                      "Can only call GetAttr with ObjectRef types.");
        auto it = attrs.find(attr_key);
        if (it != attrs.end()) {
            // For backwards compatibility, return through TVMRetValue.
            // This triggers any automatic conversions registered with
            // PackedFuncValueConverter.  Importantly, this allows use of
            // `GetAttr<Integer>` and `GetAttr<Bool>` for properties that
            // are stored internally as `runtime::Box<int64_t>` and
            // `runtime::Box<bool>`.
            TVMRetValue ret;
            ret = (*it).second;
            return ret;
        } else {
            return default_value;
        }
    }
    /*!
   * \brief Get an entry from attrs of the target
   * \tparam TObjectRef Type of the attribute
   * \param attr_key The name of the attribute key
   * \param default_value The value returned if the key is not present
   * \return An optional, NullOpt if not found, otherwise the value found
   */
    template<typename TObjectRef>
    Optional<TObjectRef> GetAttr(const std::string& attr_key, TObjectRef default_value) const {
        return GetAttr<TObjectRef>(attr_key, Optional<TObjectRef>(default_value));
    }

    /*!
   * \brief Get a Target feature
   *
   * \param feature_key The feature key.
   * \param default_value The default value if the key does not exist, defaults to nullptr.
   *
   * \return The result
   *
   * \tparam TOBjectRef the expected object type.
   * \throw Error if the key exists but the value does not match TObjectRef
   *
   * \code
   *
   *  void GetTargetFeature(const Target& target) {
   *    Bool has_feature = target->GetFeature<Bool>("has_feature", false).value();
   *  }
   *
   * \endcode
   */
    template<typename TObjectRef>
    Optional<TObjectRef> GetFeature(
            const std::string& feature_key,
            Optional<TObjectRef> default_value = Optional<TObjectRef>(nullptr)) const {
        Optional<TObjectRef> feature = Downcast<Optional<TObjectRef>>(features.Get(feature_key));
        if (!feature) {
            return default_value;
        }
        return feature;
    }
    // variant that uses TObjectRef to enable implicit conversion to default value.
    template<typename TObjectRef>
    Optional<TObjectRef> GetFeature(const std::string& attr_key, TObjectRef default_value) const {
        return GetFeature<TObjectRef>(attr_key, Optional<TObjectRef>(default_value));
    }

    /*! \brief Get the keys for this target as a vector of string */
    LITETVM_API std::vector<std::string> GetKeys() const;
    /*! \brief Get the keys for this target as an unordered_set of string */
    LITETVM_API std::unordered_set<std::string> GetLibs() const;

    bool SEqualReduce(const TargetNode* other, SEqualReducer equal) const;
    void SHashReduce(SHashReducer hash_reduce) const;

    static constexpr const char* _type_key = "Target";
    static constexpr const bool _type_has_method_sequal_reduce = true;
    static constexpr const bool _type_has_method_shash_reduce = true;
    TVM_DECLARE_FINAL_OBJECT_INFO(TargetNode, Object);

private:
    /*! \brief Internal string repr. */
    mutable std::string str_repr_;

    friend class TargetInternal;
};

/*!
 * \brief Managed reference class to TargetNode.
 * \sa TargetNode
 */
class Target : public ObjectRef {
public:
    /*! \brief Construct a null Target */
    LITETVM_API explicit Target(std::nullptr_t) { data_ = nullptr; }
    /*!
   * \brief Construct a Target given a string
   * \param tag_or_config_or_target_str the string to parse for target
   */
    LITETVM_API explicit Target(const String& tag_or_config_or_target_str);
    /*!
   * \brief Construct a Target using a JSON-like configuration
   * \param config The JSON-like configuration for target
   */
    LITETVM_API explicit Target(const Map<String, ObjectRef>& config);
    /*!
   * \brief Get the current target context from thread local storage.
   * \param allow_not_defined If the context stack is empty and this is set to true, an
   *   undefined Target will be returned. Otherwise, an empty context stack will cause a
   *   runtime error.
   * \return The target that is the current context. The target may not be defined if
   * allow_not_defined is true.
   */
    LITETVM_API static Target Current(bool allow_not_defined = true);
    /*!
   * \brief Construct a Target given target and host
   * \param target The Target typed object with host field undefined for target
   * \param host The Target typed object for target host
   */
    LITETVM_API explicit Target(Target target, Target host);
    TVM_DEFINE_OBJECT_REF_METHODS(Target, ObjectRef, TargetNode);
    /*!
   * \brief Create a new Target object with given target (w.o host) and target host.
   * \param target The current Target typed object target, with or without host field.
   * \param host The given Target typed object target host
   * \return The new Target object with the given target and host field of given host.
   */
    static Target WithHost(const Target& target, const Target& host);

    /*! \return The target with the host stripped out */
    Target WithoutHost() const;

    /*!
   * \brief Returns true if \p this target represents an external codegen. If so,
   * \p this->kind->name can be used as the "Compiler" attribute on partitioned functions,
   * and can be used to retrieve a partitioning pattern table using
   * \p get_pattern_table.
   */
    bool IsExternalCodegen() const;

    /*!
   * \brief Returns true if \p this target represents an external codegen which is compatible
   * with \p that target. In particular:
   *  - \p this has a true ::tvm::attr::kIsExternalCodegen attribute
   *  - \p that does not have a true ::tvm::attr::kIsExternalCodegen attribute
   *  - \p this and \p that have the same GetTargetDeviceType()
   *
   * After partitioning, the external codegen compilation path may use \p that to guide it's
   * compilation to a \p runtime::Module. Given \p this, an appropriate \p that can be
   * found using \p CompilationConfig::FindPrimitiveTargetOrFail(this->GetTargetDeviceType()).
   *
   * The \p CollagePartition pass uses this method to guide it's search over candidate partitions
   * using external codegen.
   */
    bool IsExternalCodegenFor(const Target& that) const;

private:
    Target(TargetKind kind, Optional<ObjectRef> host, String tag, Array<String> keys,
           Map<String, ObjectRef> attrs);

    // enable with syntax.
    friend class TargetInternal;
    friend class With<Target>;
    /*!
   * \brief Push a new target context onto the thread local stack.
   *  The Target on top of the stack is used to determine which
   *  specialization to use when invoking a GenericFunc.
   */
    LITETVM_API void EnterWithScope();
    /*!
   * \brief Pop a target off the thread local context stack,
   *  restoring the previous target as the current context.
   */
    LITETVM_API void ExitWithScope() const;
};


/*!
 * \brief Check and update host field of the given legacy target and target host pair.
 *  Note that this function is for legacy target api compatibility issue only, not
 *  recommended for other use.
 * \param target The pointer to a Target typed object with host field to be updated
 * \param host The pointer to a Target typed object for target host to be updated
 */
void CheckAndUpdateHostConsistency(Target* target, Target* host);

}// namespace litetvm


#endif//LITETVM_TARGET_TARGET_H
