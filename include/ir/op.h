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
   * -1 means it is variable length
   */
    int32_t num_inputs = -1;
    /*!
   * \brief support level of the operator,
   *  The lower the more priority it contains.
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

}// namespace litetvm

#endif//LITETVM_IR_OP_H
