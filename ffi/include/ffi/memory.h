//
// Created by richard on 5/15/25.
//

#ifndef LITETVM_FFI_MEMORY_H
#define LITETVM_FFI_MEMORY_H

#include "ffi/object.h"

#include <type_traits>
#include <utility>

namespace litetvm {
namespace ffi {

/*! \brief Deleter function for obeject */
// typedef void (*FObjectDeleter)(TVMFFIObject* obj);
using FObjectDeleter = void (*)(TVMFFIObject* obj);

/*!
 * \brief Allocate an object using default allocator.
 * \param args arguments to the constructor.
 * \tparam T the node type.
 * \return The ObjectPtr to the allocated object.
 */
template<typename T, typename... Args>
ObjectPtr<T> make_object(Args&&... args);

// Detail implementations after this
//
// The current design allows swapping the
// allocator pattern when necessary.
//
// Possible future allocator optimizations:
// - Arena allocator that gives ownership of memory to arena (deleter = nullptr)
// - Thread-local object pools: one pool per size and alignment requirement.
// - Can specialize by type of object to give the specific allocator to each object.

/*!
 * \brief Base class of object allocators that implements make.
 *  Use curiously recurring template pattern (CRTP).
 *
 * \tparam Derived The derived class.
 */
template<typename Derived>
class ObjAllocatorBase {
public:
    /*!
   * \brief Make a new object using the allocator.
   * \tparam T The type to be allocated.
   * \tparam Args The constructor signature.
   * \param args The arguments.
   */
    template<typename T, typename... Args>
    ObjectPtr<T> make_object(Args&&... args) {
        using Handler = typename Derived::template Handler<T>;
        static_assert(std::is_base_of_v<Object, T>, "make can only be used to create Object");
        T* ptr = Handler::New(static_cast<Derived*>(this), std::forward<Args>(args)...);
        TVMFFIObject* ffi_ptr = details::ObjectUnsafe::GetHeader(ptr);
        ffi_ptr->ref_counter = 1;
        ffi_ptr->type_index = T::RuntimeTypeIndex();
        ffi_ptr->deleter = Handler::Deleter();
        return details::ObjectUnsafe::ObjectPtrFromOwned<T>(ptr);
    }

    /*!
   * \tparam ArrayType The type to be allocated.
   * \tparam ElemType The type of array element.
   * \tparam Args The constructor signature.
   * \param num_elems The number of array elements.
   * \param args The arguments.
   */
    template<typename ArrayType, typename ElemType, typename... Args>
    ObjectPtr<ArrayType> make_inplace_array(size_t num_elems, Args&&... args) {
        using Handler = typename Derived::template ArrayHandler<ArrayType, ElemType>;
        static_assert(std::is_base_of_v<Object, ArrayType>, "make_inplace_array can only be used to create Object");
        ArrayType* ptr = Handler::New(static_cast<Derived*>(this), num_elems, std::forward<Args>(args)...);
        TVMFFIObject* ffi_ptr = details::ObjectUnsafe::GetHeader(ptr);
        ffi_ptr->ref_counter = 1;
        ffi_ptr->type_index = ArrayType::RuntimeTypeIndex();
        ffi_ptr->deleter = Handler::Deleter();
        return details::ObjectUnsafe::ObjectPtrFromOwned<ArrayType>(ptr);
    }
};

// Simple allocator that uses new/delete.
class SimpleObjAllocator : public ObjAllocatorBase<SimpleObjAllocator> {
public:
    template<typename T>
    class Handler {
    public:
        struct alignas(T) StorageType {
            char data[sizeof(T)];
        };

        template<typename... Args>
        static T* New(SimpleObjAllocator*, Args&&... args) {
            // NOTE: the first argument is not needed for SimpleObjAllocator
            // It is reserved for special allocators that needs to recycle
            // the object to itself (e.g. in the case of object pool).
            //
            // In the case of an object pool, an allocator needs to create
            // a special chunk memory that hides reference to the allocator
            // and call allocator's release function in the deleter.

            // NOTE2: Use inplace new to allocate
            // This is used to get rid of warning when deleting a virtual
            // class with non-virtual destructor.
            // We are fine here as we captured the right deleter during construction.
            // This is also the right way to get storage type for an object pool.
            auto* data = new StorageType();
            new (data) T(std::forward<Args>(args)...);
            return reinterpret_cast<T*>(data);
        }

        static FObjectDeleter Deleter() {
            return Deleter_;
        }

    private:
        static void Deleter_(TVMFFIObject* objptr) {
            T* tptr = details::ObjectUnsafe::RawObjectPtrFromUnowned<T>(objptr);
            // It is important to do tptr->T::~T(),
            // so that we explicitly call the specific destructor
            // instead of tptr->~T(), which could mean the intention
            // calls a virtual destructor (which may not be available and is not required).
            tptr->T::~T();
            delete reinterpret_cast<StorageType*>(tptr);
        }
    };

    // Array handler that uses new/delete.
    template<typename ArrayType, typename ElemType>
    class ArrayHandler {
    public:
        using StorageType = std::aligned_storage_t<sizeof(ArrayType), alignof(ArrayType)>;
        // for now only support elements that aligns with array header.
        static_assert(alignof(ArrayType) % alignof(ElemType) == 0 && sizeof(ArrayType) % alignof(ElemType) == 0,
                      "element alignment constraint");

        template<typename... Args>
        static ArrayType* New(SimpleObjAllocator*, size_t num_elems, Args&&... args) {
            // NOTE: the first argument is not needed for ArrayObjAllocator
            // It is reserved for special allocators that needs to recycle
            // the object to itself (e.g. in the case of object pool).
            //
            // In the case of an object pool, an allocator needs to create
            // a special chunk memory that hides reference to the allocator
            // and call allocator's release function in the deleter.
            // NOTE2: Use inplace new to allocate
            // This is used to get rid of warning when deleting a virtual
            // class with non-virtual destructor.
            // We are fine here as we captured the right deleter during construction.
            // This is also the right way to get storage type for an object pool.
            size_t unit = sizeof(StorageType);
            size_t requested_size = num_elems * sizeof(ElemType) + sizeof(ArrayType);
            size_t num_storage_slots = (requested_size + unit - 1) / unit;
            auto* data = new StorageType[num_storage_slots];
            new (data) ArrayType(std::forward<Args>(args)...);
            return reinterpret_cast<ArrayType*>(data);
        }

        static FObjectDeleter Deleter() {
            return Deleter_;
        }

    private:
        static void Deleter_(TVMFFIObject* objptr) {
            ArrayType* tptr = details::ObjectUnsafe::RawObjectPtrFromUnowned<ArrayType>(objptr);
            // It is important to do tptr->ArrayType::~ArrayType(),
            // so that we explicitly call the specific destructor
            // instead of tptr->~ArrayType(), which could mean the intention
            // call a virtual destructor(which may not be available and is not required).
            tptr->ArrayType::~ArrayType();
            auto* p = reinterpret_cast<StorageType*>(tptr);
            delete[] p;
        }
    };
};

template<typename T, typename... Args>
ObjectPtr<T> make_object(Args&&... args) {
    return SimpleObjAllocator().make_object<T>(std::forward<Args>(args)...);
}

template<typename ArrayType, typename ElemType, typename... Args>
ObjectPtr<ArrayType> make_inplace_array_object(size_t num_elems, Args&&... args) {
    return SimpleObjAllocator().make_inplace_array<ArrayType, ElemType>(num_elems, std::forward<Args>(args)...);
}

}// namespace ffi

// Export the make_object function
// rationale: ease of use, and no ambiguity
using ffi::make_object;
}// namespace litetvm

#endif//LITETVM_FFI_MEMORY_H
