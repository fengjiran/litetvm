//
// Created by richard on 1/31/25.
//

#include "runtime/ndarray.h"
#include "runtime/device_api.h"

#include <iostream>

static void TVMNDArrayDLPackDeleter(const DLManagedTensor* tensor);

namespace litetvm::runtime {

inline void VerifyDataType(DLDataType dtype) {
    CHECK_GE(dtype.lanes, 1);
    if (dtype.code == static_cast<uint8_t>(DLDataTypeCode::kDLFloat)) {
        CHECK_EQ(dtype.bits % 8, 0);
    } else {
        // allow uint1 as a special flag for bool.
        if (dtype.bits == 1 && dtype.code == static_cast<uint8_t>(DLDataTypeCode::kDLUInt))
            return;
        // allow int1/uint4/int4
        if (dtype.bits == 1 && dtype.code == static_cast<uint8_t>(DLDataTypeCode::kDLInt))
            return;
        if (dtype.bits == 4 && dtype.code == static_cast<uint8_t>(DLDataTypeCode::kDLUInt))
            return;
        if (dtype.bits == 4 && dtype.code == static_cast<uint8_t>(DLDataTypeCode::kDLInt))
            return;
        CHECK_EQ(dtype.bits % 8, 0);
    }
    CHECK_EQ(dtype.bits & (dtype.bits - 1), 0);
}

void ArrayCopyFromBytes(DLTensor* handle, const void* data, size_t nbytes) {
    size_t arr_size = GetDataSize(*handle);
    CHECK_EQ(arr_size, nbytes) << "ArrayCopyFromBytes: size mismatch";
    CHECK(IsContiguous(*handle)) << "ArrayCopyFromBytes only support contiguous array for now";

    DLTensor from;
    from.data = const_cast<void*>(data);
    from.device = Device{DLDeviceType::kDLCPU, 0};
    from.ndim = handle->ndim;
    from.dtype = handle->dtype;
    from.shape = handle->shape;
    from.strides = nullptr;
    from.byte_offset = 0;

    auto* device_api = DeviceAPIManager::Get(handle->device);
    device_api->CopyDataFromTo(&from, handle, nullptr);
    // Synchronize in case data become unavailable later.
    device_api->StreamSync(handle->device, nullptr);
}

void ArrayCopyToBytes(const DLTensor* handle, void* data, size_t nbytes) {
    size_t arr_size = GetDataSize(*handle);
    CHECK_EQ(arr_size, nbytes) << "ArrayCopyToBytes: size mismatch";
    CHECK(IsContiguous(*handle)) << "ArrayCopyToBytes only support contiguous array for now";

    DLTensor to;
    to.data = data;
    to.device = Device{DLDeviceType::kDLCPU, 0};
    to.ndim = handle->ndim;
    to.dtype = handle->dtype;
    to.shape = handle->shape;
    to.strides = nullptr;
    to.byte_offset = 0;

    auto* device_api = DeviceAPIManager::Get(handle->device);
    device_api->CopyDataFromTo(const_cast<DLTensor*>(handle), &to, nullptr);
    // Synchronize in case data become unavailable later.
    device_api->StreamSync(handle->device, nullptr);
}

struct NDArray::Internal {
    // Default deleter for the container
    static void DefaultDeleter(Object* p) {
        auto* ptr = static_cast<Container*>(p);
        if (ptr->manager_ctx != nullptr) {
            static_cast<Container*>(ptr->manager_ctx)->DecRef();
        } else if (ptr->dl_tensor.data != nullptr) {
            auto* device_api = DeviceAPIManager::Get(ptr->dl_tensor.device);
            device_api->FreeDataSpace(ptr->dl_tensor.device, ptr->dl_tensor.data);
        }
        delete ptr;
    }

    // Deleter for NDArray converted from DLPack
    // This is used from data which is passed from external DLPack(DLManagedTensor)
    // that are not allocated inside TVM.
    // This enables us to create NDArray from memory allocated by other
    // frameworks that are DLPack compatible
    static void DLPackDeleter(Object* p) {
        auto* ptr = static_cast<Container*>(p);
        auto* tensor = static_cast<DLManagedTensor*>(ptr->manager_ctx);
        if (tensor->deleter) {
            tensor->deleter(tensor);
        }
        delete ptr;
    }

    // Deleter for NDArray based on external DLTensor
    // The memory is allocated from outside and it is assumed that
    // responsibility for its freeing is also outside
    static void SelfDeleter(Object* ptr_obj) {
        auto* ptr = static_cast<Container*>(ptr_obj);
        delete ptr;
    }

    // Local create function which allocates tensor metadata
    // but does not allocate space for the data.
    static NDArray Create(ShapeTuple shape, DLDataType dtype, Device dev) {
        VerifyDataType(dtype);

        // critical zone: construct header
        auto* data = new Container();
        data->SetDeleter(DefaultDeleter);

        // RAII now in effect
        NDArray ret(GetObjectPtr<Object>(data));
        // setup shape
        data->shape_ = std::move(shape);
        data->dl_tensor.shape = const_cast<ShapeTuple::index_type*>(data->shape_.data());
        data->dl_tensor.ndim = static_cast<int>(data->shape_.size());
        data->dl_tensor.dtype = dtype;
        data->dl_tensor.device = dev;
        return ret;
    }

    // Implementation of API function
    static DLTensor* MoveToFFIHandle(NDArray arr) {
        DLTensor* handle = FFIGetHandle(arr);
        FFIClearAfterMove(&arr);
        return handle;
    }

    static void FFIDecRef(TVMArrayHandle tensor) {
        NDArray::FFIDecRef(tensor);
    }

    // Container to DLManagedTensor
    static DLManagedTensor* ToDLPack(TVMArrayHandle handle) {
        auto* from = static_cast<Container*>(reinterpret_cast<ContainerBase*>(handle));
        return ToDLPack(from);
    }

    static DLManagedTensor* ToDLPack(Container* from) {
        CHECK(from != nullptr);
        auto* ret = new DLManagedTensor();
        ret->dl_tensor = from->dl_tensor;
        ret->manager_ctx = from;
        from->IncRef();
        ret->deleter = NDArrayDLPackDeleter;
        return ret;
    }

    // Delete dlpack object.
    static void NDArrayDLPackDeleter(const DLManagedTensor* tensor) {
        std::cout << "delete DLManagedTensor" << std::endl;
        static_cast<Container*>(tensor->manager_ctx)->DecRef();
        delete tensor;
    }
};


void NDArray::CopyFromTo(const DLTensor* from, DLTensor* to, TVMStreamHandle stream) {
    size_t from_size = GetDataSize(*from);
    size_t to_size = GetDataSize(*to);
    CHECK_EQ(from_size, to_size) << "TVMArrayCopyFromTo: The size in bytes must exactly match.";

    CHECK(from->device.device_type == to->device.device_type ||
          from->device.device_type == DLDeviceType::kDLCPU ||
          to->device.device_type == DLDeviceType::kDLCPU ||
          from->device.device_type == DLDeviceType::kDLCUDAHost ||
          to->device.device_type == DLDeviceType::kDLCUDAHost ||
          from->device.device_type == DLDeviceType::kDLROCMHost ||
          to->device.device_type == DLDeviceType::kDLROCMHost)
            << "Can not copy across different device types directly. From device type: "
            << static_cast<int>(from->device.device_type) << " to device type: "
            << static_cast<int>(to->device.device_type);

    // Use the device that is *not* a cpu device to get the correct device
    // api manager.
    Device dev = from->device.device_type != DLDeviceType::kDLCPU ? from->device : to->device;
    auto* device_api = DeviceAPIManager::Get(dev);
    device_api->CopyDataFromTo(const_cast<DLTensor*>(from), to, stream);
}

NDArray NDArray::CopyTo(const Device& dev, const Optional<String>& mem_scope) const {
    CHECK(data_ != nullptr);
    const DLTensor* dptr = operator->();
    NDArray ret = Empty(ShapeTuple(dptr->shape, dptr->shape + dptr->ndim),
                        dptr->dtype,
                        dev,
                        mem_scope);
    this->CopyTo(ret);
    Device copy_gpu_dev = dptr->device.device_type != DLDeviceType::kDLCPU ? dptr->device : dev;
    auto* device_api = DeviceAPIManager::Get(dev);
    device_api->StreamSync(copy_gpu_dev, nullptr);
    return ret;
}

NDArray NDArray::CreateView(const ShapeTuple& shape, DLDataType dtype, uint64_t relative_byte_offset) const {
    CHECK(data_ != nullptr);
    auto* self = get_mutable();
    const DLTensor& origin_tensor = self->dl_tensor;
    auto msg = [&origin_tensor] {
        std::stringstream ss;
        ss << "Can only create view for compact tensor, but found strides ";
        ss << "[";
        int n = origin_tensor.ndim;
        for (int i = 0; i < origin_tensor.ndim; ++i) {
            ss << origin_tensor.strides[i] << (--n ? ", " : "");
        }
        ss << "]";

        ss << ", for shape ";
        ss << "[";
        n = origin_tensor.ndim;
        for (int i = 0; i < origin_tensor.ndim; ++i) {
            ss << origin_tensor.shape[i] << (--n ? ", " : "");
        }
        ss << "]";
        return ss.str();
    };

    CHECK(IsContiguous()) << msg();

    NDArray ret = Internal::Create(shape, dtype, origin_tensor.device);

    size_t origin_size = GetDataSize(origin_tensor);
    size_t view_size = GetDataSize(ret.get_mutable()->dl_tensor);
    // CHECK_LE(relative_byte_offset + view_size, curr_size)
    //         << std::format("ValueError: View with shape {0} and datatype {1} would have a size of {2} bytes. "
    //                        "This would occupy bytes {3} <= i_byte < {4} within the backing array. "
    //                        "However, the NDArray being viewed only contains {5} bytes (shape = {6}, "
    //                        "dtype = {7}).",
    //                        shape, dtype, view_size, relative_byte_offset, relative_byte_offset + view_size,
    //                        curr_size, ShapeTuple(curr_dl_tensor.shape, curr_dl_tensor.shape + curr_dl_tensor.ndim),
    //                        curr_dl_tensor.dtype);

    CHECK_LE(relative_byte_offset + view_size, origin_size)
            << "ValueError: "
            << "View with shape " << shape << " and datatype " << dtype << " would have a size of "
            << view_size << " bytes. "
            << "This would occupy bytes " << relative_byte_offset << " <= i_byte < "
            << relative_byte_offset + view_size << " within the backing array. "
            << "However, the NDArray being viewed only contains " << origin_size << " bytes (shape = "
            << ShapeTuple(origin_tensor.shape, origin_tensor.shape + origin_tensor.ndim)
            << ", dtype= " << origin_tensor.dtype << ").";

    // increase ref count
    self->IncRef();
    ret.get_mutable()->manager_ctx = self;
    ret.get_mutable()->dl_tensor.data = origin_tensor.data;
    ret.get_mutable()->dl_tensor.byte_offset = origin_tensor.byte_offset + relative_byte_offset;
    return ret;
}

void NDArray::CopyToBytes(void* data, size_t nbytes) const {
    CHECK(data != nullptr);
    CHECK(data_ != nullptr);
    ArrayCopyToBytes(&get_mutable()->dl_tensor, data, nbytes);
}

void NDArray::CopyFromBytes(const void* data, size_t nbytes) const {
    CHECK(data != nullptr);
    CHECK(data_ != nullptr);
    ArrayCopyFromBytes(&get_mutable()->dl_tensor, data, nbytes);
}

NDArray NDArray::Empty(const ShapeTuple& shape, DLDataType dtype, Device dev, const Optional<String>& mem_scope) {
    NDArray nd_array = Internal::Create(shape, dtype, dev);
    auto* device_api = DeviceAPIManager::Get(nd_array->device);
    nd_array.get_mutable()->dl_tensor.data = device_api->AllocDataSpace(nd_array->device,
                                                                        static_cast<int>(shape.size()),
                                                                        shape.data(),
                                                                        nd_array->dtype,
                                                                        mem_scope);
    return nd_array;
}

NDArray NDArray::FromExternalDLTensor(const DLTensor& dl_tensor) {
    CHECK(::litetvm::runtime::IsContiguous(dl_tensor)) << "External DLTensor must be contiguous.";
    CHECK(IsAligned(dl_tensor)) << "Data in DLTensor is not aligned as required by NDArray";
    auto* data = new Container();

    data->SetDeleter(Internal::SelfDeleter);
    data->dl_tensor = dl_tensor;
    data->shape_ = ShapeTuple(dl_tensor.shape, dl_tensor.shape + dl_tensor.ndim);
    data->dl_tensor.shape = const_cast<ShapeTuple::index_type*>(data->shape_.data());

    return NDArray(GetObjectPtr<Object>(data));
}

NDArray NDArray::NewFromDLTensor(const DLTensor* tensor, const Device& dev) {
    CHECK(::litetvm::runtime::IsContiguous(*tensor))
            << "DLTensor is not contiguous. Copying from non-contiguous data is currently not supported";
    ShapeTuple shape(tensor->shape, tensor->shape + tensor->ndim);
    NDArray ary = Empty(shape, tensor->dtype, dev);
    ary.CopyFrom(tensor);
    return ary;
}

NDArray NDArray::FromDLPack(DLManagedTensor* tensor) {
    auto data = new Container();
    // construct header
    data->SetDeleter(Internal::DLPackDeleter);
    // fill up content.
    data->manager_ctx = tensor;
    CHECK(::litetvm::runtime::IsContiguous(tensor->dl_tensor)) << "DLManagedTensor must be contiguous.";
    CHECK(IsAligned(tensor->dl_tensor)) << "Data in DLManagedTensor is not aligned as required by NDArray";
    data->dl_tensor = tensor->dl_tensor;
    // update shape_
    // std::vector<ShapeTuple::index_type> shape;
    // shape.resize(data->dl_tensor.ndim);
    // shape.assign(data->dl_tensor.shape, data->dl_tensor.shape + data->dl_tensor.ndim);
    data->shape_ = ShapeTuple(data->dl_tensor.shape, data->dl_tensor.shape + data->dl_tensor.ndim);
    data->dl_tensor.shape = const_cast<ShapeTuple::index_type*>(data->shape_.data());
    return NDArray(GetObjectPtr<Object>(data));
}


ShapeTuple NDArray::Shape() const {
    return static_cast<const Container*>(data_.get())->shape_;
}

DataType NDArray::DataType() const {
    return runtime::DataType(get_mutable()->dl_tensor.dtype);
}

DLManagedTensor* NDArray::ToDLPack() const {
    return Internal::ToDLPack(get_mutable());
}

bool NDArray::IsAligned(const DLTensor& tensor) {
    return reinterpret_cast<size_t>(static_cast<char*>(tensor.data) + tensor.byte_offset) % kAllocAlignment == 0;
}

bool NDArray::AbilityOfZeroCopyForDLTensor(DLTensor* tensor, const Device& dev) {
    bool device_check = dev.device_type == tensor->device.device_type;
    bool device_id_check = dev.device_id == tensor->device.device_id;
    bool alignment_check = IsAligned(*tensor);
    return device_check && device_id_check && alignment_check;
}

TVM_REGISTER_OBJECT_TYPE(NDArray::Container);
}// namespace litetvm::runtime

using namespace litetvm::runtime;
void TVMNDArrayDLPackDeleter(const DLManagedTensor* tensor) {
    NDArray::Internal::NDArrayDLPackDeleter(tensor);
}

int TVMArrayCopyToBytes(TVMArrayHandle handle, void* data, size_t nbytes) {
    API_BEGIN();
    ArrayCopyToBytes(handle, data, nbytes);
    API_END();
}