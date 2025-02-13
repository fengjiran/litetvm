//
// Created by richard on 2/12/25.
//

#include "runtime/meta_data.h"
#include "runtime/c_backend_api.h"
#include "runtime/c_runtime_api.h"
#include "runtime/registry.h"

namespace litetvm {
namespace runtime {
namespace metadata {

TVM_REGISTER_OBJECT_TYPE(MetadataBaseNode);

ArrayAccessor<TVMTensorInfo, TensorInfo> MetadataNode::inputs() const {
    return ArrayAccessor<TVMTensorInfo, TensorInfo>(data_->inputs, data_->num_inputs);
}

ArrayAccessor<TVMTensorInfo, TensorInfo> MetadataNode::outputs() const {
    return ArrayAccessor<TVMTensorInfo, TensorInfo>(data_->outputs, data_->num_outputs);
}

ArrayAccessor<TVMTensorInfo, TensorInfo> MetadataNode::workspace_pools() const {
    return ArrayAccessor<TVMTensorInfo, TensorInfo>(data_->workspace_pools,
                                                    data_->num_workspace_pools);
}

ArrayAccessor<TVMConstantInfo, ConstantInfoMetadata> MetadataNode::constant_pools() const {
    return ArrayAccessor<TVMConstantInfo, ConstantInfoMetadata>(data_->constant_pools,
                                                                data_->num_constant_pools);
}
// TVM_REGISTER_OBJECT_TYPE(MetadataBaseNode);

MetadataArray::MetadataArray(Array<ObjectRef> array, MetadataKind kind, const char* struct_name)
    : MetadataBase{make_object<MetadataArrayNode>(array, kind, struct_name)} {}

const char* MetadataArrayNode::get_c_struct_name() const {
    CHECK(false) << "MetadataArrayNode get_c_struct_name is unimplemented";
    return nullptr;
}
TVM_REGISTER_OBJECT_TYPE(MetadataArrayNode);

Metadata::Metadata(const ::TVMMetadata* data)
    : MetadataBase{make_object<MetadataNode>(data)} {}

TVM_REGISTER_OBJECT_TYPE(MetadataNode);

const char* MetadataNode::get_c_struct_name() const { return "TVMMetadata"; }

TensorInfo::TensorInfo(const ::TVMTensorInfo* data)
    : MetadataBase{make_object<TensorInfoNode>(data)} {}

TVM_REGISTER_OBJECT_TYPE(TensorInfoNode);

const char* TensorInfoNode::get_c_struct_name() const { return "TVMTensorInfo"; }

ConstantInfoMetadata::ConstantInfoMetadata(const ::TVMConstantInfo* data)
    : MetadataBase{make_object<ConstantInfoMetadataNode>(data)} {}

TVM_REGISTER_OBJECT_TYPE(ConstantInfoMetadataNode);

const char* ConstantInfoMetadataNode::get_c_struct_name() const { return "TVMConstantInfo"; }


}// namespace metadata

class MetadataModuleNode : public ModuleNode {
public:
    explicit MetadataModuleNode(metadata::Metadata metadata)
        : metadata_{::std::move(metadata)} {}

    const char* type_key() const final { return "metadata_module"; }

    /*! \brief Get the property of the runtime module .*/
    int GetPropertyMask() const final { return static_cast<int>(ModulePropertyMask::kBinarySerializable); }

    static Module LoadFromBinary() {
        return Module(make_object<MetadataModuleNode>(metadata::Metadata()));
    }

    void SaveToBinary(dmlc::Stream* stream) final {}

    PackedFunc GetFunction(const String& name, const ObjectPtr<Object>& sptr_to_self) {
        if (name == "get_metadata") {
            return PackedFunc([this, sptr_to_self](TVMArgs args, TVMRetValue* rv) {
                if (!metadata_.defined()) {
                    TVMFunctionHandle f_handle;
                    int32_t ret_code = TVMBackendGetFuncFromEnv(this, symbol::tvm_get_c_metadata, &f_handle);
                    CHECK_EQ(ret_code, 0) << "Unable to locate " << symbol::tvm_get_c_metadata
                                          << " PackedFunc";

                    TVMValue ret_value;
                    int ret_type_code;
                    ret_code = TVMFuncCall(f_handle, nullptr, nullptr, 0, &ret_value, &ret_type_code);
                    CHECK_EQ(ret_code, 0) << "Invoking " << symbol::tvm_get_c_metadata
                                          << ": TVMFuncCall returned " << ret_code;

                    CHECK_EQ(ret_type_code, static_cast<int>(TVMArgTypeCode::kTVMOpaqueHandle))
                            << "Expected kOpaqueHandle returned; got " << ret_type_code;
                    CHECK(ret_value.v_handle != nullptr)
                            << symbol::tvm_get_c_metadata << " returned nullptr";

                    metadata_ = runtime::metadata::Metadata(
                            static_cast<const struct TVMMetadata*>(ret_value.v_handle));
                }

                *rv = metadata_;
            });
        }

        return PackedFunc();
    }

private:
    runtime::metadata::Metadata metadata_;
};

Module MetadataModuleCreate(metadata::Metadata metadata) {
    return Module(make_object<MetadataModuleNode>(metadata));
}

TVM_REGISTER_GLOBAL("runtime.module.loadbinary_metadata_module")
        .set_body([](TVMArgs args, TVMRetValue* rv) { *rv = MetadataModuleNode::LoadFromBinary(); });

}// namespace runtime
}// namespace litetvm