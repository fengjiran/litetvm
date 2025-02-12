//
// Created by richard on 1/31/25.
//

#ifndef NDARRAY_H
#define NDARRAY_H

#include <utility>

#include "runtime/c_runtime_api.h"
#include "runtime/data_type.h"
#include "runtime/object.h"
#include "runtime/optional.h"
#include "runtime/shape_tuple.h"
#include "runtime/string.h"

// nested namespace
namespace litetvm::runtime {

using Device = DLDevice;

class NDArray : public ObjectRef {
public:
    class ContainerBase;
    class Container;
    /*! \brief Container type for Object system. */
    using ContainerType = Container;

    // internal namespace
    struct Internal;

    /*! \brief default constructor */
    NDArray() = default;

    /*!
   * \brief constructor.
   * \param data ObjectPtr to the data container.
   */
    explicit NDArray(ObjectPtr<Object> data) : ObjectRef(std::move(data)) {}

    /*! \brief reset the content of NDArray to be nullptr */
    // inline void reset();

    /*!
   * \return the reference counter
   * \note this number is approximate in multi-threaded setting.
   */
    NODISCARD int use_count() const {
        return data_.use_count();
    }

    /*! \return Pointer to content of DLTensor */
    inline const DLTensor* operator->() const;

    /*! \return Whether the tensor is contiguous */
    NODISCARD inline bool IsContiguous() const;

    NODISCARD ShapeTuple Shape() const;

    NODISCARD runtime::DataType DataType() const;

    /*!
   * \brief Function to copy data from one array to another.
   * \param from The source array.
   * \param to The target array.
   * \param stream The stream used in copy.
   */
    static void CopyFromTo(const DLTensor* from, DLTensor* to,
                           TVMStreamHandle stream = nullptr);

    /*!
   * \brief Copy data content from another array.
   * \param other The source array to be copied from.
   * \note The copy may happen asynchronously if it involves a GPU context.
   *       TVMSynchronize is necessary.
   */
    inline void CopyFrom(const DLTensor* other) const;
    inline void CopyFrom(const NDArray& other) const;

    /*!
   * \brief Copy data content into another array.
   * \param other The source array to be copied from.
   * \note The copy may happen asynchronously if it involves a GPU context.
   *       TVMSynchronize is necessary.
   */
    inline void CopyTo(DLTensor* other) const;
    inline void CopyTo(const NDArray& other) const;

    /*!
   * \brief Copy the data to another device.
   * \param dev The target device.
   * \param mem_scope The memory scope of the target array.
   * \return The array under another device.
   * \note The copy always triggers a TVMSynchronize.
   */
    NODISCARD NDArray CopyTo(const Device& dev, const Optional<String>& mem_scope = NullOpt) const;

    /*!
   * \brief Copy data content from a byte buffer.
   * \param data The source bytes to be copied from.
   * \param nbytes The size of the buffer in bytes
   *        Must be equal to the size of the NDArray.
   * \note The copy always triggers a TVMSynchronize.
   */
    void CopyFromBytes(const void* data, size_t nbytes) const;

    /*!
   * \brief Copy data content into another array.
   * \param data The source bytes to be copied from.
   * \param nbytes The size of the data buffer.
   *        Must be equal to the size of the NDArray.
   * \note The copy always triggers a TVMSynchronize.
   */
    void CopyToBytes(void* data, size_t nbytes) const;

    /*!
   * \brief Create a NDArray that shares the data memory with the current one.
   *
   * \param shape The shape of the new array.
   *
   * \param dtype The data type of the new array.
   *
   * \param relative_byte_offset The offset of the output NDArray,
   *     relative to the current byte offset.
   *
   *     By default, the offset of the view is the same as the offset
   *     of the current array.
   *
   * \note The new array must not allow access of addresses which
   *       would be out of bounds in the current array.  If the new
   *       array is larger than the current array, or if the
   *       `relative_byte_offset` would place the end of the new array
   *       outside the bounds of the current array, this function will
   *       raise an exception.
   */
    NODISCARD NDArray CreateView(const ShapeTuple& shape, DLDataType dtype, uint64_t relative_byte_offset = 0) const;

    /*!
   * \brief Create a reference view of NDArray that
   *  represents as DLManagedTensor.
   * \return A DLManagedTensor
   */
    NODISCARD DLManagedTensor* ToDLPack() const;

    /*!
   * \brief Create an empty NDArray.
   * \param shape The shape of the new array.
   * \param dtype The data type of the new array.
   * \param dev The device of the array.
   * \param mem_scope The memory scope of the array.
   * \return The created Array
   */
    static NDArray Empty(const ShapeTuple& shape, DLDataType dtype, Device dev,
                         const Optional<String>& mem_scope = NullOpt);

    /*!
   * \brief Create a NDArray backed by an external DLTensor without memory copying.
   *
   * If DLTensor is not contiguous or has bad aligned data, It fails.
   * This allows us to create a NDArray using the memory
   * allocated by an external source. Responsibility for memory
   * retaining lies with the external source.
   * \param dl_tensor The DLTensor for NDArray base.
   * \return The created NDArray view.
   */
    static NDArray FromExternalDLTensor(const DLTensor& dl_tensor);

    /*!
   * \brief Create new NDArray, data is copied from DLTensor.
   *
   * \param dl_tensor The DLTensor to copy from.
   * \param dev device location of the created NDArray.
   * \return The created NDArray view.
   */
    static NDArray NewFromDLTensor(DLTensor* dl_tensor, const Device& dev);

    /*!
     * \brief Create a NDArray backed by a dlpack tensor.
     *
     * This allows us to create a NDArray using the memory
     * allocated by an external deep learning framework
     * that is DLPack compatible.
     *
     * The memory is retained until the NDArray went out of scope.
     * \param tensor The DLPack tensor to copy from.
     * \return The created NDArray view.
     */
    static NDArray FromDLPack(DLManagedTensor* tensor);

    /*!
   * \brief Check conditions for construction NDArray over DLTensor without copying.
   * There are three conditions to check:
   * 1. Destination device is the same as DLTensor device
   * 2. Destination device id is the same as DLTensor device id
   * 3. Memory in DLTensor is aligned as expected for NDArray
   * \param tensor the DLTensor.
   * \param dev destination device.
   * \return true if all conditions are satisfied.
   */
    static bool AbilityOfZeroCopyForDLTensor(DLTensor* tensor, const Device& dev);

protected:
    /*!
   * \brief Get mutable internal container pointer.
   * \return a mutable container pointer.
   */
    NODISCARD inline Container* get_mutable() const;

    // Helper functions for FFI handling.
    /*!
     * \brief Construct NDArray's Data field from array handle in FFI.
     * \param handle The array handle.
     * \return The corresponding ObjectPtr to the constructed container object.
     *
     * \note We keep a special calling convention for NDArray by passing
     *       ContainerBase pointer in FFI.
     *       As a result, the argument is compatible to DLTensor*.
     */
    inline static ObjectPtr<Object> FFIDataFromHandle(TVMArrayHandle handle);

    /*!
   * \brief DecRef resource managed by an FFI array handle.
   * \param handle The array handle.
   */
    inline static void FFIDecRef(TVMArrayHandle handle);

    /*!
   * \brief Get FFI Array handle from ndarray.
   * \param nd The object with ndarray type.
   * \return The result array handle.
   */
    inline static TVMArrayHandle FFIGetHandle(const ObjectRef& nd);

private:
    static bool IsAligned(const DLTensor& tensor);

protected:
    friend class TVMPODValue_;
    template<typename Derived>
    friend class TVMPODValue_CRTP_;
    friend class TVMRetValue;
    friend class TVMArgsSetter;
};

/*!
 * \brief The container base structure
 *        contains all the fields except for the Object header.
 *
 * \note We explicitly declare this structure in order to pass
 *       PackedFunc argument using ContainerBase*.
 */
class NDArray::ContainerBase {
public:
    /*!
     * \brief The corresponding dl_tensor field.
     * \note it is important that the first field is DLTensor
     *  So that this data structure is DLTensor compatible.
     *  The head ptr of this struct can be viewed as DLTensor*.
     */
    DLTensor dl_tensor{};

    /*!
     * \brief additional context, reserved for recycling
     * \note We can attach additional content here
     *  which the current container depend on
     *  (e.g. reference to original memory when creating views).
     */
    void* manager_ctx{nullptr};

protected:
    /*!
     * \brief The shape container,
     *  can be used for shape data.
     */
    ShapeTuple shape_;
};

/*!
 * \brief Object container class that backs NDArray.
 * \note do not use this function directly, use NDArray.
 */
class NDArray::Container : public Object, public ContainerBase {
public:
    /*! \brief default constructor */
    Container() {
        // Initialize the type index.
        type_index_ = RuntimeTypeIndex();
        dl_tensor.data = nullptr;
        dl_tensor.ndim = 0;
        dl_tensor.shape = nullptr;
        dl_tensor.strides = nullptr;
        dl_tensor.byte_offset = 0;
    }

    Container(void* data, ShapeTuple shape, DLDataType dtype, Device dev) {
        // Initialize the type index.
        type_index_ = RuntimeTypeIndex();
        dl_tensor.data = data;
        shape_ = std::move(shape);
        dl_tensor.ndim = static_cast<int>(shape_.size());
        dl_tensor.shape = const_cast<ShapeTuple::index_type*>(shape_.data());
        dl_tensor.dtype = dtype;
        dl_tensor.strides = nullptr;
        dl_tensor.byte_offset = 0;
        dl_tensor.device = dev;
    }

    /*!
   * \brief Set the deleter field.
   * \param deleter The deleter.
   */
    void SetDeleter(FDeleter deleter) {
        deleter_ = std::move(deleter);
    }

    // Expose DecRef and IncRef as public function
    // NOTE: they are only for developer purposes only.
    using Object::DecRef;
    using Object::IncRef;

    // Information for object protocol.
    static constexpr uint32_t _type_index = static_cast<uint32_t>(TypeIndex::kRuntimeNDArray);
    static constexpr uint32_t _type_child_slots = 0;
    static constexpr uint32_t _type_child_slots_can_overflow = true;
    static constexpr const char* _type_key = "runtime.NDArray";
    TVM_DECLARE_BASE_OBJECT_INFO(NDArray::Container, Object);

protected:
    // friend class RPCWrappedFunc;
    friend class NDArray;
};

// implementations of inline functions
/*!
 * \brief return the size of data the DLTensor hold, in terms of number of bytes
 *
 *  \param arr the input DLTensor
 *  \return number of bytes of data in the DLTensor.
 */
inline size_t GetDataSize(const DLTensor& arr) {
    size_t size = 1;
    for (tvm_index_t i = 0; i < arr.ndim; ++i) {
        size *= static_cast<size_t>(arr.shape[i]);
    }
    size *= (arr.dtype.bits * arr.dtype.lanes + 7) / 8;
    return size;
}

/*!
 * \brief check if a DLTensor is contiguous.
 * \param arr The input DLTensor.
 * \return The check result.
 */
static bool IsContiguous(const DLTensor& arr) {
    if (arr.strides == nullptr) return true;
    int64_t expected_stride = 1;
    for (int32_t i = arr.ndim; i != 0; --i) {
        int32_t k = i - 1;
        if (arr.shape[k] == 1) {
            // Skip stride check if shape[k] is 1, where the dimension is contiguous
            // regardless of the value of stride.
            //
            // For example, PyTorch will normalize stride to 1 if shape is 1 when exporting
            // to DLPack.
            // More context: https://github.com/pytorch/pytorch/pull/83158
            continue;
        }
        if (arr.strides[k] != expected_stride) return false;
        expected_stride *= arr.shape[k];
    }
    return true;
}

inline bool NDArray::IsContiguous() const {
    return runtime::IsContiguous(get_mutable()->dl_tensor);
}

inline void NDArray::CopyFrom(const DLTensor* other) const {
    CHECK(data_ != nullptr);
    CopyFromTo(other, &get_mutable()->dl_tensor);
}

inline void NDArray::CopyFrom(const NDArray& other) const {
    CHECK(data_ != nullptr);
    CHECK(other.data_ != nullptr);
    CopyFromTo(&other.get_mutable()->dl_tensor, &get_mutable()->dl_tensor);
}

inline void NDArray::CopyTo(DLTensor* other) const {
    CHECK(data_ != nullptr);
    CopyFromTo(&get_mutable()->dl_tensor, other);
}

inline void NDArray::CopyTo(const NDArray& other) const {
    CHECK(data_ != nullptr);
    CHECK(other.data_ != nullptr);
    CopyFromTo(&get_mutable()->dl_tensor, &other.get_mutable()->dl_tensor);
}

inline const DLTensor* NDArray::operator->() const {
    return &get_mutable()->dl_tensor;
}

inline NDArray::Container* NDArray::get_mutable() const {
    return static_cast<Container*>(data_.get());
}

inline ObjectPtr<Object> NDArray::FFIDataFromHandle(TVMArrayHandle handle) {
    return GetObjectPtr<Object>(static_cast<Container*>(reinterpret_cast<ContainerBase*>(handle)));
}

inline void NDArray::FFIDecRef(TVMArrayHandle handle) {
    static_cast<Container*>(reinterpret_cast<ContainerBase*>(handle))->DecRef();
}

inline TVMArrayHandle NDArray::FFIGetHandle(const ObjectRef& nd) {
    // NOTE: it is necessary to cast to container then to base
    //       so that the FFI handle uses the ContainerBase address.
    auto ptr = reinterpret_cast<TVMArrayHandle>(static_cast<ContainerBase*>(
            static_cast<Container*>(const_cast<Object*>(nd.get()))));
    return ptr;
}


inline Object* TVMArrayHandleToObjectHandle(TVMArrayHandle handle) {
    return static_cast<NDArray::Container*>(reinterpret_cast<NDArray::ContainerBase*>(handle));
}


}// namespace litetvm::runtime

#endif//NDARRAY_H
