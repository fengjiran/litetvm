//
// Created by 赵丹 on 25-4-17.
//

#include "relax/distributed/struct_info.h"

namespace litetvm {
namespace relax {
namespace distributed {

PlacementSpec PlacementSpec::Sharding(int axis) {
    ObjectPtr<PlacementSpecNode> n = make_object<PlacementSpecNode>();
    n->axis = axis;
    n->kind = PlacementSpecKind::kSharding;
    return PlacementSpec(n);
}

PlacementSpec PlacementSpec::Replica() {
    ObjectPtr<PlacementSpecNode> n = make_object<PlacementSpecNode>();
    n->axis = -1;
    n->kind = PlacementSpecKind::kReplica;
    return PlacementSpec(n);
}

TVM_REGISTER_NODE_TYPE(PlacementSpecNode);

TVM_REGISTER_GLOBAL("relax.distributed.Sharding").set_body_typed([](int axis) {
    return PlacementSpec::Sharding(axis);
});

TVM_REGISTER_GLOBAL("relax.distributed.Replica").set_body_typed([]() {
    return PlacementSpec::Replica();
});

String PlacementNode::ToString() const {
    std::stringstream ss;
    for (size_t i = 0; i < dim_specs.size(); ++i) {
        if (i != 0) {
            ss << ", ";
        }
        if (dim_specs[i]->kind == PlacementSpecKind::kReplica) {
            ss << "R";
        } else {
            ss << "S[" << dim_specs[i]->axis << "]";
        }
    }
    return ss.str();
}

Placement::Placement(Array<PlacementSpec> dim_specs) {
    ObjectPtr<PlacementNode> n = make_object<PlacementNode>();
    n->dim_specs = std::move(dim_specs);
    data_ = std::move(n);
}

Placement Placement::FromText(String text_repr) {
    Array<PlacementSpec> dim_specs;
    std::stringstream ss(text_repr);
    while (true) {
        char indicator = 0;
        ss >> indicator;
        if (ss.eof()) {
            break;
        }
        if (indicator == 'R') {
            dim_specs.push_back(PlacementSpec::Replica());
        } else if (indicator == 'S') {
            char lbracket;
            ss >> lbracket;
            CHECK_EQ(lbracket, '[');
            std::string substr;
            getline(ss, substr, ']');
            std::stringstream ss2(substr);
            int dim;
            ss2 >> dim;
            dim_specs.push_back(PlacementSpec::Sharding(dim));
            CHECK(ss2.eof()) << "Invalid placement text repr";
        } else if (indicator == ',') {
            continue;
        } else if (indicator == ' ') {
            continue;
        } else {
            LOG(FATAL) << "Invalid placement text repr";
        }
    }
    return Placement(dim_specs);
}

TVM_REGISTER_NODE_TYPE(PlacementNode);
TVM_REGISTER_GLOBAL("relax.distributed.PlacementFromText").set_body_typed(Placement::FromText);
TVM_REGISTER_GLOBAL("relax.distributed.Placement")
        .set_body_typed([](Array<PlacementSpec> dim_specs) { return Placement(dim_specs); });

// DTensor
DTensorStructInfo::DTensorStructInfo(TensorStructInfo tensor_sinfo, DeviceMesh device_mesh,
                                     Placement placement) {
    CHECK_EQ(device_mesh->shape.size(), placement->dim_specs.size())
            << "ValueError: The device mesh and placement must have the same dimension size";
    for (auto spec: placement->dim_specs) {
        if (spec->kind == PlacementSpecKind::kReplica) continue;
        CHECK_LT(spec->axis, tensor_sinfo->ndim)
                << "ValueError: Sharding dimension should be smaller than tensor ndim";
    }
    ObjectPtr<DTensorStructInfoNode> n = make_object<DTensorStructInfoNode>();
    n->device_mesh = std::move(device_mesh);
    n->placement = std::move(placement);
    n->tensor_sinfo = std::move(tensor_sinfo);
    // n->span = span;
    data_ = std::move(n);
}

TVM_REGISTER_NODE_TYPE(DTensorStructInfoNode);

TVM_REGISTER_GLOBAL("relax.distributed.DTensorStructInfo")
        .set_body_typed([](TensorStructInfo tensor_sinfo, DeviceMesh device_mesh, Placement placement) {
            return DTensorStructInfo(tensor_sinfo, device_mesh, placement);
        });

}// namespace distributed
}// namespace relax
}// namespace litetvm
