//
// Created by 赵丹 on 25-4-17.
//

#ifndef LITETVM_RELAX_DISTRIBUTED_GLOBAL_INFO_H
#define LITETVM_RELAX_DISTRIBUTED_GLOBAL_INFO_H

#include "ir/expr.h"
#include "ir/global_info.h"
#include "ir/module.h"
#include "runtime/shape_tuple.h"

namespace litetvm {
namespace relax {
namespace distributed {

using runtime::ShapeTuple;

/*
 * \brief Device mesh express a view of topology of devices, represented by an n-d matrix of
 * device ids
 */
class DeviceMeshNode : public GlobalInfoNode {
public:
    /*! \brief logical shape of the mesh*/
    ShapeTuple shape;

    /*! \brief device ids in the mesh*/
    Array<Integer> device_ids;

    /*! \brief Optionally use range to represent device_ids*/
    Optional<Range> device_range;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("shape", &shape);
        v->Visit("device_ids", &device_ids);
        v->Visit("device_range", &device_range);
    }
    static constexpr const char* _type_key = "relax.distributed.DeviceMesh";

    bool SEqualReduce(const DeviceMeshNode* other, SEqualReducer equal) const {
        if (shape.size() != other->shape.size()) {
            return false;
        }
        for (int i = 0; i < static_cast<int>(shape.size()); i++) {
            if (!equal(shape[i], other->shape[i])) {
                return false;
            }
        }
        return equal(device_ids, other->device_ids);
    }

    void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(device_ids);
        for (int i = 0; i < static_cast<int>(shape.size()); i++) {
            hash_reduce(shape[i]);
        }
    }

    TVM_DECLARE_FINAL_OBJECT_INFO(DeviceMeshNode, GlobalInfoNode);
};

/*!
 * \brief Managed reference to a DeviceMesh.
 * \sa DeviceMeshNode
 */
class DeviceMesh : public GlobalInfo {
public:
    LITETVM_API DeviceMesh(ShapeTuple shape, Array<Integer> device_ids);
    LITETVM_API DeviceMesh(ShapeTuple shape, Range device_range);
    TVM_DEFINE_OBJECT_REF_METHODS(DeviceMesh, GlobalInfo, DeviceMeshNode);
};

}// namespace distributed
}// namespace relax
}// namespace litetvm


#endif//LITETVM_RELAX_DISTRIBUTED_GLOBAL_INFO_H
