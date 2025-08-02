//
// Created by richard on 8/2/25.
//

#ifndef LITETVM_NODE_STRUCTURAL_EQUAL_H
#define LITETVM_NODE_STRUCTURAL_EQUAL_H

#include "node/functor.h"
#include "node/object_path.h"
#include "runtime/data_type.h"

#include <cmath>
#include <string>

namespace litetvm {

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
        } else if (std::isnan(lhs) || std::isnan(rhs)) {
            return false;
        } else if (lhs == rhs) {
            return true;
        } else {
            // fuzzy float pt comparison
            constexpr double atol = 1e-9;
            double diff = lhs - rhs;
            return diff > -atol && diff < atol;
        }
    }

    bool operator()(const int64_t& lhs, const int64_t& rhs) const { return lhs == rhs; }
    bool operator()(const uint64_t& lhs, const uint64_t& rhs) const { return lhs == rhs; }
    bool operator()(const Optional<int64_t>& lhs, const Optional<int64_t>& rhs) const {
        return lhs == rhs;
    }
    bool operator()(const Optional<double>& lhs, const Optional<double>& rhs) const {
        return lhs == rhs;
    }
    bool operator()(const int& lhs, const int& rhs) const { return lhs == rhs; }
    bool operator()(const bool& lhs, const bool& rhs) const { return lhs == rhs; }
    bool operator()(const std::string& lhs, const std::string& rhs) const { return lhs == rhs; }
    bool operator()(const DataType& lhs, const DataType& rhs) const { return lhs == rhs; }
    template<typename ENum, typename = typename std::enable_if<std::is_enum<ENum>::value>::type>
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

    ObjectPathPairNode(ObjectPath lhs_path, ObjectPath rhs_path);

    static constexpr const char* _type_key = "node.ObjectPathPair";
    TVM_DECLARE_FINAL_OBJECT_INFO(ObjectPathPairNode, Object);
};

class ObjectPathPair : public ObjectRef {
public:
    ObjectPathPair(ObjectPath lhs_path, ObjectPath rhs_path);

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
 *  Vars(tir::Var, relax::Var) nodes are graph nodes.
 *
 *  A var-type node(e.g. tir::Var) can be mapped as equal to another var
 *  with the same type if one of the following condition holds:
 *
 *  - They appear in a same definition point(e.g. function argument).
 *  - They points to the same VarNode via the same_as relation.
 *  - They appear in a same usage point, and map_free_vars is set to be True.
 */
class StructuralEqual : public BaseValueEqual {
public:
    // inheritate operator()
    using BaseValueEqual::operator();
    /*!
   * \brief Compare objects via strutural equal.
   * \param lhs The left operand.
   * \param rhs The right operand.
   * \param map_free_params Whether or not to map free variables.
   * \return The comparison result.
   */
    TVM_DLL bool operator()(const ffi::Any& lhs, const ffi::Any& rhs,
                            const bool map_free_params = false) const;
};
}// namespace litetvm

#endif//LITETVM_NODE_STRUCTURAL_EQUAL_H
