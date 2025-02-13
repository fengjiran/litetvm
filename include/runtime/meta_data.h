//
// Created by richard on 2/12/25.
//

#ifndef META_DATA_H
#define META_DATA_H

#include "runtime/array.h"
#include "runtime/c_runtime_api.h"
#include "runtime/data_type.h"
#include "runtime/object.h"
#include "runtime/string.h"
#include "runtime/ndarray.h"
#include "runtime/module.h"

#include <dmlc/io.h>
#include <dmlc/json.h>
#include <dmlc/memory_io.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

// meta data types
/*!
 * \brief Describes one tensor argument to `run_model`.
 * NOTE: while TIR allows for other types of arguments, such as scalars, the AOT run_model
 * function does not currently accept these. Therefore, it's not possible to express those
 * in this metadata. A future patch may modify this.
 */
struct TVMTensorInfo {
    /*! \brief Name of the tensor, as specified in the Relay program. */
    const char* name;
    /*! \brief Shape of the tensor. */
    const int64_t* shape;
    /*! \brief Rank of this tensor. */
    int64_t num_shape;
    /*! \brief Data type of one element of this tensor. */
    DLDataType dtype;
};


/*!
 * \brief Top-level metadata structure. Holds all other metadata types.
 */
struct TVMMetadata {
    /*! \brief Version identifier for this metadata. */
    int64_t version;
    /*! \brief Inputs to the AOT run_model function.
     * The order of the elements is the same as in the arguments to run_model. That is to say,
     * this array specifies the first `num_inputs` arguments to run_model.
     */
    const struct TVMTensorInfo* inputs;
    /*! \brief Number of elements in `inputs` array. */
    int64_t num_inputs;
    /*! \brief Outputs of the AOT run_model function.
     * The order of the elements is the same as in the arguments to run_model. That is to say,
     * this array specifies the last `num_outputs` arguments to run_model.
     */
    const struct TVMTensorInfo* outputs;
    /*! \brief Number of elements in `outputs` array. */
    int64_t num_outputs;
    /*! \brief Workspace Memory Pools needed by the AOT main function.
     * The order of the elements is the same as in the arguments to run_model. That is to say,
     * this array specifies the last `num_workspace_pools` arguments to run_model.
     */
    const struct TVMTensorInfo* workspace_pools;
    /*! \brief Number of elements in `workspace_pools` array. */
    int64_t num_workspace_pools;
    /*! \brief Constant pools needed by the AOT main function.
     */
    const struct TVMConstantInfo* constant_pools;
    /*! \brief Number of elements in `constant_pools` array. */
    int64_t num_constant_pools;
    /*! \brief Name of the model, as passed to tvm.relay.build. */
    const char* mod_name;
};

/*!
 * \brief Describes one constant argument to `run_model`.
 *
 */
struct TVMConstantInfo {
    /*! \brief Name of the constant */
    const char* name_hint;
    /*! \brief Offset in bytes of the constant */
    int64_t byte_offset;
    /*! \brief length of the data_bytes field */
    int64_t data_len;
    /*! \brief data bytes of serialized NDArray */
    const void* data_bytes;
};

#ifdef __cplusplus
}  // extern "C"
#endif

namespace litetvm {
namespace runtime {
namespace metadata {

/*!
 * \brief Common base class for all Metadata.
 *
 * This class is used in the visitor classes as a internal check to ensure that verify that all
 * parts of the Metadata struct used in codegen are Metadata objects.
 */
class MetadataBaseNode : public Object {
public:
    NODISCARD virtual const char* get_c_struct_name() const = 0;
    virtual ~MetadataBaseNode() = default;

    static constexpr const char* _type_key = "metadata.MetadataBaseNode";
    TVM_DECLARE_BASE_OBJECT_INFO(MetadataBaseNode, Object);
};

/*! \brief Reference class for the common MetadataBaseNode class. */
class MetadataBase : public ObjectRef {
public:
    TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(MetadataBase, ObjectRef, MetadataBaseNode);
};

template<typename C, class Ref>
class ArrayAccessor;

/*! \brief An iterator implementation that lazily instantiates the C++ wrapping Metadata class. */
template<typename C, class Ref>
class ArrayIterator {
public:
    ArrayIterator(size_t index, const ArrayAccessor<C, Ref>* parent)
        : index_{index}, parent_{parent} {}

    Ref operator*() {
        return (*parent_)[index_];
    }

    ArrayIterator& operator++() {
        if (index_ < parent_->size()) {
            index_++;
        }

        return *this;
    }

    bool operator==(const ArrayIterator& other) const {
        return parent_ == other.parent_ && index_ == other.index_;
    }

    bool operator!=(const ArrayIterator& other) const {
        return !operator==(other);
    }

private:
    size_t index_;
    const ArrayAccessor<C, Ref>* parent_;
};

/*! \brief A span-like class which permits access to Array fields with complex elements.
 * These array fields should be accessed from C++ using the Metadata wrapper classes. This class
 * lazily instantiates those wrappers as they are accessed.
 */
template<typename C, class Ref>
class ArrayAccessor {
public:
    using value_type = Ref;
    using iterator = ArrayIterator<C, Ref>;
    using const_iterator = iterator;

    template<typename T = std::enable_if_t<std::is_base_of_v<ObjectRef, Ref>>>
    ArrayAccessor(const C* data, size_t num_data) : data_{data}, num_data_{num_data} {}

    NODISCARD size_t size() const {
        return num_data_;
    }

    Ref operator[](size_t index) const {
        if (index >= num_data_) {
            throw std::runtime_error("Index out of range");
        }

        return Ref(&data_[index]);
    }

    ArrayIterator<C, Ref> begin() const {
        return ArrayIterator<C, Ref>{0, this};
    }

    ArrayIterator<C, Ref> end() const {
        return ArrayIterator<C, Ref>{num_data_, this};
    }

private:
    const C* data_;
    size_t num_data_;
};

/*! \brief A specialization of ArrayAccessor for String.
 * This class is needed because the String constructor signature is different from the typical
 * Metadata subclass.
 */
template<>
class ArrayAccessor<const char*, String> {
public:
    using value_type = String;
    using iterator = ArrayIterator<const char*, String>;
    using const_iterator = iterator;

    ArrayAccessor(const char** data, size_t num_data) : data_{data}, num_data_{num_data} {}

    NODISCARD size_t size() const { return num_data_; }

    String operator[](size_t index) const {
        if (index >= num_data_) {
            throw std::runtime_error("Index out of range");
        }
        return {data_[index]};
    }

    NODISCARD ArrayIterator<const char*, String> begin() const {
        return ArrayIterator{0, this};
    }

    NODISCARD ArrayIterator<const char*, String> end() const {
        return ArrayIterator{num_data_, this};
    }

private:
    const char** data_;
    size_t num_data_;
};

/*! \brief Enumerates the primitive types which can be part of a Metadata instance.
 *
 * These are separate from TIR DataType because TIR does not model structs.
 */
enum class MetadataKind : uint8_t {
    kUint64 = 0,
    kInt64 = 1,
    kBool = 2,
    kString = 3,
    kHandle = 4,
    kMetadata = 5,
};

/*! \brief Container for arrays in the metadata.
 *
 * Type information is needed when emitting arrays. This container augments the data field with
 * the necessary typing information.
 */
class MetadataArrayNode : public MetadataBaseNode {
public:
    MetadataArrayNode(Array<ObjectRef> array, MetadataKind kind, const char* type_key)
        : array(::std::move(array)), kind{kind}, type_key{type_key} {}

    NODISCARD const char* get_c_struct_name() const final;

    NODISCARD std::string get_element_c_struct_name() const {
        CHECK(kind == MetadataKind::kMetadata)
                << "cannot get struct name for MetadataArray with kind=" << static_cast<uint8_t>(kind);
        constexpr int prefix_size = sizeof("metadata.") - 1;
        constexpr int suffix_size = sizeof("Node") - 1;
        std::string type_key_str(type_key);
        return std::string("TVM") +
               type_key_str.substr(prefix_size, type_key_str.size() - prefix_size - suffix_size);
    }

    Array<ObjectRef> array;

    /*! \brief Describes the storage class of the emitted struct member. */
    MetadataKind kind;

    /*! \brief When `kind` is Metadata, type_key of the MetadataBaseNode used with this array. */
    const char* type_key;

    static constexpr const char* _type_key = "metadata.MetadataArrayNode";
    TVM_DECLARE_BASE_OBJECT_INFO(MetadataArrayNode, MetadataBaseNode);
};

/*! \brief Reference class for MetadataArray. */
class MetadataArray : public MetadataBase {
public:
    MetadataArray(Array<ObjectRef> array, MetadataKind kind, const char* struct_name);

    TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(MetadataArray, MetadataBase, MetadataArrayNode);
};

// Version number recorded in emitted artifacts for runtime checking.
#define TVM_METADATA_VERSION 1

/*!
 * \brief Version of metadata emitted and understood by this compiler/runtime.
 * Should be populated into the `version` field of all TVMMetadata.
 */
static constexpr int64_t kMetadataVersion = TVM_METADATA_VERSION;

class Metadata;
class TensorInfo;
class ConstantInfoMetadata;

class MetadataNode : public MetadataBaseNode {
public:
    explicit MetadataNode(const ::TVMMetadata* data) : data_{data} {}
    static constexpr const char* _type_key = "metadata.MetadataNode";
    NODISCARD const char* get_c_struct_name() const override;
    NODISCARD int64_t version() const { return data_->version; }
    NODISCARD int64_t num_inputs() const { return data_->num_inputs; }
    ArrayAccessor<TVMTensorInfo, TensorInfo> inputs() const;
    NODISCARD int64_t num_outputs() const { return data_->num_outputs; }
    ArrayAccessor<TVMTensorInfo, TensorInfo> outputs() const;
    NODISCARD int64_t num_workspace_pools() const { return data_->num_workspace_pools; }
    ArrayAccessor<TVMTensorInfo, TensorInfo> workspace_pools() const;
    NODISCARD String mod_name() const { return {data_->mod_name}; }
    NODISCARD const ::TVMMetadata* data() const { return data_; }
    ArrayAccessor<TVMConstantInfo, ConstantInfoMetadata> constant_pools() const;
    NODISCARD int64_t num_constant_pools() const { return data_->num_constant_pools; }
    TVM_DECLARE_FINAL_OBJECT_INFO(MetadataNode, MetadataBaseNode);

private:
    const ::TVMMetadata* data_;
};

class Metadata : public MetadataBase {
public:
    explicit Metadata(const ::TVMMetadata* data);
    TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(Metadata, MetadataBase, MetadataNode);
};

class TensorInfoNode : public MetadataBaseNode {
public:
    explicit TensorInfoNode(const ::TVMTensorInfo* data) : data_{data} {}
    static constexpr const char* _type_key = "metadata.TensorInfoNode";
    NODISCARD const char* get_c_struct_name() const override;
    NODISCARD String name() const { return {data_->name}; }
    NODISCARD int64_t num_shape() const { return data_->num_shape; }
    // NODISCARD ::litetvm::support::Span<const int64_t, int64_t> shape() const {
    //     return ::tvm::support::Span<const int64_t, int64_t>(data_->shape,
    //                                                         data_->shape + data_->num_shape);
    // }
    NODISCARD DataType dtype() const { return runtime::DataType(data_->dtype); }
    NODISCARD const ::TVMTensorInfo* data() const { return data_; }
    TVM_DECLARE_FINAL_OBJECT_INFO(TensorInfoNode, MetadataBaseNode);

private:
    const ::TVMTensorInfo* data_;
};

class TensorInfo : public MetadataBase {
public:
    explicit TensorInfo(const ::TVMTensorInfo* data);
    TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(TensorInfo, MetadataBase, TensorInfoNode);
};

class ConstantInfoMetadataNode : public MetadataBaseNode {
public:
    explicit ConstantInfoMetadataNode(const struct ::TVMConstantInfo* data) : data_{data} {}
    // This name should match TVMConstantInfo after processing
    static constexpr const char* _type_key = "metadata.ConstantInfoNode";
    NODISCARD const char* get_c_struct_name() const override;
    NODISCARD String name_hint() const {
        return {data_->name_hint};
    }
    NODISCARD size_t byte_offset() const { return data_->byte_offset; }
    NODISCARD NDArray data() const {
        NDArray ndarray;
        if (data_->data_len) {
            dmlc::MemoryFixedSizeStream bytes(const_cast<void*>(data_->data_bytes), data_->data_len);
            ndarray.Load(&bytes);
        }
        return ndarray;
    }
    TVM_DECLARE_FINAL_OBJECT_INFO(ConstantInfoMetadataNode, MetadataBaseNode);

protected:
    const ::TVMConstantInfo* data_;
};

class ConstantInfoMetadata : public MetadataBase {
public:
    explicit ConstantInfoMetadata(const ::TVMConstantInfo* data);
    TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(ConstantInfoMetadata, MetadataBase,
                                          ConstantInfoMetadataNode);
};

}// namespace metadata

inline String get_name_mangled(const String& module_name, const String& name) {
    std::stringstream ss;
    CHECK(module_name.defined());
    CHECK(name.defined());
    ss << module_name << "_" << name;
    return ss.str();
}

/*!
 * \brief Create a metadata module object.
 *
 * \param metadata Exported metadata structure.
 *
 * \return The created metadata module.
 */
Module MetadataModuleCreate(metadata::Metadata metadata);

namespace launch_param {

/*! \brief A tag to specify whether or not dynamic shared memory is used */
constexpr const char* kUseDynamicSharedMemoryTag = "tir.use_dyn_shared_memory";

}  // namespace launch_param

/*! \brief function information needed by device */
struct FunctionInfo {
    std::string name;
    std::vector<DLDataType> arg_types;
    std::vector<std::string> launch_param_tags;

    void Save(dmlc::JSONWriter* writer) const;
    void Load(dmlc::JSONReader* reader);
    void Save(dmlc::Stream* writer) const;
    bool Load(dmlc::Stream* reader);
};


}// namespace runtime
}// namespace litetvm

namespace dmlc {
DMLC_DECLARE_TRAITS(has_saveload, ::litetvm::runtime::FunctionInfo, true);
}  // namespace dmlc

#endif//META_DATA_H
