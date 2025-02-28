//
// Created by 赵丹 on 25-2-28.
//

#ifndef STRUCTURAL_EQUAL_H
#define STRUCTURAL_EQUAL_H

#include "node/functor.h"
#include "node/object_path.h"
#include "runtime/array.h"
#include "runtime/data_type.h"

#include <cmath>
#include <numeric>
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

    ObjectPathPairNode(ObjectPath lhs_path, ObjectPath rhs_path);

    static constexpr const char* _type_key = "ObjectPathPair";
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
 *  Vars(tir::Var, TypeVar) and non-constant relay expression nodes are graph nodes.
 *  For example, it means that `%1 = %x + %y; %1 + %1` is not structurally equal
 *  to `%1 = %x + %y; %2 = %x + %y; %1 + %2` in relay.
 *
 *  A var-type node(e.g. tir::Var, TypeVar) can be mapped as equal to another var
 *  with the same type if one of the following condition holds:
 *
 *  - They appear in a same definition point(e.g. function argument).
 *  - They point to the same VarNode via the same_as relation.
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
     * \param map_free_params Whether to map free variables.
     * \return The comparison result.
     */
    LITETVM_API bool operator()(const ObjectRef& lhs, const ObjectRef& rhs,
                                const bool map_free_params = false) const;
};



}// namespace litetvm


#endif//STRUCTURAL_EQUAL_H
