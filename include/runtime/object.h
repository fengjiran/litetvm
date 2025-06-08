//
// Created by richard on 7/3/24.
//

#ifndef LITETVM_RUNTIME_OBJECT_H
#define LITETVM_RUNTIME_OBJECT_H

#include "ffi/cast.h"
#include "ffi/object.h"
#include "runtime/utils.h"

#include <cstdint>
#include <dmlc/logging.h>
#include <format>
#include <functional>
#include <optional>
#include <string>

/*!
 * \brief Whether or not use atomic reference counter.
 *  If the reference counter is not atomic,
 *  an object cannot be owned by multiple threads.
 *  We can, however, move an object across threads
 */
// #ifndef TVM_OBJECT_ATOMIC_REF_COUNTER
// #define TVM_OBJECT_ATOMIC_REF_COUNTER 1
// #endif
//
// #if TVM_OBJECT_ATOMIC_REF_COUNTER
// #include <atomic>
// #endif// TVM_OBJECT_ATOMIC_REF_COUNTER

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

static_assert(static_cast<int>(kCustomStaticIndex) >= static_cast<int>(kTVMFFIStaticObjectEnd), "Static slot overflows to custom indices");

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
        ICHECK(data_ != nullptr);                                    \
        if (!data_.unique()) {                                       \
            auto n = make_object<ObjectName>(*(operator->()));       \
            ObjectPtr<Object>(std::move(n)).swap(data_);             \
        }                                                            \
        return static_cast<ObjectName*>(data_.get());                \
    }

/*!
 * \brief A custom smart pointer for Object.
 * \tparam T the content data type.
 * \sa make_object
 */
// template<typename T>
// class ObjectPtr {
// public:
//     ObjectPtr() = default;
//
//     ObjectPtr(std::nullptr_t) {}
//
//     ObjectPtr(const ObjectPtr& other) : ObjectPtr(other.data_) {}
//
//     /*!
//    * \brief copy constructor
//    * \param other The value to be moved
//    */
//     template<typename U,
//              typename = std::enable_if_t<std::is_base_of_v<T, U>>>
//     ObjectPtr(const ObjectPtr<U>& other) : ObjectPtr(other.data_) {}
//
//     /*!
//    * \brief move constructor
//    * \param other The value to be moved
//    */
//     ObjectPtr(ObjectPtr&& other) noexcept : data_(other.data_) {
//         other.data_ = nullptr;
//     }
//
//     /*!
//    * \brief move constructor
//    * \param other The value to be moved
//    */
//     template<typename U,
//              typename = std::enable_if_t<std::is_base_of_v<T, U>>>
//     ObjectPtr(ObjectPtr<U>&& other) noexcept : data_(other.data_) {
//         other.data_ = nullptr;
//     }
//
//     ~ObjectPtr() {
//         reset();
//     }
//
//     // NODISCARD const Object* get_raw_pointer() const {
//     //     return data_;
//     // }
//
//     // Get the content of the pointer
//     NODISCARD T* get() const {
//         return static_cast<T*>(data_);
//     }
//
//     T* operator->() const {
//         return get();
//     }
//
//     T& operator*() const {
//         return *get();
//     }
//
//     /*!
//    * \brief copy assignment
//    * \param other The value to be assigned.
//    * \return reference to self.
//    */
//     ObjectPtr& operator=(const ObjectPtr& other) {
//         ObjectPtr tmp(other);
//         tmp.swap(*this);
//         return *this;
//     }
//
//     /*!
//    * \brief move assignment
//    * \param other The value to be assigned.
//    * \return reference to self.
//    */
//     ObjectPtr& operator=(ObjectPtr&& other) noexcept {
//         ObjectPtr tmp(std::move(other));
//         tmp.swap(*this);
//         return *this;
//     }
//
//     /*!
//    * \brief nullptr check
//    * \return result of comparison of internal pointer with nullptr.
//    */
//     explicit operator bool() const {
//         return get() != nullptr;
//     }
//
//     /*! \brief reset the content of ptr to be nullptr */
//     void reset() {
//         if (data_) {
//             data_->DecRef();
//             data_ = nullptr;
//         }
//     }
//
//     /*!
//    * \brief Swap this array with another Object
//    * \param other The other Object
//    */
//     void swap(ObjectPtr& other) noexcept {
//         std::swap(data_, other.data_);
//     }
//
//     /*! \return The use count of the ptr, for debug purposes */
//     NODISCARD int use_count() const { return data_ != nullptr ? data_->use_count() : 0; }
//     /*! \return whether the reference is unique */
//     NODISCARD bool unique() const { return data_ != nullptr && data_->use_count() == 1; }
//     /*! \return Whether two ObjectPtr do not equal each other */
//     bool operator==(const ObjectPtr& other) const { return data_ == other.data_; }
//     /*! \return Whether two ObjectPtr equals each other */
//     bool operator!=(const ObjectPtr& other) const { return data_ != other.data_; }
//     /*! \return Whether the pointer is nullptr */
//     bool operator==(std::nullptr_t) const { return data_ == nullptr; }
//     /*! \return Whether the pointer is not nullptr */
//     bool operator!=(std::nullptr_t) const { return data_ != nullptr; }
//
// private:
//     Object* data_{nullptr};
//
//     explicit ObjectPtr(Object* data) : data_(data) {
//         if (data) {
//             data_->IncRef();
//         }
//     }
//
//     /*!
//    * \brief Move an ObjectPtr from an RValueRef argument.
//    * \param ref The rvalue reference.
//    * \return the moved result.
//    */
//     static ObjectPtr MoveFromRValueRefArg(Object** ref) {
//         ObjectPtr ptr(*ref);
//         // ptr.data_ = *ref;
//         *ref = nullptr;
//         return ptr;
//     }
//
//     friend class Object;
//     friend class ObjectRef;
//     friend class TVMPODValue_;
//     friend class TVMRetValue;
//     friend class TVMArgValue;
//     friend class TVMMovableArgValue_;
//
//     template<typename>
//     friend class ObjectPtr;
//     friend class TVMArgsSetter;
//
//     template<typename>
//     friend class BaseAllocator;
//
//     template<typename RefType, typename ObjType>
//     friend RefType GetRef(const ObjType* ptr);
//
//     template<typename BaseType, typename ObjType>
//     friend ObjectPtr<BaseType> GetObjectPtr(ObjType* ptr);
// };

/*! \brief Base class of all object reference */
// class ObjectRef {
// public:
//     /*! \brief type indicate the container type. */
//     using ContainerType = Object;
//
//     /*! \brief default constructor */
//     ObjectRef() = default;
//
//     /*! \brief Constructor from existing object ptr */
//     explicit ObjectRef(ObjectPtr<Object> data) : data_(std::move(data)) {}
//
//     /*!
//    * \brief Comparator
//    * \param other Another object ref.
//    * \return the compare result.
//    */
//     NODISCARD bool same_as(const ObjectRef& other) const {
//         return data_ == other.data_;
//     }
//
//     /*!
//    * \brief Comparator
//    * \param other Another object ref.
//    * \return the compare result.
//    */
//     bool operator==(const ObjectRef& other) const {
//         return data_ == other.data_;
//     }
//     /*!
//      * \brief Comparator
//      * \param other Another object ref.
//      * \return the compare result.
//      */
//     bool operator!=(const ObjectRef& other) const {
//         return data_ != other.data_;
//     }
//     /*!
//      * \brief Comparator
//      * \param other Another object ref by address.
//      * \return the compare result.
//      */
//     bool operator<(const ObjectRef& other) const {
//         return data_.get() < other.data_.get();
//     }
//
//     /*!
//    * \return whether the object is defined(not null).
//    */
//     NODISCARD bool defined() const { return data_ != nullptr; }
//     /*! \return the internal object pointer */
//     NODISCARD const Object* get() const { return data_.get(); }
//     /*! \return the internal object pointer */
//     const Object* operator->() const { return get(); }
//     /*! \return whether the reference is unique */
//     NODISCARD bool unique() const { return data_.unique(); }
//     /*! \return The use count of the ptr, for debug purposes */
//     NODISCARD int use_count() const { return data_.use_count(); }
//
//     /*!
//    * \brief Try to downcast the internal Object to a
//    *  raw pointer of a corresponding type.
//    *
//    *  The function will return a nullptr if the cast failed.
//    *
//    *      if (const AddNode *ptr = node_ref.as<AddNode>()) {
//    *        // This is an add node
//    *      }
//    *
//    * \tparam ObjectType the target type must be a subtype of Object
//    */
//     template<typename ObjectType,
//              typename = std::enable_if_t<std::is_base_of_v<Object, ObjectType>>>
//     const ObjectType* as() const {
//         if (data_ != nullptr && data_->IsInstance<ObjectType>()) {
//             return static_cast<ObjectType*>(data_.get());
//         }
//         return nullptr;
//     }
//
//     /*!
//    * \brief Try to downcast the ObjectRef to a
//    *    Optional<T> of the requested type.
//    *
//    *  The function will return a NullOpt if the cast failed.
//    *
//    *      if (Optional<Add> opt = node_ref.as<Add>()) {
//    *        // This is an add node
//    *      }
//    *
//    * \note While this method is declared in <tvm/runtime/object.h>,
//    * the implementation is in <tvm/runtime/container/optional.h> to
//    * prevent circular includes.  This additional include file is only
//    * required in compilation units that uses this method.
//    *
//    * \tparam ObjectRefType the target type, must be a subtype of ObjectRef
//    */
//     template<typename ObjectRefType,
//              typename = std::enable_if_t<std::is_base_of_v<ObjectRef, ObjectRefType>>>
//     std::optional<ObjectRefType> as() const {
//         if (auto* ptr = this->as<typename ObjectRefType::ContainerType>()) {
//             return GetRef<ObjectRefType>(ptr);
//         }
//         return std::nullopt;
//     }
//
//     // Default type properties for the reference class.
//     static constexpr bool _type_is_nullable = true;
//
// protected:
//     /*! \brief Internal pointer that backs the reference. */
//     ObjectPtr<Object> data_;
//
//     /*! \return return a mutable internal ptr, can be used by sub-classes. */
//     NODISCARD Object* get_mutable() const { return data_.get(); }
//
//     /*!
//    * \brief Internal helper function downcast a ref without check.
//    * \note Only used for internal dev purposes.
//    * \tparam T The target reference type.
//    * \return The casted result.
//    */
//     template<typename T>
//     static T DowncastNoCheck(ObjectRef ref) {
//         return T(std::move(ref.data_));
//     }
//
//     /*!
//    * \brief Clear the object ref data field without DecRef
//    *        after we successfully moved the field.
//    * \param ref The reference data.
//    */
//     static void FFIClearAfterMove(ObjectRef* ref) {
//         ref->data_.data_ = nullptr;
//     }
//
//     /*!
//    * \brief Internal helper function get data_ as ObjectPtr of ObjectType.
//    * \note only used for internal dev purpose.
//    * \tparam ObjectType The corresponding object type.
//    * \return the corresponding type.
//    */
//     template<typename ObjectType>
//     static ObjectPtr<ObjectType> GetDataPtr(const ObjectRef& ref) {
//         return ObjectPtr<ObjectType>(ref.data_.data_);
//     }
//
//     // friend classes and functions
//     friend struct ObjectPtrHash;
//     friend class TVMRetValue;
//     friend class TVMArgsSetter;
//     friend class ObjectInternal;
//
//     template<typename SubRef, typename BaseRef>
//     friend SubRef Downcast(BaseRef ref);
// };


/*! \brief ObjectRef hash functor */
// struct ObjectPtrHash {
//     template<typename T>
//     size_t operator()(const ObjectPtr<T>& ptr) const {
//         return std::hash<Object*>()(ptr.get());
//     }
//
//     size_t operator()(const ObjectRef& ref) const {
//         return operator()(ref.data_);
//     }
// };

/*! \brief ObjectRef equal functor */
// struct ObjectPtrEqual {
//     template<typename T>
//     bool operator()(const ObjectPtr<T>& a, const ObjectPtr<T>& b) const {
//         return a == b;
//     }
//
//     bool operator()(const ObjectRef& a, const ObjectRef& b) const {
//         return a.same_as(b);
//     }
// };

// template<typename TargetType>
// bool Object::IsInstance() const {
//     const Object* self = this;
//
//     // Everything is a subclass of object.
//     if (std::is_same_v<TargetType, Object>) {
//         return true;
//     }
//
//     if (TargetType::_type_final) {
//         // if the target type is a final type
//         // then we only need to check the equivalence.
//         return self->type_index_ == TargetType::RuntimeTypeIndex();
//     }
//
//     // if the target type is a non-leaf type
//     // Check if type index falls into the range of reserved slots.
//     uint32_t begin = TargetType::RuntimeTypeIndex();
//
//     if (TargetType::_type_child_slots != 0) {
//         uint32_t end = begin + TargetType::_type_child_slots;
//         if (self->type_index_ >= begin && self->type_index_ < end) {
//             return true;
//         }
//     } else {
//         if (self->type_index_ == begin) {
//             return true;
//         }
//     }
//
//     if (!TargetType::_type_child_slots_can_overflow) {
//         return false;
//     }
//
//     if (self->type_index_ < TargetType::RuntimeTypeIndex()) {
//         return false;
//     }
//
//     return self->DerivedFrom(TargetType::RuntimeTypeIndex());
// }
//
// template<typename RefType,
//          typename ObjType>
// RefType GetRef(const ObjType* ptr) {
//     static_assert(std::is_base_of_v<typename RefType::ContainerType, ObjType>, "Can only cast to the ref of same container type");
//     if (!RefType::_type_is_nullable) {
//         CHECK(ptr != nullptr);
//     }
//     return RefType(ObjectPtr<Object>(const_cast<Object*>(static_cast<const Object*>(ptr))));
// }

// template<typename BaseType,
//          typename ObjType>
// ObjectPtr<BaseType> GetObjectPtr(ObjType* ptr) {
//     static_assert(std::is_base_of_v<BaseType, ObjType>, "Can only cast to the ref of same container type");
//     return ObjectPtr<BaseType>(static_cast<Object*>(ptr));
// }
//
// template<typename SubRef, typename BaseRef>
// SubRef Downcast(BaseRef ref) {
//     if (ref.defined()) {
//         CHECK(ref->template IsInstance<typename SubRef::ContainerType>())
//                 << "Downcast from " << ref->GetTypeKey() << " to " << SubRef::ContainerType::_type_key << " failed.";
//     } else {
//         CHECK(SubRef::_type_is_nullable) << "Downcast from nullptr to not nullable reference of " << SubRef::ContainerType::_type_key;
//     }
//     return SubRef(std::move(ref.data_));
// }

/*!
 * \brief helper macro to declare a base object type that can be inherited.
 * \param TypeName The name of the current type.
 * \param ParentType The name of the ParentType
 */
#define TVM_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType)                                             \
    static_assert(!ParentType::_type_final, "ParentObj marked as final");                              \
                                                                                                       \
    static uint32_t RuntimeTypeIndex() {                                                               \
        static_assert(TypeName::_type_child_slots == 0 || ParentType::_type_child_slots == 0 ||        \
                              TypeName::_type_child_slots < ParentType::_type_child_slots,             \
                      "Need to set _type_child_slots when parent specifies it.");                      \
        if (TypeName::_type_index != static_cast<uint32_t>(runtime::TypeIndex::kDynamic)) {            \
            return TypeName::_type_index;                                                              \
        }                                                                                              \
        return _GetOrAllocRuntimeTypeIndex();                                                          \
    }                                                                                                  \
                                                                                                       \
    static uint32_t _GetOrAllocRuntimeTypeIndex() {                                                    \
        static uint32_t tindex = Object::GetOrAllocRuntimeTypeIndex(                                   \
                TypeName::_type_key, TypeName::_type_index, ParentType::_GetOrAllocRuntimeTypeIndex(), \
                TypeName::_type_child_slots, TypeName::_type_child_slots_can_overflow);                \
        return tindex;                                                                                 \
    }

/*!
 * \brief helper macro to declare type information in a final class.
 * \param TypeName The name of the current type.
 * \param ParentType The name of the ParentType
 */
#define TVM_DECLARE_FINAL_OBJECT_INFO(TypeName, ParentType) \
    static const constexpr bool _type_final = true;         \
    static const constexpr int _type_child_slots = 0;       \
    TVM_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType)


/*! \brief helper macro to suppress unused warning */
#if defined(__GNUC__)
#define TVM_ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define TVM_ATTRIBUTE_UNUSED
#endif

#define TVM_STR_CONCAT_(__x, __y) __x##__y
#define TVM_STR_CONCAT(__x, __y) TVM_STR_CONCAT_(__x, __y)

#define TVM_OBJECT_REG_VAR_DEF static TVM_ATTRIBUTE_UNUSED uint32_t __make_Object_tid

/*!
 * \brief Helper macro to register the object type to runtime.
 *  Makes sure that the runtime type table is correctly populated.
 *
 *  Use this macro in the cc file for each terminal class.
 */
#define TVM_REGISTER_OBJECT_TYPE(TypeName) \
    TVM_STR_CONCAT(TVM_OBJECT_REG_VAR_DEF, __COUNTER__) = TypeName::_GetOrAllocRuntimeTypeIndex()

// Object register type is now a nop
#define TVM_REGISTER_OBJECT_TYPE(x)

/*
 * \brief Define the default copy/move constructor and assign operator
 * \param TypeName The class typename.
 */
#define TVM_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName) \
    TypeName(const TypeName& other) = default;            \
    TypeName(TypeName&& other) = default;                 \
    TypeName& operator=(const TypeName& other) = default; \
    TypeName& operator=(TypeName&& other) = default;


/*
 * \brief Define object reference methods.
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 */
#define TVM_DEFINE_OBJECT_REF_METHODS_WITHOUT_DEFAULT_CONSTRUCTOR(TypeName, ParentType, ObjectName) \
    explicit TypeName(runtime::ObjectPtr<runtime::Object> n) : ParentType(n) {}                     \
    TVM_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                              \
    const ObjectName* operator->() const { return static_cast<const ObjectName*>(data_.get()); }    \
    const ObjectName* get() const { return operator->(); }                                          \
    using ContainerType = ObjectName;


/*
 * \brief Define object reference methods.
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 */
#define TVM_DEFINE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName) \
    TypeName() = default;                                               \
    TVM_DEFINE_OBJECT_REF_METHODS_WITHOUT_DEFAULT_CONSTRUCTOR(TypeName, ParentType, ObjectName)


/*
 * \brief Define object reference methods that is not nullable.
 *
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 */
#define TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)              \
    explicit TypeName(runtime::ObjectPtr<runtime::Object> n) : ParentType(n) {}                  \
    TVM_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                           \
    const ObjectName* operator->() const { return static_cast<const ObjectName*>(data_.get()); } \
    const ObjectName* get() const { return operator->(); }                                       \
    static constexpr bool _type_is_nullable = false;                                             \
    using ContainerType = ObjectName;


/*
 * \brief Define object reference methods of whose content is mutable.
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 * \note We recommend making objects immutable when possible.
 *       This macro is only reserved for objects that stores runtime states.
 */
#define TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)      \
    TypeName() = default;                                                            \
    TVM_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                               \
    explicit TypeName(runtime::ObjectPtr<runtime::Object> n) : ParentType(n) {}      \
    ObjectName* operator->() const { return static_cast<ObjectName*>(data_.get()); } \
    using ContainerType = ObjectName;


/*
 * \brief Define object reference methods that is both not nullable and mutable.
 *
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 */
#define TVM_DEFINE_MUTABLE_NOTNULLABLE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName) \
    explicit TypeName(runtime::ObjectPtr<runtime::Object> n) : ParentType(n) {}             \
    TVM_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                      \
    ObjectName* operator->() const { return static_cast<ObjectName*>(data_.get()); }        \
    ObjectName* get() const { return operator->(); }                                        \
    static constexpr bool _type_is_nullable = false;                                        \
    using ContainerType = ObjectName;


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

}// namespace runtime

using ffi::ObjectPtr;
using ffi::ObjectRef;
}// namespace litetvm


#endif//LITETVM_RUNTIME_OBJECT_H
