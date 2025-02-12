//
// Created by 赵丹 on 25-1-13.
//

#ifndef C_RUNTIME_API_H
#define C_RUNTIME_API_H

#include <cstddef>
#include <cstdint>
#include <functional>

using tvm_index_t = int64_t;

enum class TVMDeviceExtType : int32_t {
    // To help avoid accidental conflicts between `DLDeviceType`
    // and this enumeration, start numbering the new enumerators
    // a little higher than (currently) seems necessary.
    kDLAOCL = 32,
    kDLSDAccel,
    kOpenGL,
    kDLMicroDev,
    TVMDeviceExtType_End,// sentinel value
};

/*!
 * \brief The type code options DLDataType.
 */
enum class DLDataTypeCode : uint8_t {
    /*! \brief signed integer */
    kDLInt = 0U,
    /*! \brief unsigned integer */
    kDLUInt = 1U,
    /*! \brief IEEE floating point */
    kDLFloat = 2U,
    /*!
 * \brief Opaque handle type, reserved for testing purposes.
 * Frameworks need to agree on the handle data type for the exchange to be well-defined.
 */
    kDLOpaqueHandle = 3U,
    /*! \brief bfloat16 */
    kDLBfloat = 4U,
    /*!
 * \brief complex number
 * (C/C++/Python layout: compact struct per complex number)
 */
    kDLComplex = 5U,
};

enum class DLDeviceType : uint8_t {
    /*! \brief CPU device */
    kDLCPU = 1,
    /*! \brief CUDA GPU device */
    kDLCUDA = 2,
    /*!
   * \brief Pinned CUDA CPU memory by cudaMallocHost
   */
    kDLCUDAHost = 3,
    /*! \brief OpenCL devices. */
    kDLOpenCL = 4,
    /*! \brief Vulkan buffer for next generation graphics. */
    kDLVulkan = 7,
    /*! \brief Metal for Apple GPU. */
    kDLMetal = 8,
    /*! \brief Verilog simulator buffer */
    kDLVPI = 9,
    /*! \brief ROCm GPUs for AMD GPUs */
    kDLROCM = 10,
    /*!
   * \brief Pinned ROCm CPU memory allocated by hipMallocHost
   */
    kDLROCMHost = 11,
    /*!
   * \brief Reserved extension device type,
   * used for quickly test extension device
   * The semantics can differ depending on the implementation.
   */
    kDLExtDev = 12,
    /*!
   * \brief CUDA managed/unified memory allocated by cudaMallocManaged
   */
    kDLCUDAManaged = 13,
    /*!
   * \brief Unified shared memory allocated on a oneAPI non-partititioned
   * device. Call to oneAPI runtime is required to determine the device
   * type, the USM allocation type and the sycl context it is bound to.
   *
   */
    kDLOneAPI = 14,
    /*! \brief GPU support for next generation WebGPU standard. */
    kDLWebGPU = 15,
    /*! \brief Qualcomm Hexagon DSP */
    kDLHexagon = 16
};

/*!
 * \brief A Device for Tensor and operator.
 */
struct DLDevice {
    /*! \brief The device type used in the device. */
    DLDeviceType device_type;
    /*!
   * \brief The device index.
   * For vanilla CPU memory, pinned memory, or managed memory, this is set to 0.
   */
    int32_t device_id;
};

/*!
 * \brief The data type the tensor can hold. The data type is assumed to follow the
 * native endian-ness. An explicit error message should be raised when attempting to
 * export an array with non-native endianness
 *
 *  Examples
 *   - float: type_code = 2, bits = 32, lanes=1
 *   - float4(vectorized 4 float): type_code = 2, bits = 32, lanes=4
 *   - int8: type_code = 0, bits = 8, lanes=1
 *   - std::complex<float>: type_code = 5, bits = 64, lanes = 1
 */
struct DLDataType {
    /*!
 * \brief Type code of base types.
 * We keep it uint8_t instead of DLDataTypeCode for minimal memory
 * footprint, but the value should be one of DLDataTypeCode enum values.
 * */
    uint8_t code;
    /*!
 * \brief Number of bits, common choices are 8, 16, 32.
 */
    uint8_t bits;
    /*! \brief Number of lanes in the type, used for vector types. */
    uint16_t lanes;
};

/*!
 * \brief Plain C Tensor object, does not manage memory.
 */
struct DLTensor {
    /*!
   * \brief The data pointer points to the allocated data. This will be CUDA
   * device pointer or cl_mem handle in OpenCL. It may be opaque on some device
   * types. This pointer is always aligned to 256 bytes as in CUDA. The
   * `byte_offset` field should be used to point to the beginning of the data.
   *
   * Note that as of Nov 2021, multiply libraries (CuPy, PyTorch, TensorFlow,
   * TVM, perhaps others) do not adhere to this 256 byte aligment requirement
   * on CPU/CUDA/ROCm, and always use `byte_offset=0`.  This must be fixed
   * (after which this note will be updated); at the moment it is recommended
   * to not rely on the data pointer being correctly aligned.
   *
   * For given DLTensor, the size of memory required to store the contents of
   * data is calculated as follows:
   *
   * \code{.c}
   * static inline size_t GetDataSize(const DLTensor* t) {
   *   size_t size = 1;
   *   for (tvm_index_t i = 0; i < t->ndim; ++i) {
   *     size *= t->shape[i];
   *   }
   *   size *= (t->dtype.bits * t->dtype.lanes + 7) / 8;
   *   return size;
   * }
   * \endcode
   */
    void* data;
    /*! \brief The device of the tensor */
    DLDevice device;
    /*! \brief Number of dimensions */
    int32_t ndim;
    /*! \brief The data type of the pointer*/
    DLDataType dtype;
    /*! \brief The shape of the tensor */
    int64_t* shape;
    /*!
   * \brief strides of the tensor (in number of elements, not bytes)
   *  can be NULL, indicating tensor is compact and row-majored.
   */
    int64_t* strides;
    /*! \brief The offset in bytes to the beginning pointer to data */
    uint64_t byte_offset;
};

/*!
 * \brief C Tensor object, manage memory of DLTensor. This data structure is
 *  intended to facilitate the borrowing of DLTensor by another framework. It is
 *  not meant to transfer the tensor. When the borrowing framework doesn't need
 *  the tensor, it should call the deleter to notify the host that the resource
 *  is no longer needed.
 */
struct DLManagedTensor {
    using FDeleter = std::function<void(DLManagedTensor*)>;

    /*! \brief DLTensor which is being memory managed */
    DLTensor dl_tensor;

    /*! \brief the context of the original host framework of DLManagedTensor in
   *   which DLManagedTensor is used in the framework. It can also be NULL.
   */
    void* manager_ctx;

    /*! \brief Destructor signature void (*)(void*) - this should be called
   *   to destruct manager_ctx which holds the DLManagedTensor. It can be NULL
   *   if there is no way for the caller to provide a reasonable destructor.
   *   The destructors deletes the argument self as well.
   */
    FDeleter deleter;
};


/*!
 * \brief The type code in used and only used in TVM FFI for argument passing.
 *
 * DLPack consistency:
 * 1) kTVMArgInt is compatible with kDLInt
 * 2) kTVMArgFloat is compatible with kDLFloat
 * 3) kDLUInt is not in ArgTypeCode, but has a spared slot
 *
 * Downstream consistency:
 * The kDLInt, kDLUInt, kDLFloat are kept consistent with the original ArgType code
 *
 * It is only used in argument passing, and should not be confused with
 * DataType::TypeCode, which is DLPack-compatible.
 *
 * \sa tvm::runtime::DataType::TypeCode
 */
enum class TVMArgTypeCode : uint8_t {
    kTVMArgInt = static_cast<uint8_t>(DLDataTypeCode::kDLInt),
    kTVMArgFloat = static_cast<uint8_t>(DLDataTypeCode::kDLFloat),
    kTVMOpaqueHandle = 3U,
    kTVMNullptr = 4U,
    kTVMDataType = 5U,
    kDLDevice = 6U,
    kTVMDLTensorHandle = 7U,
    kTVMObjectHandle = 8U,
    kTVMModuleHandle = 9U,
    kTVMPackedFuncHandle = 10U,
    kTVMStr = 11U,
    kTVMBytes = 12U,
    kTVMNDArrayHandle = 13U,
    kTVMObjectRValueRefArg = 14U,
    kTVMArgBool = 15U,
    // Extension codes for other frameworks to integrate TVM PackedFunc.
    // To make sure each framework's id do not conflict, use first and
    // last sections to mark ranges.
    // Open an issue at the repo if you need a section of code.
    kTVMExtBegin = 16U,
    kTVMNNVMFirst = 16U,
    kTVMNNVMLast = 20U,
    // The following section of code is used for non-reserved types.
    kTVMExtReserveEnd = 64U,
    kTVMExtEnd = 128U,
};

/*! \brief the array handle */
using TVMArrayHandle = DLTensor*;

/*! \brief Handle to TVM runtime modules. */
using TVMModuleHandle = void*;
/*! \brief Handle to packed function handle. */
using TVMFunctionHandle = void*;
/*! \brief Handle to hold return value. */
using TVMRetValueHandle = void*;
/*!
 * \brief The stream that is specific to device
 * can be NULL, which indicates the default one.
 */
using TVMStreamHandle = void*;
/*! \brief Handle to Object. */
using TVMObjectHandle = void*;

/*!
 * \brief Union type of values
 *  being passed through API and function calls.
 */
union TVMValue {
    int64_t v_int64;
    double v_float64;
    void* v_handle;
    const char* v_str;
    DLDataType v_type;
    DLDevice v_device;
};

/*!
 * \brief Byte array type used to pass in byte array
 *  When kTVMBytes is used as data type.
 */
struct TVMByteArray {
    const char* data;
    size_t size;
};

#endif//C_RUNTIME_API_H
