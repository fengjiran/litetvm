//
// Created by 赵丹 on 25-4-17.
//

#include "relax/distributed/global_info.h"
#include "runtime/registry.h"

namespace litetvm {
namespace relax {
namespace distributed {

DeviceMesh::DeviceMesh(ShapeTuple shape, Array<Integer> device_ids) {
    int prod = 1;
    for (int i = 0; i < static_cast<int>(shape.size()); i++) {
        prod *= shape[i];
    }
    ObjectPtr<DeviceMeshNode> n = make_object<DeviceMeshNode>();
    CHECK_EQ(prod, static_cast<int>(device_ids.size()))
            << "The number of device ids must match the product of the shape";
    n->shape = std::move(shape);
    n->device_ids = std::move(device_ids);
    data_ = std::move(n);
}

DeviceMesh::DeviceMesh(ShapeTuple shape, Range device_range) {
    ObjectPtr<DeviceMeshNode> n = make_object<DeviceMeshNode>();
    Array<Integer> device_ids;
    int range_start = device_range->min.as<IntImmNode>()->value;
    int range_extent = device_range->extent.as<IntImmNode>()->value;
    for (int i = range_start; i < range_start + range_extent; i++) {
        device_ids.push_back(i);
    }
    int prod = 1;
    for (int i = 0; i < static_cast<int>(shape.size()); i++) {
        prod *= shape[i];
    }
    CHECK_EQ(prod, static_cast<int>(device_ids.size()))
            << "The number of device ids must match the product of the shape";
    n->device_ids = std::move(device_ids);
    n->shape = std::move(shape);
    n->device_range = std::move(device_range);
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(DeviceMeshNode);
TVM_REGISTER_GLOBAL("relax.distributed.DeviceMesh")
        .set_body_typed([](ShapeTuple shape, Array<Integer> device_ids, Optional<Range> device_range) {
            if (device_range.defined())
                return DeviceMesh(shape, device_range.value());
            else
                return DeviceMesh(shape, device_ids);
        });

}// namespace distributed
}// namespace relax
}// namespace litetvm