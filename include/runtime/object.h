//
// Created by richard on 7/3/24.
//

#ifndef LITETVM_RUNTIME_OBJECT_H
#define LITETVM_RUNTIME_OBJECT_H

#include "ffi/cast.h"
#include "ffi/object.h"
// #include "runtime/utils.h"

#include <utility>

namespace litetvm {
namespace runtime {

using ffi::Object;
using ffi::ObjectPtr;
using ffi::ObjectPtrEqual;
using ffi::ObjectPtrHash;
using ffi::ObjectRef;

using ffi::Downcast;
using ffi::GetObjectPtr;
using ffi::GetRef;

/*!
 * \brief Namespace for the list of type index.
 * \note Use struct so that we have to use TypeIndex::ENumName to refer to
 *       the constant, but still able to use enum.
 */
enum TypeIndex : int32_t {
    // Standard static index assignments,
    // Frontends can take benefit of these constants.
    /*! \brief runtime::Module. */
    kRuntimeModule = kTVMFFIModule,
    /*! \brief runtime::NDArray. */
    kRuntimeNDArray = kTVMFFINDArray,
    /*! \brief runtime::Shape. */
    kRuntimeShape = kTVMFFIShape,
    // Extra builtin static index here
    // We reserve 16 extra static indices for custom types
    kCustomStaticIndex = kTVMFFIDynObjectBegin - 16,
    /*! \brief ffi::Function. */
    kRuntimePackedFunc = kCustomStaticIndex + 1,
    /*! \brief runtime::DRef for disco distributed runtime */
    kRuntimeDiscoDRef = kCustomStaticIndex + 2,
    /*! \brief runtime::RPCObjectRef */
    kRuntimeRPCObjectRef = kCustomStaticIndex + 3,
    // custom builtin
    kRuntimeString,
    kRuntimeMap,
    kRuntimeArray,
    // static assignments that may subject to change.
    kStaticIndexEnd,
};

static_assert(static_cast<int>(kCustomStaticIndex) >= static_cast<int>(kTVMFFIStaticObjectEnd),
              "Static slot overflows to custom indices");

/*
 * \brief Define the default copy/move constructor and assign operator
 * \param TypeName The class typename.
 */
#define TVM_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName) \
    TypeName(const TypeName& other) = default;            \
    TypeName(TypeName&& other) = default;                 \
    TypeName& operator=(const TypeName& other) = default; \
    TypeName& operator=(TypeName&& other) = default;


/*!
 * \brief Define CopyOnWrite function in an ObjectRef.
 * \param ObjectName The Type of the Node.
 *
 *  CopyOnWrite will generate a unique copy of the internal node.
 *  The node will be copied if it is referenced by multiple places.
 *  The function returns the raw pointer to the node to allow modification
 *  of the content.
 *
 * \code
 *
 *  MyCOWObjectRef ref, ref2;
 *  ref2 = ref;
 *  ref.CopyOnWrite()->value = new_value;
 *  assert(ref2->value == old_value);
 *  assert(ref->value == new_value);
 *
 * \endcode
 */
#define TVM_DEFINE_OBJECT_REF_COW_METHOD(ObjectName)                 \
    static_assert(ObjectName::_type_final,                           \
                  "TVM's CopyOnWrite may only be used for "          \
                  "Object types that are declared as final, "        \
                  "using the TVM_DECLARE_FINAL_OBJECT_INFO macro."); \
    ObjectName* CopyOnWrite() {                                      \
        CHECK(data_ != nullptr);                                     \
        if (!data_.unique()) {                                       \
            auto n = make_object<ObjectName>(*(operator->()));       \
            ObjectPtr<Object>(std::move(n)).swap(data_);             \
        }                                                            \
        return static_cast<ObjectName*>(data_.get());                \
    }

/*
 * \brief Define object reference methods.
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 */
#define TVM_DEFINE_OBJECT_REF_METHODS_WITHOUT_DEFAULT_CONSTRUCTOR(TypeName, ParentType, ObjectName) \
    explicit TypeName(::litetvm::ffi::ObjectPtr<::litetvm::ffi::Object> n) : ParentType(n) {}       \
    TVM_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                              \
    const ObjectName* operator->() const { return static_cast<const ObjectName*>(data_.get()); }    \
    const ObjectName* get() const { return operator->(); }                                          \
    using ContainerType = ObjectName;

#define TVM_DECLARE_BASE_OBJECT_INFO TVM_FFI_DECLARE_BASE_OBJECT_INFO
#define TVM_DECLARE_FINAL_OBJECT_INFO TVM_FFI_DECLARE_FINAL_OBJECT_INFO
#define TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS TVM_FFI_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS

#define TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS TVM_FFI_DEFINE_MUTABLE_OBJECT_REF_METHODS
#define TVM_DEFINE_OBJECT_REF_METHODS TVM_FFI_DEFINE_OBJECT_REF_METHODS
#define TVM_DEFINE_MUTABLE_NOTNULLABLE_OBJECT_REF_METHODS TVM_FFI_DEFINE_MUTABLE_NOTNULLABLE_OBJECT_REF_METHODS

#define TVM_STR_CONCAT_(__x, __y) __x##__y
#define TVM_STR_CONCAT(__x, __y) TVM_STR_CONCAT_(__x, __y)

// Object register type is now a nop
#define TVM_REGISTER_OBJECT_TYPE(x)

}// namespace runtime

using ffi::ObjectPtr;
using ffi::ObjectRef;
}// namespace litetvm


#endif//LITETVM_RUNTIME_OBJECT_H
