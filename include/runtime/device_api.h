//
// Created by richard on 2/1/25.
//

#ifndef LITETVM_RUNTIME_DEVICE_API_H
#define LITETVM_RUNTIME_DEVICE_API_H

#include "ffi/any.h"
#include "ffi/optional.h"
#include "runtime/base.h"
#include "runtime/logging.h"

#include <string>

using TVMStreamHandle = void*;

namespace litetvm {

using Device = DLDevice;

namespace runtime {

#ifdef __cplusplus
typedef enum : int32_t {
#else
typedef enum {
#endif
    // To help avoid accidental conflicts between `DLDeviceType`
    // and this enumeration, start numbering the new enumerators
    // a little higher than (currently) seems necessary.
    TVMDeviceExtType_End = 36,// sentinel value
} TVMDeviceExtType;


/*!
 * \brief the query type into GetAttr
 */
enum DeviceAttrKind : int {
    kExist = 0,
    kMaxThreadsPerBlock = 1,
    kWarpSize = 2,
    kMaxSharedMemoryPerBlock = 3,
    kComputeVersion = 4,
    kDeviceName = 5,
    kMaxClockRate = 6,
    kMultiProcessorCount = 7,
    kMaxThreadDimensions = 8,
    kMaxRegistersPerBlock = 9,
    kGcnArch = 10,
    kApiVersion = 11,
    kDriverVersion = 12,
    kL2CacheSizeBytes = 13,
    kTotalGlobalMemory = 14,
    kAvailableGlobalMemory = 15,
    kImagePitchAlignment = 16,
};

#ifdef TVM_KALLOC_ALIGNMENT
/*! \brief Number of bytes each allocation must align to */
constexpr int kAllocAlignment = TVM_KALLOC_ALIGNMENT;

/*! \brief Number of bytes each allocation must align to in temporary allocation */
constexpr int kTempAllocaAlignment = TVM_KALLOC_ALIGNMENT;
#else
/*! \brief Number of bytes each allocation must align to */
constexpr int kAllocAlignment = 64;

/*! \brief Number of bytes each allocation must align to in temporary allocation */
constexpr int kTempAllocaAlignment = 64;
#endif// TVM_KALLOC_ALIGNMENT

/*! \brief Maximum size that can be allocated on stack */
constexpr int kMaxStackAlloca = 1024;

/*! \brief Number of bytes each allocation must align to by default in the workspace buffer to
 * service intermediate tensors */
constexpr int kDefaultWorkspaceAlignment = 1;

/*!
 *  \brief TVM Runtime Device API, abstracts the device
 *  specific interface for memory management.
 */
class DeviceAPI {
public:
    /*! \brief virtual destructor */
    virtual ~DeviceAPI() = default;

    /*!
   * \brief Set the environment device id to device
   * \param dev The device to be set.
   */
    virtual void SetDevice(Device dev) = 0;

    /*!
   * \brief Get attribute of a specified device.
   * \param dev The device device
   * \param kind The result kind
   * \param rv The return value.
   * \sa DeviceAttrKind
   */
    virtual void GetAttr(Device dev, DeviceAttrKind kind, Any* rv) = 0;

    /*!
   * \brief Get the physical memory size required.
   * \param arr the tensor object.
   * \param mem_scope the memory scope if any
   * \return the memory size.
   */
    virtual size_t GetDataSize(const DLTensor& arr, const Optional<String>& mem_scope = std::nullopt);

    /*!
   * \brief Query the device for specified properties.
   *
   * This is used to expand "-from_device=N" in the target string to
   * all properties that can be determined from that device.
   */
    virtual void GetTargetProperty(Device dev, const std::string& property, Any* rv) {}

    /*!
   * \brief Allocate a data space on device.
   * \param dev The device device to perform operation.
   * \param nbytes The number of bytes in memory.
   * \param alignment The alignment of the memory.
   * \param type_hint The type of elements. Only needed by certain backends such
   * as OpenGL, as nbytes & alignment are sufficient for most backends.
   * \return The allocated device pointer.
   */
    virtual void* AllocDataSpace(Device dev, size_t nbytes, size_t alignment, DLDataType type_hint) = 0;

    /*!
   * \brief Allocate a data space on device with memory scope support.
   * \param dev The device device to perform operation.
   * \param ndim The number of dimension of allocated tensor.
   * \param shape The shape of allocated tensor.
   * \param dtype The type of elements.
   * \param mem_scope The memory scope of allocated tensor.
   * \return The allocated device pointer.
   */
    virtual void* AllocDataSpace(Device dev, int ndim, const int64_t* shape, DLDataType dtype,
                                 const Optional<String>& mem_scope = std::nullopt);

    /*!
   * \brief Free a data space on device.
   * \param dev The device device to perform operation.
   * \param ptr The data space.
   */
    virtual void FreeDataSpace(Device dev, void* ptr) = 0;

    /*!
   * \brief copy data from one place to another
   * \note This API is designed to support special memory with shape dependent layout.
   *       We pass in DLTensor* with shape information to support these cases.
   * \param from The source array.
   * \param to The target array.
   * \param stream Optional stream object.
   * \note The copy may happen asynchronously if it involves a GPU context.
   *       Call StreamSync to ensure the copy completes from host's pov.
   */
    virtual void CopyDataFromTo(DLTensor* from, DLTensor* to, TVMStreamHandle stream);

    /*!
  * \brief Create a new stream of execution.
  *
  * \param dev The device of allocation.
  */
    virtual TVMStreamHandle CreateStream(Device dev) {
        return nullptr;
    }

    /*!
  * \brief Free a stream of execution
  *
  * \param dev The device of the stream
  * \param stream The pointer to be freed.
  */
    virtual void FreeStream(Device dev, TVMStreamHandle stream) {}

    /*!
   * \brief Synchronize the stream
   * \param dev The device to perform operation.
   * \param stream The stream to be sync.
   */
    virtual void StreamSync(Device dev, TVMStreamHandle stream) = 0;

    /*!
   * \brief Set the stream
   * \param dev The device to set stream.
   * \param stream The stream to be set.
   */
    virtual void SetStream(Device dev, TVMStreamHandle stream) {}

    /*!
   * \brief Get the current stream
   * \param dev The device to get stream.
   * \return The current stream of the device.
   */
    virtual TVMStreamHandle GetCurrentStream(Device dev) {
        return nullptr;
    }

    /*!
   * \brief Synchronize 2 streams of execution.
   *
   * An event is created in event_src stream that the second then
   * stream waits on.  Neither event_src or event_dst need to be of
   * the same device ID as the device, but they must be of the same
   * device type.
   *
   * \param dev The device of the streams.
   * \param event_src The source stream to synchronize.
   * \param event_dst The destination stream to synchronize.
   */
    virtual void SyncStreamFromTo(Device dev, TVMStreamHandle event_src, TVMStreamHandle event_dst) {}

    /*!
   * \brief Allocate temporal workspace for backend execution.
   *
   *  \note We have the following assumption about backend temporal
   *   workspace allocation, and backend will optimize for such assumption:
   *
   *  - Only a few allocation will happen, and space will be released after use.
   *  - The release order is usually in reverse order of allocate (stack style).
   *  - Repeative pattern of same allocations over different runs.
   *  - Workspace should not overlap between different threads(i.e. be threadlocal)
   *
   * \param dev The device of allocation.
   * \param nbytes The size to be allocated.
   * \param type_hint The type of elements. Only needed by certain backends such
   * as OpenGL, as nbytes is sufficient for most backends.
   */
    virtual void* AllocWorkspace(Device dev, size_t nbytes, DLDataType type_hint = {});

    /*!
   * \brief Free temporal workspace in backend execution.
   *
   * \param dev The device of allocation.
   * \param ptr The pointer to be freed.
   */
    virtual void FreeWorkspace(Device dev, void* ptr);

    /*!
   * \brief Get device API based on device.
   * \param dev The device
   * \param allow_missing Whether allow missing
   * \return The corresponding device API.
   */
    static DeviceAPI* Get(Device dev, bool allow_missing = false);

    /*!
   * \brief Whether a certian device type requires set device device
   *        before launching the kernel function.
   * \param device_type The device type.
   */
    static bool NeedSetDevice(int device_type) {
        return device_type != kDLCPU;
    }

    /*!
  * \brief Whether pointer arithmetics on a device owned pointer may be performed on the host.
  */
    virtual bool SupportsDevicePointerArithmeticsOnHost() {
        return false;
    }

protected:
    /*!
   * \brief copy data from one place to another
   * \param from The source array.
   * \param from_offset The byte offeset in the from.
   * \param to The target array.
   * \param to_offset The byte offset in the to.
   * \param num_bytes The size of the memory in bytes
   * \param dev_from The source device
   * \param dev_to The target device
   * \param type_hint The type of elements, only neded by certain backends.
   *                  can be useful for cross device endian converison.
   * \param stream Optional stream object.
   */
    virtual void CopyDataFromTo(const void* from, size_t from_offset, void* to, size_t to_offset,
                                size_t num_bytes, Device dev_from, Device dev_to,
                                DLDataType type_hint, TVMStreamHandle stream);
};

/*!
 * \brief The name of DLDeviceType.
 * \param type The device type.
 * \return the device name.
 */
inline const char* DLDeviceType2Str(int type) {
    switch (type) {
        case kDLCPU:
            return "cpu";
        case kDLCUDA:
            return "cuda";
        case kDLCUDAHost:
            return "cuda_host";
        case kDLCUDAManaged:
            return "cuda_managed";
        case kDLOpenCL:
            return "opencl";
        case kDLVulkan:
            return "vulkan";
        case kDLMetal:
            return "metal";
        case kDLVPI:
            return "vpi";
        case kDLROCM:
            return "rocm";
        case kDLROCMHost:
            return "rocm_host";
        case kDLExtDev:
            return "ext_dev";
        case kDLOneAPI:
            return "oneapi";
        case kDLWebGPU:
            return "webgpu";
        case kDLHexagon:
            return "hexagon";
        default:
            LOG(FATAL) << "unknown type = " << type;
    }
    throw;
}


/*! \brief The device type bigger than this is RPC device */
constexpr int kRPCSessMask = 128;
static_assert(kRPCSessMask >= TVMDeviceExtType_End);

/*!
 * \brief Return true if a Device is owned by an RPC session.
 */
inline bool IsRPCSessionDevice(Device dev) { return static_cast<int>(dev.device_type) / kRPCSessMask > 0; }

/*!
 * \brief Return the RPCSessTable index of the RPC Session that owns this device.
 * \return the table index.
 */
inline int GetRPCSessionIndex(Device dev) {
    CHECK(IsRPCSessionDevice(dev)) << "GetRPCSessionIndex: dev has no RPC session";
    return static_cast<int>(dev.device_type) / kRPCSessMask - 1;
}

/*!
 * \brief Remove the RPC session mask from a Device.
 * RPC clients typically do this when encoding a Device for transmission to an RPC remote.
 * On the wire, RPCdevice are expected to be valid on the server without interpretation.
 * \param dev A Device with non-zero RPC Session mask, valid on the RPC client.
 * \return A Device without any RPC Session mask, valid on the RPC server.
 */
inline Device RemoveRPCSessionMask(Device dev) {
    dev.device_type = static_cast<DLDeviceType>(static_cast<int>(dev.device_type) % kRPCSessMask);
    return dev;
}

inline std::ostream& operator<<(std::ostream& os, DLDevice dev) {// NOLINT(*)
    if (IsRPCSessionDevice(dev)) {
        os << "remote[" << GetRPCSessionIndex(dev) << "]-";
        dev = RemoveRPCSessionMask(dev);
    }
    os << DLDeviceType2Str(dev.device_type) << ":" << dev.device_id;
    return os;
}

/*!
 * \brief Add a RPC session mask to a Device.
 * RPC clients typically do this when decoding a Device received from a RPC remote.
 * \param dev A Device without any RPC Session mask, valid on the RPC server.
 * \param session_table_index Numeric index of the RPC session in the session table.
 * \return A Device with RPC session mask added, valid on the RPC client.
 */
inline Device AddRPCSessionMask(Device dev, int session_table_index) {
    CHECK(!IsRPCSessionDevice(dev)) << "AddRPCSessionMask: dev already non-zero RPCSessionIndex: "
                                    << dev;
    dev.device_type = static_cast<DLDeviceType>(static_cast<int>(dev.device_type) | kRPCSessMask * (session_table_index + 1));
    return dev;
}


}// namespace runtime

}// namespace litetvm


#endif//LITETVM_RUNTIME_DEVICE_API_H
