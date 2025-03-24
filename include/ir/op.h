//
// Created by 赵丹 on 25-3-20.
//

#ifndef LITETVM_IR_OP_H
#define LITETVM_IR_OP_H

#include "ir/attrs.h"
#include "ir/expr.h"
#include "ir/type.h"
#include "node/attr_registry_map.h"
#include "runtime/registry.h"

#include <string>
#include <vector>

namespace litetvm {

// forward declare name.
template<typename>
class OpAttrMap;


/*!
 * \brief Primitive Op(builtin intrinsics)
 *
 * This data structure stores the meta-data
 * about primitive operators that can be invoked via Call.
 *
 * Low-level IR intrinsics(such as libc.expf) are also
 * implemented via Op.
 *
 * \sa Op
 */
class OpNode : public RelaxExprNode {
public:
    /*! \brief name of the operator */
    String name;

    /*! \brief the type of the operator */
    mutable FuncType op_type;

    /*!
   * \brief detailed description of the operator
   *  This can be used to generate docstring automatically for the operator.
   */
    String description;

    /* \brief Information of input arguments to the operator */
    Array<AttrFieldInfo> arguments;

    /*!
   * \brief The type key of the attribute field
   *  This can be empty, in which case it defaults to anything.
   */
    String attrs_type_key;

    /*!
   * \brief attribute type index,
   * this field varies in each run and is not exposed to frontend.
   */
    uint32_t attrs_type_index{0};

    /*!
   * \brief number of input arguments to the operator,
   * -1 means it is a variable length
   */
    int32_t num_inputs = -1;

    /*!
   * \brief support level of the operator,
   *  The lower, the higher priority it contains.
   *  This is in analogies to BLAS levels.
   */
    int32_t support_level = 10;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("name", &name);
        v->Visit("op_type", &op_type);
        v->Visit("description", &description);
        v->Visit("arguments", &arguments);
        v->Visit("attrs_type_key", &attrs_type_key);
        v->Visit("num_inputs", &num_inputs);
        v->Visit("support_level", &support_level);
    }

    bool SEqualReduce(const OpNode* other, SEqualReducer equal) const {
        // pointer equality is fine as there is only one op with the same name.
        return this == other;
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        // Name uniquely identifies an Op.
        hash_reduce(name);
    }

    static constexpr const char* _type_key = "Op";
    TVM_DECLARE_FINAL_OBJECT_INFO(OpNode, RelaxExprNode);

private:
    /*! \return the internal attr registry index. */
    uint32_t AttrRegistryIndex() const { return index_; }
    /*! \brief repr to be printed in registry*/
    std::string AttrRegistryName() const { return name; }

    // friend class
    template<typename>
    friend class AttrRegistryMapContainerMap;
    template<typename, typename>
    friend class AttrRegistry;
    friend class OpRegEntry;

    // Program internal unique index of operator.
    // Used to help index the program.
    uint32_t index_{0};
};

/*!
 * \brief Managed reference class to OpNode.
 * \sa OpNode
 */
class Op : public RelaxExpr {
public:
    /*!
   * \brief Get additional registered attribute about operators.
   *  If nothing has been registered, an empty OpAttrMap will be returned.
   * \param attr_name The name of the attribute.
   * \return An OpAttrMap of specified attr_name.
   * \tparam ValueType The type of the attribute.
   */
    template<typename ValueType>
    static OpAttrMap<ValueType> GetAttrMap(const String& attr_name);
    /*!
   * \brief Checks if an attr map is present in the registry.
   * \param attr_name The name of the attribute.
   * \return bool True if the attr is present.
   */
    LITETVM_API static bool HasAttrMap(const String& attr_name);
    /*!
   * \brief Get an Op for a given operator name.
   *  Will raise an error if the op has not been registered.
   * \param op_name Name of the operator.
   * \return Pointer to a Op, valid throughout program lifetime.
   */
    LITETVM_API static const Op& Get(const String& op_name);

    TVM_DEFINE_OBJECT_REF_METHODS(Op, RelaxExpr, OpNode)

private:
    /*!
   * \brief Get generic attrmap given attr name
   * \param key The attribute key
   * \return The attr map.
   */
    LITETVM_API static const AttrRegistryMapContainerMap<Op>& GetAttrMapContainer(const String& key);
};

/*!
 * \brief Helper structure to register operators
 * \sa TVM_REGISTER_OP
 */
class OpRegEntry {
public:
    /*! \return the operator */
    const Op& op() const {
        return op_;
    }

    /*!
   * \brief setter function during registration
   *  Set the description of operator
   * \param descr the description string.
   * \return reference to self.
   */
    OpRegEntry& describe(const std::string& descr) {
        get()->description = descr;
        return *this;
    }

    /*!
   * \brief Add argument information to the function.
   * \param name Name of the argument.
   * \param type Type of the argument.
   * \param description Description of the argument.
   * \return reference to self.
   */
    OpRegEntry& add_argument(const std::string& name, const std::string& type,
                             const std::string& description) {
        auto n = make_object<AttrFieldInfoNode>();
        n->name = name;
        n->type_info = type;
        n->description = description;
        get()->arguments.push_back(AttrFieldInfo(n));
        return *this;
    }

    /*!
   * \brief Set the attrs type key and index to be AttrsType.
   * \tparam AttrsType the attribute type to b set.
   * \return reference to self.
   */
    template<typename AttrsType>
    OpRegEntry& set_attrs_type() {
        get()->attrs_type_key = AttrsType::_type_key;
        get()->attrs_type_index = AttrsType::RuntimeTypeIndex();
        return *this;
    }

    /*!
   * \brief Set the attrs type key and index to be AttrsType.
   * \param key The attribute type key to be set.
   * \return reference to self.
   */
    OpRegEntry& set_attrs_type_key(const String& key) {
        get()->attrs_type_key = key;
        get()->attrs_type_index = Object::TypeKey2Index(key);
        return *this;
    }

    /*!
   * \brief Set the num_inputs
   * \param n The number of inputs to be set.
   * \return reference to self.
   */
    OpRegEntry& set_num_inputs(int32_t n) {
        get()->num_inputs = n;
        return *this;
    }

    /*!
   * \brief Set the support level of op.
   * \param level The support level.
   * \return reference to self.
   */
    OpRegEntry& set_support_level(int32_t level) {
        get()->support_level = level;
        return *this;
    }
    /*!
   * \brief Register additional attributes to operator.
   * \param attr_name The name of the attribute.
   * \param value The value to be set.
   * \param plevel The priority level of this set,
   *  an higher priority level attribute
   *  will replace lower priority level attribute.
   *  Must be bigger than 0.
   *
   *  Cannot set with same plevel twice in the code.
   *
   * \tparam ValueType The type of the value to be set.
   */
    template<typename ValueType>
    OpRegEntry& set_attr(const std::string& attr_name,// NOLINT(*)
                         const ValueType& value, int plevel = 10);

    /*!
   * \brief Resets an attr of the registry.
   * \param attr_name The name of the attribute.
   */
    inline void reset_attr(const std::string& attr_name);

    // set the name of the op to be the same as registry
    OpRegEntry& set_name() {// NOLINT(*)
        if (get()->name.length() == 0) {
            get()->name = name;
        }
        return *this;
    }
    /*!
   * \brief Register or get a new entry.
   * \param name The name of the operator.
   * \return the corresponding entry.
   */
    LITETVM_API static OpRegEntry& RegisterOrGet(const String& name);

private:
    template<typename, typename>
    friend class AttrRegistry;
    // the name
    std::string name;
    /*! \brief The operator */
    Op op_;
    // private constructor
    LITETVM_API OpRegEntry(uint32_t reg_index);

    // return an internal pointer to op.
    OpNode* get() const {
        return const_cast<OpNode*>(op_.operator->());
    }

    // update the attribute OpAttrMap
    LITETVM_API void UpdateAttr(const String& key, TVMRetValue value, int plevel);
};

}// namespace litetvm

#endif//LITETVM_IR_OP_H
