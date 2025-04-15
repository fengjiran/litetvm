//
// Created by 赵丹 on 25-4-15.
//

/*!
 * \file tvm/ir/transform.h
 *
 * This file implements a pass manager. The pass manager manages a sequence
 * of IRModule -> IRModule transformation passes over a particlar unit of AST. The
 * design is largely inspired from LLVM's pass manager and modern deep learning
 * frameworks that perform tensor->tensor transformations.
 *
 * The responsibilities of a traditional compiler pass manager usually involves:
 *  - Organizing the execution order of optimization passes though not
 * necessarily in the optimal sequence.
 *  - Collecting required analysis information and keep them up-to-date.
 *  - Reducing the effort required to implement new passes for compiler
 * developers, etc.
 *
 * Similar to LLVM's pass manager, we designed the Relax pass manager to work
 * different granularity, i.e. module level, function level, and even sequential
 * passe that contains a host of passes.
 *
 * However, we also extend the functionality of the traditional pass manager
 * with the consideration of requirements/convention from deep learning
 * frameworks, such as Pytorch and Gluon, etc. Each pass in the Relax pass
 * manager performs the IRModule -> IRModule transformation. All
 * different types of passes, including the sequential-level pass object, are
 * essentially pass objects. This design, therefore, effectively provides users
 * a consistent and convenient interface, i.e. Pass, to play with. It offers a
 * means to ease the development and testing of Relax passes. For example, with
 * the pass manager, external users will be able to have custom passes correctly
 * scheduled without having to modify a single handcrafted pass order.
 *
 * In the future we need to describe constraints between passes. For example,
 * we may want to preserve dependencies between different passes and validate
 * them on the completion of a certain pass.
 *
 * We also need to store side information and import the error reporting system.
 */
#ifndef LITETVM_IR_TRANSFORM_H
#define LITETVM_IR_TRANSFORM_H

#include "ir/module.h"
#include "ir/instrument.h"
#include "runtime/array.h"
#include "runtime/string.h"
#include "support/with.h"

#include <string>

namespace litetvm {
namespace transform {

/*!
 * \brief PassContextNode contains the information that a pass can rely on,
 * such as analysis results.
 * \sa PassContext
 */
class PassContextNode : public Object {
public:
    /*! \brief The default optimization level. */
    int opt_level{2};

    /*! \brief The list of required passes. */
    Array<String> required_pass;
    /*! \brief The list of disabled passes. */
    Array<String> disabled_pass;
    /*! \brief The diagnostic context. */
    // mutable Optional<DiagnosticContext> diag_ctx;
    /*! \brief Pass specific configurations. */
    Map<String, ObjectRef> config;

    /*! \brief A list of pass instrument implementations. */
    Array<instrument::PassInstrument> instruments;
    // TODO(@sunggg): Fix dependency issue in the header file and correct the types
    // e.g., relax::trace, relax::database in tvm/relax/tuning_api.h
    /*! \brief Trace stack for relax pass infra. */
    mutable Array<ObjectRef> trace_stack;
    /*! \brief List of passes to be traced. If not defined, make every pass traceable. */
    Optional<Map<String, Bool>> make_traceable;
    /*! \brief Number of evaluations conducted in the pass pipeline. */
    mutable int num_evals{0};
    /*! \brief Database for tuning API. */
    Optional<ObjectRef> tuning_api_database;
    PassContextNode() = default;

    /*!
   * \brief Get a config value from the pass context.
   *
   * \param key The config key.
   * \param default_value The default value if the key does not exist, defaults to nullptr.
   *
   * \return The result
   *
   * \tparam TOBjectRef the expected object type.
   * \throw Error if the key exists but the value does not match TObjectRef.
   */
    template<typename TObjectRef>
    Optional<TObjectRef> GetConfig(const std::string& key, Optional<TObjectRef> default_value =
                                                                   Optional<TObjectRef>(nullptr)) const {
        static_assert(std::is_base_of<ObjectRef, TObjectRef>::value,
                      "Can only call GetAttr with ObjectRef types.");
        if (!config.defined()) return default_value;
        auto it = config.find(key);
        if (it != config.end()) {
            return Downcast<Optional<TObjectRef>>((*it).second);
        } else {
            return default_value;
        }
    }
    // variant that uses TObjectRef to enable implicit conversion to default value.
    template<typename TObjectRef>
    Optional<TObjectRef> GetConfig(const std::string& key, TObjectRef default_value) const {
        return GetConfig<TObjectRef>(key, Optional<TObjectRef>(default_value));
    }

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("opt_level", &opt_level);
        v->Visit("required_pass", &required_pass);
        v->Visit("disabled_pass", &disabled_pass);
        v->Visit("instruments", &instruments);
        v->Visit("config", &config);
        // v->Visit("diag_ctx", &diag_ctx);
        v->Visit("trace_stack", &trace_stack);
        v->Visit("make_traceable", &make_traceable);
        v->Visit("num_evals", &num_evals);
        v->Visit("tuning_api_daatabase", &tuning_api_database);
    }

    Array<ObjectRef> GetTraceStack() { return trace_stack; }
    void PushTrace(ObjectRef new_trace) { trace_stack.push_back(new_trace); }
    void PopTrace() {
        ICHECK(GetTraceStackSize()) << "Trace stack is currently empty. Please double check.";
        trace_stack.pop_back();
    }
    int GetTraceStackSize() { return trace_stack.size(); }
    ObjectRef GetCurrentTrace() {
        ICHECK(GetTraceStackSize()) << "Trace stack is currently empty. Please double check.";
        return trace_stack.back();
    }
    void SetNumEvals(int _num_evals) { num_evals = _num_evals; }
    void IncNumEvals(int _num_evals) { num_evals += _num_evals; }

    Optional<ObjectRef> GetTuningAPIDatabase() { return tuning_api_database; }

    static constexpr const char* _type_key = "transform.PassContext";
    static constexpr bool _type_has_method_sequal_reduce = false;
    TVM_DECLARE_FINAL_OBJECT_INFO(PassContextNode, Object);
};

/*!
 * \brief PassContext that is used to configure the pass behavior.
 *
 * \code
 *
 *  auto new_ctx = PassContext::Create();
 *  ctx->opt_level = 2;
 *  With<PassContext> scope(ctx);
 *  // pass context in effect.
 *
 * \endcode
 * \sa PassContextNode
 */
class PassContext : public ObjectRef {
public:
    PassContext() {}
    explicit PassContext(ObjectPtr<Object> n) : ObjectRef(n) {}
    /*!
   * \brief const accessor.
   * \return const access pointer.
   */
    const PassContextNode* operator->() const {
        ICHECK(get() != nullptr);
        return static_cast<const PassContextNode*>(get());
    }
    /*!
   * \brief mutable accessor.
   * \return mutable access pointer.
   */
    PassContextNode* operator->() {
        ICHECK(get() != nullptr);
        return static_cast<PassContextNode*>(get_mutable());
    }

    /*!
   * \brief Construct a PassContext containing the default configurations.
   * \return The new PassContext.
   */
    TVM_DLL static PassContext Create();
    /*!
   * \brief Get the default pass context in the current scope.
   * \return The pass context.
   */
    TVM_DLL static PassContext Current();

    /*!
   * \brief Get all supported configuration names and metadata, registered within the PassContext.
   * \return Map indexed by the config name, pointing to the metadata map as key-value
   */
    TVM_DLL static Map<String, Map<String, String>> ListConfigs();

    /*!
   * \brief Call instrument implementations' callbacks when entering PassContext.
   *        The callbacks are called in order, and if one raises an exception, the rest will not be
   *        called.
   */
    TVM_DLL void InstrumentEnterPassContext();

    /*!
   * \brief Call instrument implementations' callbacks when exiting PassContext.
   *        The callbacks are called in order, and if one raises an exception, the rest will not be
   *        called.
   */
    TVM_DLL void InstrumentExitPassContext();

    /*!
   * \brief Call instrument implementations' callbacks before a pass run.
   *        The callbacks are called in order, and if one raises an exception, the rest will not be
   *        called.
   *
   * \param mod The module that an optimization pass runs on.
   * \param info The pass information.
   *
   * \return false: the pass is skipped; true: the pass runs.
   */
    TVM_DLL bool InstrumentBeforePass(const IRModule& mod, const PassInfo& info) const;

    /*!
   * \brief Call instrument implementations callbacks after a pass run.
   *        The callbacks are called in order, and if one raises an exception, the rest will not be
   *        called.
   *
   * \param mod The module that an optimization pass runs on.
   * \param info The pass information.
   */
    TVM_DLL void InstrumentAfterPass(const IRModule& mod, const PassInfo& info) const;

    /*!
   * \brief Check whether a pass is enabled.
   * \param info The pass information.
   * \return true if the pass is enabled. Otherwise, false.
   */
    TVM_DLL bool PassEnabled(const PassInfo& info) const;

    /*!
   * \brief Register a valid configuration option and its ValueType for validation.
   *
   * \param key The configuration key.
   * \tparam ValueType The value type to be registered
   */
    template<typename ValueType>
    static uint32_t RegisterConfigOption(const char* key) {
        using ValueNodeType = typename ValueType::ContainerType;
        // NOTE: we could further update the function later.
        uint32_t tindex = ValueNodeType::_GetOrAllocRuntimeTypeIndex();
        auto type_key = runtime::Object::TypeIndex2Key(tindex);

        auto* reflection = ReflectionVTable::Global();

        auto legalization = [=](ObjectRef obj) -> ObjectRef {
            if (obj->IsInstance<Map<String, ObjectRef>::ContainerType>()) {
                return reflection->CreateObject(type_key, Downcast<Map<String, ObjectRef>>(obj));
            } else {
                // Backwards compatibility for config options defined prior to
                // https://github.com/apache/tvm/pull/16183.  This commit
                // changed the default FFI conversion of python integers from
                // `tvm::IntImm` to `runtime::Int`.
                //
                // This backwards compatibility fix can be removed when all
                // options registered with TVM_REGISTER_PASS_CONFIG_OPTION are
                // updated to use `runtime::Int` and `runtime::Bool`.
                TVMRetValue ret;
                ret = obj;
                try {
                    ValueType legalized = ret;
                    return legalized;
                } catch (Error& err) {
                    LOG(FATAL) << "AttributeError: expect config " << key << " to have type " << type_key
                               << ", but received error when converting to this type.\n"
                               << err.what();
                }
            }
        };

        RegisterConfigOption(key, tindex, legalization);
        return tindex;
    }

    // accessor.
    using ContainerType = PassContextNode;
    class Internal;

private:
    // The entry of a pass context scope.
    TVM_DLL void EnterWithScope();
    // The exit of a pass context scope.
    TVM_DLL void ExitWithScope();
    // Register configuration key value type.
    TVM_DLL static void RegisterConfigOption(const char* key, uint32_t value_type_index,
                                             std::function<ObjectRef(ObjectRef)> legalization);

    // Classes to get the Python `with` like syntax.
    friend class Internal;
    friend class With<PassContext>;
};

}// namespace transform


}// namespace litetvm

#endif//LITETVM_IR_TRANSFORM_H
