//
// Created by richard on 2/4/25.
//

#include "runtime/c_runtime_api.h"
#include "runtime/device_api.h"
#include "runtime/packed_func.h"
#include "runtime/registry.h"

#include <array>

namespace litetvm::runtime {

std::string GetCustomTypeName(uint8_t type_code) {
    auto f = RegistryManager::Global().Get("runtime._datatype_get_type_name");
    CHECK(f) << "Function runtime._datatype_get_type_name not found";
    return (*f)(type_code).operator std::string();
}

uint8_t GetCustomTypeCode(const std::string& type_name) {
    auto f = RegistryManager::Global().Get("runtime._datatype_get_type_code");
    CHECK(f) << "Function runtime._datatype_get_type_code not found";
    return (*f)(type_name).operator int();
}
//
bool GetCustomTypeRegistered(uint8_t type_code) {
    auto f = runtime::RegistryManager::Global().Get("runtime._datatype_get_type_registered");
    CHECK(f) << "Function runtime._datatype_get_type_registered not found";
    return (*f)(type_code).operator bool();
}

uint8_t ParseCustomDatatype(const std::string& s, const char** scan) {
    CHECK(s.substr(0, 6) == "custom") << "Not a valid custom datatype string";

    auto tmp = s.c_str();

    CHECK(s.c_str() == tmp);
    *scan = s.c_str() + 6;
    CHECK(s.c_str() == tmp);
    if (**scan != '[') LOG(FATAL) << "expected opening brace after 'custom' type in" << s;
    CHECK(s.c_str() == tmp);
    *scan += 1;
    CHECK(s.c_str() == tmp);
    size_t custom_name_len = 0;
    CHECK(s.c_str() == tmp);
    while (*scan + custom_name_len <= s.c_str() + s.length() && *(*scan + custom_name_len) != ']')
        ++custom_name_len;
    CHECK(s.c_str() == tmp);
    if (*(*scan + custom_name_len) != ']')
        LOG(FATAL) << "expected closing brace after 'custom' type in" << s;
    CHECK(s.c_str() == tmp);
    *scan += custom_name_len + 1;
    CHECK(s.c_str() == tmp);

    auto type_name = s.substr(7, custom_name_len);
    CHECK(s.c_str() == tmp);
    return GetCustomTypeCode(type_name);
}

DeviceAPI* DeviceAPI::Get(Device dev, bool allow_missing) {
    return DeviceAPIManager::Get(static_cast<int>(dev.device_type), allow_missing);
}

void* DeviceAPI::AllocWorkspace(Device dev, size_t size, DLDataType type_hint) {
    return AllocDataSpace(dev, size, kTempAllocaAlignment, type_hint);
}

static size_t GetDataAlignment(const DLDataType dtype) {
    size_t align = (dtype.bits / 8) * dtype.lanes;
    if (align < kAllocAlignment) return kAllocAlignment;
    return align;
}

size_t DeviceAPI::GetDataSize(const DLTensor& arr, const Optional<String>& mem_scope) {
    if (!mem_scope.defined() || mem_scope.value().empty() || mem_scope.value() == "global") {
        size_t size = 1;
        for (tvm_index_t i = 0; i < arr.ndim; ++i) {
            size *= static_cast<size_t>(arr.shape[i]);
        }
        size *= (arr.dtype.bits * arr.dtype.lanes + 7) / 8;
        return size;
    }
    LOG(FATAL) << "Device does not support physical mem computation with "
               << "specified memory scope: " << mem_scope.value();
    return 0;
}

void* DeviceAPI::AllocDataSpace(Device dev, int ndim, const int64_t* shape, DLDataType dtype,
                                const Optional<String>& mem_scope) {
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

void DeviceAPI::CopyDataFromTo(DLTensor* from, DLTensor* to, TVMStreamHandle stream) {
    // by default, we can always redirect to the flat memory copy operation.
    size_t nbytes = GetDataSize(*from);
    CHECK_EQ(nbytes, GetDataSize(*to));

    CHECK(IsContiguous(*from) && IsContiguous(*to))
            << "CopyDataFromTo only support contiguous array for now";

    CopyDataFromTo(from->data, from->byte_offset,
                   to->data, to->byte_offset,
                   nbytes, from->device,
                   to->device, from->dtype, stream);
}

void DeviceAPI::CopyDataFromTo(const void* from, size_t from_offset, void* to, size_t to_offset,
                               size_t num_bytes, Device dev_from, Device dev_to,
                               DLDataType type_hint, TVMStreamHandle stream) {
    LOG(FATAL) << "Device does not support CopyDataFromTo.";
}

void DeviceAPI::FreeWorkspace(Device dev, void* ptr) { FreeDataSpace(dev, ptr); }

TVMStreamHandle DeviceAPI::CreateStream(Device dev) { return nullptr; }

void DeviceAPI::FreeStream(Device dev, TVMStreamHandle stream) {}

TVMStreamHandle DeviceAPI::GetCurrentStream(Device dev) { return nullptr; }

void DeviceAPI::SyncStreamFromTo(Device dev, TVMStreamHandle event_src, TVMStreamHandle event_dst) {
}

}// namespace litetvm::runtime