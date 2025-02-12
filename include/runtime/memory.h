//
// Created by richard on 1/30/25.
//

#ifndef MEMORY_H
#define MEMORY_H

#include "runtime/object.h"
#include <type_traits>
#include <utility>

// nested namespace
namespace litetvm::runtime {

/*!
 * \brief Allocate an object using default allocator.
 * \param args arguments to the constructor.
 * \tparam T the node type.
 * \return The ObjectPtr to the allocated object.
 */
template <typename T, typename... Args>
ObjectPtr<T> make_object(Args&&... args);

/*!
 * \brief Base class of object allocators that implements make.
 *  Use curiously recurring template pattern.
 *
 * \tparam Derived The derived class.
 */
template<typename Derived>
class BaseAllocator {
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
        ptr->type_index_ = T::RuntimeTypeIndex();
        ptr->deleter_ = Handler::Deleter();
        return ObjectPtr<T>(ptr);
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
        static_assert(std::is_base_of_v<Object, ArrayType>,
                      "make_inplace_array can only be used to create Object");
        ArrayType* ptr = Handler::New(static_cast<Derived*>(this), num_elems, std::forward<Args>(args)...);
        ptr->type_index_ = ArrayType::RuntimeTypeIndex();
        ptr->deleter_ = Handler::Deleter();
        return ObjectPtr<ArrayType>(ptr);
    }
};

class ObjectAllocator : public BaseAllocator<ObjectAllocator> {
public:
    template<typename T>
    class Handler {
    public:
        using StorageType = std::aligned_storage_t<sizeof(T), alignof(T)>;

        template<typename... Args>
        static T* New(ObjectAllocator*, Args&&... args) {
            auto* data = new StorageType;
            // auto* data = ::operator new(sizeof(StorageType));
            ::new (data) T(std::forward<Args>(args)...);
            return reinterpret_cast<T*>(data);
        }

        static Object::FDeleter Deleter() {
            return Deleter_;
        }

    private:
        static void Deleter_(Object* objptr) {
            // std::cout << "delete object.\n";
            T* tptr = static_cast<T*>(objptr);
            tptr->T::~T();
            delete reinterpret_cast<StorageType*>(tptr);
        }
    };

    template<typename ArrayType, typename ElemType>
    class ArrayHandler {
    public:
        using StorageType = std::aligned_storage_t<sizeof(ArrayType), alignof(ArrayType)>;
        // for now only support elements that aligns with array header.
        static_assert(alignof(ArrayType) % alignof(ElemType) == 0 &&
                              sizeof(ArrayType) % alignof(ElemType) == 0,
                      "element alignment constraint");

        template<typename... Args>
        static ArrayType* New(ObjectAllocator*, size_t num_elems, Args&&... args) {
            size_t unit = sizeof(StorageType);
            size_t requested_size = num_elems * sizeof(ElemType) + sizeof(ArrayType);
            size_t num_storage_slots = (requested_size + unit - 1) / unit;
            auto* data = new StorageType[num_storage_slots];
            ::new (data) ArrayType(std::forward<Args>(args)...);
            return reinterpret_cast<ArrayType*>(data);
        }

        static Object::FDeleter Deleter() {
            return Deleter_;
        }

    private:
        static void Deleter_(Object* objptr) {
            // NOTE: this is important to cast back to ArrayType*
            // because objptr and tptr may not be the same
            // depending on how sub-class allocates the space.
            auto* tptr = static_cast<ArrayType*>(objptr);
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
    return ObjectAllocator().make_object<T>(std::forward<Args>(args)...);
}

template<typename ArrayType, typename ElemType, typename... Args>
ObjectPtr<ArrayType> make_inplace_array_object(size_t num_elems, Args&&... args) {
    return ObjectAllocator().make_inplace_array<ArrayType, ElemType>(num_elems, std::forward<Args>(args)...);
}

}// namespace litetvm::runtime

#endif//MEMORY_H
