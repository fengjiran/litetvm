//
// Created by 赵丹 on 25-7-31.
//

#include "runtime/device_api.h"
#include "ffi/container/ndarray.h"
#include "ffi/function.h"
#include "ffi/optional.h"
#include "ffi/reflection/registry.h"
#include "ffi/rvalue_ref.h"
#include "ffi/string.h"
#include "runtime/base.h"
#include "runtime/c_backend_api.h"
#include "runtime/module.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <tuple>
#include <variant>

namespace litetvm {
namespace runtime {

static size_t GetDataAlignment(const DLDataType dtype) {
    size_t align = dtype.bits / 8 * dtype.lanes;
    if (align < kAllocAlignment) {
        return kAllocAlignment;
    }
    return align;
}

size_t DeviceAPI::GetDataSize(const DLTensor& arr, const Optional<String>& mem_scope) {
    if (!mem_scope.defined() || mem_scope.value().empty() || mem_scope.value() == "global") {
        size_t size = 1;
        for (int i = 0; i < arr.ndim; ++i) {
            size *= static_cast<size_t>(arr.shape[i]);
        }
        return ffi::GetDataSize(size, arr.dtype);
    }
    LOG(FATAL) << "Device does not support physical mem computation with "
               << "specified memory scope: " << mem_scope.value();
    return 0;
}

void* DeviceAPI::AllocDataSpace(Device dev, int ndim, const int64_t* shape, DLDataType dtype, const Optional<String>& mem_scope) {
    if (!mem_scope.defined() || mem_scope.value().empty() || mem_scope.value() == "global") {
        // by default, we can always redirect to the flat memory allocations
        DLTensor temp;
        temp.data = nullptr;
        temp.device = dev;
        temp.ndim = ndim;
        temp.dtype = dtype;
        temp.shape = const_cast<int64_t*>(shape);
        temp.strides = nullptr;
        temp.byte_offset = 0;
        size_t size = GetDataSize(temp);
        size_t alignment = GetDataAlignment(temp.dtype);
        return AllocDataSpace(dev, size, alignment, dtype);
    }
    LOG(FATAL) << "Device does not support allocate data space with "
               << "specified memory scope: " << mem_scope.value();
    return nullptr;
}

void* DeviceAPI::AllocWorkspace(Device dev, size_t nbytes, DLDataType type_hint) {
    return AllocDataSpace(dev, nbytes, kTempAllocaAlignment, type_hint);
}

void DeviceAPI::FreeWorkspace(Device dev, void* ptr) {
    FreeDataSpace(dev, ptr);
}

void DeviceAPI::CopyDataFromTo(DLTensor* from, DLTensor* to, TVMStreamHandle stream) {
    // by default, we can always redirect to the flat memory copy operation.
    size_t nbytes = GetDataSize(*from);
    ICHECK_EQ(nbytes, GetDataSize(*to));

    ICHECK(ffi::IsContiguous(*from) && ffi::IsContiguous(*to)) << "CopyDataFromTo only support contiguous array for now";
    CopyDataFromTo(from->data, from->byte_offset, to->data, to->byte_offset, nbytes,
                   from->device, to->device, from->dtype, stream);
}

void DeviceAPI::CopyDataFromTo(const void* from, size_t from_offset, void* to, size_t to_offset,
                               size_t num_bytes, Device dev_from, Device dev_to,
                               DLDataType type_hint, TVMStreamHandle stream) {
    LOG(FATAL) << "Device does not support CopyDataFromTo.";
}

DeviceAPI* DeviceAPI::Get(Device dev, bool allow_missing) {
    return DeviceAPIManager::Get(dev.device_type, allow_missing);
}



}// namespace runtime
}// namespace litetvm