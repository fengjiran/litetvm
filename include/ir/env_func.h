//
// Created by 赵丹 on 25-3-24.
//

#ifndef LITETVM_IR_ENV_FUNC_H
#define LITETVM_IR_ENV_FUNC_H

#include "node/reflection.h"
#include "runtime/packed_func.h"

#include <string>

namespace litetvm {

using runtime::PackedFunc;

/*!
 * \brief A serializable function backed by TVM's global environment.
 *
 * This is a wrapper to enable serializable global PackedFunc.
 * An EnvFunc is saved by its name in the global registry
 * under the assumption that the same function is registered during load.
 * \sa EnvFunc
 */
class EnvFuncNode : public Object {
public:
    /*! \brief Unique name of the global function */
    String name;
    /*! \brief The internal packed function */
    PackedFunc func;
    /*! \brief constructor */
    EnvFuncNode() = default;

    void VisitAttrs(AttrVisitor* v) { v->Visit("name", &name); }

    bool SEqualReduce(const EnvFuncNode* other, SEqualReducer equal) const {
        // name uniquely identifies the env function.
        return name == other->name;
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        // Name uniquely identifies the env function.
        hash_reduce(name);
    }

    static constexpr const char* _type_key = "EnvFunc";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    TVM_DECLARE_FINAL_OBJECT_INFO(EnvFuncNode, Object);
};

/*!
 * \brief Managed reference to EnvFuncNode.
 * \sa EnvFuncNode
 */
class EnvFunc : public ObjectRef {
public:
    EnvFunc() = default;
    explicit EnvFunc(ObjectPtr<Object> n) : ObjectRef(n) {}
    /*! \return The internal global function pointer */
    const EnvFuncNode* operator->() const {
        return static_cast<const EnvFuncNode*>(get());
    }
    /*!
     * \brief Invoke the function.
     * \param args The arguments
     * \returns The return value.
     */
    template<typename... Args>
    runtime::TVMRetValue operator()(Args&&... args) const {
        const EnvFuncNode* n = operator->();
        CHECK(n != nullptr);
        return n->func(std::forward<Args>(args)...);
    }
    /*!
     * \brief Get a global function based on the name.
     * \param name The name of the global function.
     * \return The created global function.
     * \note The function can be unique
     */
    LITETVM_API static EnvFunc Get(const String& name);
    /*! \brief specify container node */
    using ContainerType = EnvFuncNode;
};

}// namespace litetvm

#endif//LITETVM_IR_ENV_FUNC_H
