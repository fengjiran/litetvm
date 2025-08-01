//
// Created by 赵丹 on 25-5-15.
//

#ifndef LITETVM_FFI_OBJECT_H
#define LITETVM_FFI_OBJECT_H

#include "ffi/base_details.h"
#include "ffi/c_api.h"

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace litetvm {
namespace ffi {

using TypeIndex = TVMFFITypeIndex;
using TypeInfo = TVMFFITypeInfo;

/*!
 * \brief Known type keys for pre-defined types.
 */
struct StaticTypeKey {
    static constexpr const char* kTVMFFIAny = "Any";
    static constexpr const char* kTVMFFINone = "None";
    static constexpr const char* kTVMFFIBool = "bool";
    static constexpr const char* kTVMFFIInt = "int";
    static constexpr const char* kTVMFFIFloat = "float";
    static constexpr const char* kTVMFFIOpaquePtr = "void*";
    static constexpr const char* kTVMFFIDataType = "DataType";
    static constexpr const char* kTVMFFIDevice = "Device";
    static constexpr const char* kTVMFFIRawStr = "const char*";
    static constexpr const char* kTVMFFIByteArrayPtr = "TVMFFIByteArray*";
    static constexpr const char* kTVMFFIObjectRValueRef = "ObjectRValueRef";
    static constexpr const char* kTVMFFIBytes = "ffi.Bytes";
    static constexpr const char* kTVMFFIStr = "ffi.String";
    static constexpr const char* kTVMFFIShape = "ffi.Shape";
    static constexpr const char* kTVMFFINDArray = "ffi.NDArray";
    static constexpr const char* kTVMFFIObject = "ffi.Object";
    static constexpr const char* kTVMFFIFunction = "ffi.Function";
    static constexpr const char* kTVMFFIArray = "ffi.Array";
    static constexpr const char* kTVMFFIMap = "ffi.Map";
};

/*!
 * \brief Get type key from type index
 * \param type_index The input type index
 * \return the type key
 */
inline std::string TypeIndexToTypeKey(int32_t type_index) {
    const TypeInfo* type_info = TVMFFIGetTypeInfo(type_index);
    return {type_info->type_key.data, type_info->type_key.size};
}

namespace details {
// Helper to perform
// unsafe operations related to object
struct ObjectUnsafe;

/*!
 * Check if the type_index is an instance of TargetObjectType.
 *
 * \tparam TargetType The target object type to be checked.
 *
 * \param object_type_index The type index to be checked, caller
 *        ensures that the index is already within the object index range.
 *
 * \return Whether the target type is true.
 */
template<typename TargetType>
TVM_FFI_INLINE bool IsObjectInstance(int32_t object_type_index);
}// namespace details

/*!
 * \brief base class of all object containers.
 *
 * Subclass of objects should declare the following static constexpr fields:
 *
 * - _type_index:
 *      Static type index of the object, if assigned to TypeIndex::kTVMFFIDynObject
 *      the type index will be assigned during runtime.
 *      Runtime type index can be accessed by ObjectType::TypeIndex();
 * - _type_key:
 *       The unique string identifier of the type.
 * - _type_final:
 *       Whether the type is terminal type(there is no subclass of the type in the object system).
 *       This field is automatically set by macro TVM_DECLARE_FINAL_OBJECT_INFO
 *       It is still OK to subclass a terminal object type T and construct it using make_object.
 *       But IsInstance check will only show that the object type is T(instead of the subclass).
 * - _type_mutable:
 *      Whether we would like to expose cast to non-constant pointer
 *      ObjectType* from Any/AnyView. By default, we set to false so it is not exposed.
 *
 *
 * The following two fields are necessary for base classes that can be subclassed.
 *
 * - _type_child_slots:
 *       Number of reserved type index slots for child classes.
 *       Used for runtime optimization for type checking in IsInstance.
 *       If an object's type_index is within range of [type_index, type_index + _type_child_slots]
 *       Then the object can be quickly decided as sub-class of the current object class.
 *       If not, a fallback mechanism is used to check the global type table.
 *       Recommendation: set to estimate number of children needed.
 *
 * - _type_child_slots_can_overflow:
 *       Whether we can add additional child classes even if the number of child classes
 *       exceeds the _type_child_slots. A fallback mechanism to check type table will be used.
 *       Recommendation: set to false for optimal runtime speed if we know exact number of children.
 *
 * Two macros are used to declare helper functions in the object:
 * - Use TVM_FFI_DECLARE_BASE_OBJECT_INFO for object classes that can be sub-classed.
 * - Use TVM_FFI_DECLARE_FINAL_OBJECT_INFO for object classes that cannot be sub-classed.
 *
 * New objects can be created using make_object function.
 * Which will automatically populate the type_index and deleter of the object.
 */
class Object {
protected:
    /*! \brief header field that is the common prefix of all objects */
    TVMFFIObject header_;

public:
    Object() {
        header_.ref_counter = 0;
        header_.deleter = nullptr;
    }
    /*!
   * Check if the object is an instance of TargetType.
   * \tparam TargetType The target type to be checked.
   * \return Whether the target type is true.
   */
    template<typename TargetType>
    NODISCARD bool IsInstance() const {
        return details::IsObjectInstance<TargetType>(header_.type_index);
    }

    /*! \return The internal runtime type index of the object. */
    NODISCARD int32_t type_index() const {
        return header_.type_index;
    }

    /*!
   * \return the type key of the object.
   * \note this operation is expensive, can be used for error reporting.
   */
    NODISCARD std::string GetTypeKey() const {
        // the function checks that the info exists
        const TypeInfo* type_info = TVMFFIGetTypeInfo(header_.type_index);
        return {type_info->type_key.data, type_info->type_key.size};
    }

    /*!
   * \return A hash value of the return of GetTypeKey.
   */
    NODISCARD uint64_t GetTypeKeyHash() const {
        // the function checks that the info exists
        const TypeInfo* type_info = TVMFFIGetTypeInfo(header_.type_index);
        return type_info->type_key_hash;
    }

    /*!
   * \brief Get the type key of the corresponding index from runtime.
   * \param tindex The type index.
   * \return the result.
   */
    static std::string TypeIndex2Key(int32_t tindex) {
        const TypeInfo* type_info = TVMFFIGetTypeInfo(tindex);
        return {type_info->type_key.data, type_info->type_key.size};
    }

    NODISCARD bool unique() const {
        return use_count() == 1;
    }

    /*!
   * \return The usage count of the cell.
   * \note We use stl style naming to be consistent with known API in shared_ptr.
   */
    NODISCARD int32_t use_count() const {
        // only need relaxed load of counters
#ifdef _MSC_VER
        return (reinterpret_cast<const volatile long*>(&header_.ref_counter))[0];// NOLINT(*)
#else
        return __atomic_load_n(&header_.ref_counter, __ATOMIC_RELAXED);
#endif
    }

    // Information about the object
    static constexpr const char* _type_key = "object.Object";

    // Default object type properties for sub-classes
    static constexpr bool _type_final = false;
    static constexpr bool _type_mutable = false;
    static constexpr uint32_t _type_child_slots = 0;
    static constexpr bool _type_child_slots_can_overflow = true;
    // NOTE: static type index field of the class
    static constexpr int32_t _type_index = kTVMFFIObject;
    // the static type depth of the class
    static constexpr int32_t _type_depth = 0;
    // the structural equality and hash kind of the type
    static constexpr TVMFFISEqHashKind _type_s_eq_hash_kind = kTVMFFISEqHashKindUnsupported;

    // The following functions are provided by macro
    // TVM_FFI_DECLARE_BASE_OBJECT_INFO and TVM_DECLARE_FINAL_OBJECT_INFO
    /*!
   * \brief Get the runtime allocated type index of the type
   * \note Getting this information may need dynamic calls into a global table.
   */
    static int32_t RuntimeTypeIndex() {
        return kTVMFFIObject;
    }

    /*!
   * \brief Internal function to get or allocate a runtime index.
   */
    static int32_t _GetOrAllocRuntimeTypeIndex() {
        return kTVMFFIObject;
    }

private:
    /*! \brief increase reference count */
    void IncRef() {
#ifdef _MSC_VER
        _InterlockedIncrement(reinterpret_cast<volatile long*>(&header_.ref_counter));// NOLINT(*)
#else
        __atomic_fetch_add(&header_.ref_counter, 1, __ATOMIC_RELAXED);
#endif
    }

    /*! \brief decrease reference count and delete the object */
    void DecRef() {
#ifdef _MSC_VER
        if (_InterlockedDecrement(                                                 //
                    reinterpret_cast<volatile long*>(&header_.ref_counter)) == 0) {// NOLINT(*)
            // full barrrier is implicit in InterlockedDecrement
            if (header_.deleter != nullptr) {
                header_.deleter(&(this->header_));
            }
        }
#else
        // first do a release, note we only need to acquire for deleter
        if (__atomic_fetch_sub(&(header_.ref_counter), 1, __ATOMIC_RELEASE) == 1) {
            // only acquire when we need to call deleter
            // in this case we need to ensure all previous writes are visible
            __atomic_thread_fence(__ATOMIC_ACQUIRE);
            if (header_.deleter != nullptr) {
                header_.deleter(&this->header_);
            }
        }
#endif
    }

    // friend classes
    template<typename>
    friend class ObjectPtr;
    friend struct details::ObjectUnsafe;
};

/*!
 * \brief A custom smart pointer for Object.
 * \tparam T the content data type.
 * \sa make_object
 */
template<typename T>
class ObjectPtr {
public:
    /*! \brief default constructor */
    ObjectPtr() = default;

    /*! \brief default constructor */
    ObjectPtr(std::nullptr_t) {}// NOLINT(*)

    /*!
   * \brief copy constructor
   * \param other The value to be moved
   */
    ObjectPtr(const ObjectPtr& other) : ObjectPtr(other.data_) {}

    /*!
   * \brief copy constructor
   * \param other The value to be moved
   */
    template<typename U>
    ObjectPtr(const ObjectPtr<U>& other) : ObjectPtr(other.data_) {// NOLINT(*)
        static_assert(std::is_base_of_v<T, U>, "can only assign of child class ObjectPtr to parent");
    }

    /*!
   * \brief move constructor
   * \param other The value to be moved
   */
    ObjectPtr(ObjectPtr&& other) noexcept : data_(other.data_) {
        other.data_ = nullptr;
    }

    /*!
   * \brief move constructor
   * \param other The value to be moved
   */
    template<typename Y>
    ObjectPtr(ObjectPtr<Y>&& other) : data_(other.data_) {// NOLINT(*)
        static_assert(std::is_base_of_v<T, Y>, "can only assign of child class ObjectPtr to parent");
        other.data_ = nullptr;
    }

    /*! \brief destructor */
    ~ObjectPtr() {
        this->reset();
    }

    /*!
   * \brief Swap this array with another Object
   * \param other The other Object
   */
    void swap(ObjectPtr& other) noexcept {// NOLINT(*)
        std::swap(data_, other.data_);
    }

    /*!
   * \return Get the content of the pointer
   */
    NODISCARD T* get() const {
        return static_cast<T*>(data_);
    }

    /*!
   * \return The pointer
   */
    T* operator->() const {
        return get();
    }

    /*!
   * \return The reference
   */
    T& operator*() const {// NOLINT(*)
        return *get();
    }
    /*!
   * \brief copy assignment
   * \param other The value to be assigned.
   * \return reference to self.
   */
    ObjectPtr& operator=(const ObjectPtr& other) {// NOLINT(*)
        // takes in plane operator to enable copy elison.
        // copy-and-swap idiom
        ObjectPtr(other).swap(*this);// NOLINT(*)
        return *this;
    }

    /*!
   * \brief move assignment
   * \param other The value to be assigned.
   * \return reference to self.
   */
    ObjectPtr& operator=(ObjectPtr&& other) noexcept {// NOLINT(*)
        // copy-and-swap idiom
        ObjectPtr(std::move(other)).swap(*this);// NOLINT(*)
        return *this;
    }

    /*!
   * \brief nullptr check
   * \return result of comparison of internal pointer with nullptr.
   */
    explicit operator bool() const {
        return get() != nullptr;
    }

    /*! \brief reset the content of ptr to be nullptr */
    void reset() {
        if (data_ != nullptr) {
            data_->DecRef();
            data_ = nullptr;
        }
    }

    /*! \return The use count of the ptr, for debug purposes */
    NODISCARD int use_count() const {
        return data_ != nullptr ? data_->use_count() : 0;
    }

    /*! \return whether the reference is unique */
    NODISCARD bool unique() const {
        return data_ != nullptr && data_->use_count() == 1;
    }

    /*! \return Whether two ObjectPtr do not equal each other */
    bool operator==(const ObjectPtr& other) const {
        return data_ == other.data_;
    }

    /*! \return Whether two ObjectPtr equals each other */
    bool operator!=(const ObjectPtr& other) const {
        return data_ != other.data_;
    }

    /*! \return Whether the pointer is nullptr */
    bool operator==(std::nullptr_t) const {
        return data_ == nullptr;
    }

    /*! \return Whether the pointer is not nullptr */
    bool operator!=(std::nullptr_t) const {
        return data_ != nullptr;
    }

private:
    /*! \brief internal pointer field */
    Object* data_{nullptr};

    /*!
   * \brief constructor from Object
   * \param data The data pointer
   */
    explicit ObjectPtr(Object* data) : data_(data) {
        if (data_ != nullptr) {
            data_->IncRef();
        }
    }

    // friend classes
    friend class Object;
    friend class ObjectRef;
    friend struct ObjectPtrHash;
    template<typename>
    friend class ObjectPtr;
    friend struct details::ObjectUnsafe;
};

/*!
 * \brief Optional data type in FFI.
 * \tparam T The underlying type of the optional.
 *
 * \note Compared to std::optional, Optional<ObjectRef>
 *   akes less storage as it used nullptr to represent nullopt.
 */
template<typename T, typename = void>
class Optional;

/*! \brief Base class of all object reference */
class ObjectRef {
public:
    /*! \brief default constructor */
    ObjectRef() = default;
    /*! \brief copy constructor */
    ObjectRef(const ObjectRef& other) = default;
    /*! \brief move constructor */
    ObjectRef(ObjectRef&& other) = default;
    /*! \brief copy assignment */
    ObjectRef& operator=(const ObjectRef& other) = default;
    /*! \brief move assignment */
    ObjectRef& operator=(ObjectRef&& other) = default;
    /*! \brief Constructor from existing object ptr */
    explicit ObjectRef(ObjectPtr<Object> data) : data_(std::move(data)) {}

    /*!
   * \brief Comparator
   * \param other Another object ref.
   * \return the compare result.
   */
    NODISCARD bool same_as(const ObjectRef& other) const {
        return data_ == other.data_;
    }

    /*!
   * \brief Comparator
   * \param other Another object ref.
   * \return the compare result.
   */
    bool operator==(const ObjectRef& other) const {
        return data_ == other.data_;
    }

    /*!
   * \brief Comparator
   * \param other Another object ref.
   * \return the compare result.
   */
    bool operator!=(const ObjectRef& other) const {
        return data_ != other.data_;
    }

    /*!
   * \brief Comparator
   * \param other Another object ref by address.
   * \return the compare result.
   */
    bool operator<(const ObjectRef& other) const {
        return data_.get() < other.data_.get();
    }

    /*!
   * \return whether the object is defined.
   */
    NODISCARD bool defined() const {
        return data_ != nullptr;
    }

    /*! \return the internal object pointer */
    NODISCARD const Object* get() const {
        return data_.get();
    }

    /*! \return the internal object pointer */
    const Object* operator->() const {
        return get();
    }

    /*! \return whether the reference is unique */
    NODISCARD bool unique() const {
        return data_.unique();
    }

    /*! \return The use count of the ptr, for debug purposes */
    NODISCARD int use_count() const {
        return data_.use_count();
    }

    /*!
   * \brief Try to downcast the internal Object to a
   *  raw pointer of a corresponding type.
   *
   *  The function will return a nullptr if the cast failed.
   *
   *      if (const AddNode *ptr = node_ref.as<AddNode>()) {
   *        // This is an add node
   *      }
   *
   * \tparam ObjectType the target type, must be a subtype of Object
   * \return The pointer to the requested type.
   */
    template<typename ObjectType,
             typename = std::enable_if_t<std::is_base_of_v<Object, ObjectType>>>
    const ObjectType* as() const {
        if (data_ != nullptr && data_->IsInstance<ObjectType>()) {
            return static_cast<ObjectType*>(data_.get());
        }
        return nullptr;
    }

    /*!
   * \brief Try to downcast the ObjectRef to Optional<T> of the requested type.
   *
   *  The function will return a std::nullopt if the cast or if the pointer is nullptr.
   *
   * \tparam ObjectRefType the target type, must be a subtype of ObjectRef'
   * \return The optional value of the requested type.
   */
    template<typename ObjectRefType,
             typename = std::enable_if_t<std::is_base_of_v<ObjectRef, ObjectRefType>>>
    TVM_FFI_INLINE std::optional<ObjectRefType> as() const {
        if (data_ != nullptr) {
            if (data_->IsInstance<typename ObjectRefType::ContainerType>()) {
                return ObjectRefType(data_);
            }
            return std::nullopt;
        }
        return std::nullopt;
    }

    /*!
   * \brief Get the type index of the ObjectRef
   * \return The type index of the ObjectRef
   */
    NODISCARD int32_t type_index() const {
        return data_ != nullptr ? data_->type_index() : kTVMFFINone;
    }

    /*!
   * \brief Get the type key of the ObjectRef
   * \return The type key of the ObjectRef
   */
    NODISCARD std::string GetTypeKey() const {
        return data_ != nullptr ? data_->GetTypeKey() : StaticTypeKey::kTVMFFINone;
    }

    /*! \brief type indicate the container type. */
    using ContainerType = Object;
    // Default type properties for the reference class.
    static constexpr bool _type_is_nullable = true;

protected:
    /*! \brief Internal pointer that backs the reference. */
    ObjectPtr<Object> data_;

    /*! \return return a mutable internal ptr, can be used by subclasses. */
    NODISCARD Object* get_mutable() const {
        return data_.get();
    }

    // friend classes.
    friend struct ObjectPtrHash;
    friend struct details::ObjectUnsafe;
};

// forward delcare variant
template<typename... V>
class Variant;

/*! \brief ObjectRef hash functor */
struct ObjectPtrHash {
    size_t operator()(const ObjectRef& a) const {
        return operator()(a.data_);
    }

    template<typename T>
    size_t operator()(const ObjectPtr<T>& a) const {
        return std::hash<Object*>()(a.get());
    }

    template<typename... V>
    TVM_FFI_INLINE size_t operator()(const Variant<V...>& a) const;
};

/*! \brief ObjectRef equal functor */
struct ObjectPtrEqual {
    bool operator()(const ObjectRef& a, const ObjectRef& b) const {
        return a.same_as(b);
    }

    template<typename T>
    bool operator()(const ObjectPtr<T>& a, const ObjectPtr<T>& b) const {
        return a == b;
    }

    template<typename... V>
    TVM_FFI_INLINE bool operator()(const Variant<V...>& a, const Variant<V...>& b) const;
};

// If dynamic type is enabled, we still need to register the runtime type of parent
#define TVM_FFI_REGISTER_STATIC_TYPE_INFO(TypeName, ParentType)                                             \
    static constexpr int32_t _type_depth = ParentType::_type_depth + 1;                                     \
    static int32_t _GetOrAllocRuntimeTypeIndex() {                                                          \
        static_assert(!ParentType::_type_final, "ParentType marked as final");                              \
        static_assert(TypeName::_type_child_slots == 0 || ParentType::_type_child_slots == 0 ||             \
                              TypeName::_type_child_slots < ParentType::_type_child_slots,                  \
                      "Need to set _type_child_slots when parent specifies it.");                           \
        TVMFFIByteArray type_key{TypeName::_type_key, std::char_traits<char>::length(TypeName::_type_key)}; \
                                                                                                            \
        static int32_t tindex = TVMFFITypeGetOrAllocIndex(                                                  \
                &type_key, TypeName::_type_index, TypeName::_type_depth, TypeName::_type_child_slots,       \
                TypeName::_type_child_slots_can_overflow, ParentType::_GetOrAllocRuntimeTypeIndex());       \
        return tindex;                                                                                      \
    }                                                                                                       \
    static inline int32_t _register_type_index = _GetOrAllocRuntimeTypeIndex()

/*!
 * \brief Helper macro to declare a object that comes with static type index.
 * \param TypeName The name of the current type.
 * \param ParentType The name of the ParentType
 */
#define TVM_FFI_DECLARE_STATIC_OBJECT_INFO(TypeName, ParentType)        \
    static int32_t RuntimeTypeIndex() { return TypeName::_type_index; } \
    TVM_FFI_REGISTER_STATIC_TYPE_INFO(TypeName, ParentType)

/*!
 * \brief helper macro to declare a base object type that can be inherited.
 * \param TypeName The name of the current type.
 * \param ParentType The name of the ParentType
 */
#define TVM_FFI_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType)                                              \
    static constexpr int32_t _type_depth = ParentType::_type_depth + 1;                                     \
    static int32_t _GetOrAllocRuntimeTypeIndex() {                                                          \
        static_assert(!ParentType::_type_final, "ParentType marked as final");                              \
        static_assert(TypeName::_type_child_slots == 0 || ParentType::_type_child_slots == 0 ||             \
                              TypeName::_type_child_slots < ParentType::_type_child_slots,                  \
                      "Need to set _type_child_slots when parent specifies it.");                           \
        TVMFFIByteArray type_key{TypeName::_type_key, std::char_traits<char>::length(TypeName::_type_key)}; \
        static int32_t tindex = TVMFFITypeGetOrAllocIndex(                                                  \
                &type_key, -1, TypeName::_type_depth, TypeName::_type_child_slots,                          \
                TypeName::_type_child_slots_can_overflow, ParentType::_GetOrAllocRuntimeTypeIndex());       \
        return tindex;                                                                                      \
    }                                                                                                       \
    static int32_t RuntimeTypeIndex() { return _GetOrAllocRuntimeTypeIndex(); }                             \
    static inline int32_t _type_index = _GetOrAllocRuntimeTypeIndex()

/*!
 * \brief helper macro to declare type information in a final class.
 * \param TypeName The name of the current type.
 * \param ParentType The name of the ParentType
 */
#define TVM_FFI_DECLARE_FINAL_OBJECT_INFO(TypeName, ParentType) \
    static constexpr int _type_child_slots = 0;                 \
    static constexpr bool _type_final = true;                   \
    TVM_FFI_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType)

/*
 * \brief Define object reference methods.
 *
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 *
 * \note This macro also defines the default constructor that puts the ObjectRef
 *       in undefined state initially.
 */
#define TVM_FFI_DEFINE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)                              \
    TypeName() = default;                                                                                \
    explicit TypeName(::litetvm::ffi::ObjectPtr<::litetvm::ffi::Object> n) : ParentType(std::move(n)) {} \
    TVM_FFI_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName)                                                \
    const ObjectName* operator->() const { return static_cast<const ObjectName*>(data_.get()); }         \
    const ObjectName* get() const { return operator->(); }                                               \
    using ContainerType = ObjectName

/*
 * \brief Define object reference methods do not have undefined state.
 *
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 */
#define TVM_FFI_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)                  \
    explicit TypeName(::litetvm::ffi::ObjectPtr<::litetvm::ffi::Object> n) : ParentType(std::move(n)) {} \
    TVM_FFI_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName)                                                \
    const ObjectName* operator->() const { return static_cast<const ObjectName*>(data_.get()); }         \
    const ObjectName* get() const { return operator->(); }                                               \
    static constexpr bool _type_is_nullable = false;                                                     \
    using ContainerType = ObjectName

/*
 * \brief Define object reference methods of whose content is mutable.
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 * \note We recommend making objects immutable when possible.
 *       This macro is only reserved for objects that stores runtime states.
 */
#define TVM_FFI_DEFINE_MUTABLE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)                              \
    TypeName() = default;                                                                                        \
    TVM_FFI_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                                       \
    explicit TypeName(::litetvm::runtime::ObjectPtr<::litetvm::runtime::Object> n) : ParentType(std::move(n)) {} \
    ObjectName* operator->() const { return static_cast<ObjectName*>(data_.get()); }                             \
    using ContainerType = ObjectName;

/*
 * \brief Define object reference methods that is both not nullable and mutable.
 *
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectref
 * \param ObjectName The type name of the object.
 */
#define TVM_FFI_DEFINE_MUTABLE_NOTNULLABLE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)          \
    explicit TypeName(::litetvm::ffi::ObjectPtr<::litetvm::ffi::Object> n) : ParentType(std::move(n)) {} \
    TVM_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                                   \
    ObjectName* operator->() const { return static_cast<ObjectName*>(data_.get()); }                     \
    ObjectName* get() const { return operator->(); }                                                     \
    static constexpr bool _type_is_nullable = false;                                                     \
    using ContainerType = ObjectName;

namespace details {
template<typename TargetType>
TVM_FFI_INLINE bool IsObjectInstance(int32_t object_type_index) {
    static_assert(std::is_base_of_v<Object, TargetType>);
    // Everything is a subclass of object.
    if constexpr (std::is_same_v<TargetType, Object>) {
        return true;
    } else if constexpr (TargetType::_type_final) {
        // if the target type is a final type
        // then we only need to check the equivalence.
        return object_type_index == TargetType::RuntimeTypeIndex();
    } else {
        // Explicitly enclose in else to eliminate this branch early in compilation.
        // if target type is a non-leaf type
        // Check if type index falls into the range of reserved slots.
        int32_t target_type_index = TargetType::RuntimeTypeIndex();
        int32_t begin = target_type_index;
        // The condition will be optimized by constant-folding.
        if constexpr (TargetType::_type_child_slots != 0) {
            // total_slots = child_slots + 1 (including self)
            int32_t end = begin + TargetType::_type_child_slots + 1;
            if (object_type_index >= begin && object_type_index < end) return true;
        } else {
            if (object_type_index == begin) return true;
        }
        if constexpr (TargetType::_type_child_slots_can_overflow) {
            // Invariance: parent index is always smaller than the child.
            if (object_type_index < target_type_index) return false;
            // Do a runtime lookup of type information
            // the function checks that the info exists
            const TypeInfo* type_info = TVMFFIGetTypeInfo(object_type_index);
            return (type_info->type_depth > TargetType::_type_depth &&
                    type_info->type_acenstors[TargetType::_type_depth]->type_index == target_type_index);
        } else {
            return false;
        }
    }
}

/*!
 * \brief Namespace to internally manipulate object class.
 * \note These functions are only supposed to be used by internal
 * implementations and not external users of the tvm::ffi
 */
struct ObjectUnsafe {
    // NOTE: get ffi header from an object
    TVM_FFI_INLINE static TVMFFIObject* GetHeader(const Object* src) {
        return const_cast<TVMFFIObject*>(&src->header_);
    }

    template<typename Class>
    TVM_FFI_INLINE static int64_t GetObjectOffsetToSubclass() {
        // return reinterpret_cast<int64_t>(&static_cast<Class*>(nullptr)->header_) -
        //        reinterpret_cast<int64_t>(&static_cast<Object*>(nullptr)->header_);

        return reinterpret_cast<char*>(&static_cast<Class*>(nullptr)->header_) -
               reinterpret_cast<char*>(&static_cast<Object*>(nullptr)->header_);
    }

    template<typename T>
    TVM_FFI_INLINE static ObjectPtr<T> ObjectPtrFromObjectRef(const ObjectRef& ref) {
        if constexpr (std::is_same_v<T, Object>) {
            return ref.data_;
        } else {
            return ffi::ObjectPtr<T>(ref.data_.data_);
        }
    }

    template<typename T>
    TVM_FFI_INLINE static ObjectPtr<T> ObjectPtrFromObjectRef(ObjectRef&& ref) {
        if constexpr (std::is_same_v<T, Object>) {
            return std::move(ref.data_);
        } else {
            return ffi::ObjectPtr<T>(std::move(ref.data_.data_));
        }
    }

    template<typename T>
    TVM_FFI_INLINE static ObjectPtr<T> ObjectPtrFromOwned(Object* raw_ptr) {
        ObjectPtr<T> ptr;
        ptr.data_ = raw_ptr;
        return ptr;
    }

    template<typename T>
    TVM_FFI_INLINE static ObjectPtr<T> ObjectPtrFromOwned(TVMFFIObject* obj_ptr) {
        return ObjectPtrFromOwned<T>(reinterpret_cast<Object*>(obj_ptr));
    }

    template<typename T>
    TVM_FFI_INLINE static T* RawObjectPtrFromUnowned(TVMFFIObject* obj_ptr) {
        // NOTE: this is important to first cast to Object*
        // then cast back to T* because objptr and tptr may not be the same
        // depending on how sub-class allocates the space.
        return static_cast<T*>(reinterpret_cast<Object*>(obj_ptr));
    }

    // Create ObjectPtr from unowned ptr
    template<typename T>
    TVM_FFI_INLINE static ObjectPtr<T> ObjectPtrFromUnowned(Object* raw_ptr) {
        return ffi::ObjectPtr<T>(raw_ptr);
    }

    template<typename T>
    TVM_FFI_INLINE static ObjectPtr<T> ObjectPtrFromUnowned(TVMFFIObject* obj_ptr) {
        return ffi::ObjectPtr<T>(reinterpret_cast<Object*>(obj_ptr));
    }

    TVM_FFI_INLINE static void DecRefObjectHandle(TVMFFIObjectHandle handle) {
        static_cast<Object*>(handle)->DecRef();
    }

    TVM_FFI_INLINE static void IncRefObjectHandle(TVMFFIObjectHandle handle) {
        static_cast<Object*>(handle)->IncRef();
    }

    TVM_FFI_INLINE static Object* RawObjectPtrFromObjectRef(const ObjectRef& src) {
        return src.data_.data_;
    }

    TVM_FFI_INLINE static TVMFFIObject* TVMFFIObjectPtrFromObjectRef(const ObjectRef& src) {
        return GetHeader(src.data_.data_);
    }

    template<typename T>
    TVM_FFI_INLINE static TVMFFIObject* TVMFFIObjectPtrFromObjectPtr(const ObjectPtr<T>& src) {
        return GetHeader(src.data_);
    }

    template<typename T>
    TVM_FFI_INLINE static TVMFFIObject* MoveObjectPtrToTVMFFIObjectPtr(ObjectPtr<T>&& src) {
        Object* obj_ptr = src.data_;
        src.data_ = nullptr;
        return GetHeader(obj_ptr);
    }

    TVM_FFI_INLINE static TVMFFIObject* MoveObjectRefToTVMFFIObjectPtr(ObjectRef&& src) {
        Object* obj_ptr = src.data_.data_;
        src.data_.data_ = nullptr;
        return GetHeader(obj_ptr);
    }
};
}// namespace details
}// namespace ffi
}// namespace litetvm

#endif//LITETVM_FFI_OBJECT_H
