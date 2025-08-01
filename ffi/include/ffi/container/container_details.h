//
// Created by richard on 5/15/25.
//

#ifndef LITETVM_FFI_CONTAINER_CONTAINER_DETAILS_H
#define LITETVM_FFI_CONTAINER_CONTAINER_DETAILS_H

#include "ffi/object.h"

#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace litetvm {
namespace ffi {
namespace details {
/*!
 * \brief Base template for classes with array like memory layout.
 *
 *        It provides general methods to access the memory. The memory
 *        layout is ArrayType + [ElemType]. The alignment of ArrayType
 *        and ElemType is handled by the memory allocator.
 *
 * \tparam ArrayType The array header type, contains object specific metadata.
 * \tparam ElemType The type of objects stored in the array right after
 * ArrayType.
 *
 * \code
 * // Example usage of the template to define a simple array wrapper
 * class ArrayObj : public tvm::ffi::details::InplaceArrayBase<ArrayObj, Elem> {
 * public:
 *  // Wrap EmplaceInit to initialize the elements
 *  template <typename Iterator>
 *  void Init(Iterator begin, Iterator end) {
 *   size_t num_elems = std::distance(begin, end);
 *   auto it = begin;
 *   this->size = 0;
 *   for (size_t i = 0; i < num_elems; ++i) {
 *     InplaceArrayBase::EmplaceInit(i, *it++);
 *     this->size++;
 *   }
 *  }
 * }
 *
 * void test_function() {
 *   vector<Elem> fields;
 *   auto ptr = make_inplace_array_object<ArrayObj, Elem>(fields.size());
 *   ptr->Init(fields.begin(), fields.end());
 *
 *   // Access the 0th element in the array.
 *   assert(ptr->operator[](0) == fields[0]);
 * }
 *
 * \endcode
 */
template<typename ArrayType, typename ElemType>
class InplaceArrayBase {
public:
    /*!
   * \brief Access element at index
   * \param idx The index of the element.
   * \return Const reference to ElemType at the index.
   */
    const ElemType& operator[](size_t idx) const {
        size_t size = Self()->GetSize();
        if (idx > size) {
            TVM_FFI_THROW(IndexError) << "Index " << idx << " out of bounds " << size;
        }
        return *static_cast<ElemType*>(AddressOf(idx));
    }

    /*!
   * \brief Access element at index
   * \param idx The index of the element.
   * \return Reference to ElemType at the index.
   */
    ElemType& operator[](size_t idx) {
        size_t size = Self()->GetSize();
        if (idx > size) {
            TVM_FFI_THROW(IndexError) << "Index " << idx << " out of bounds " << size;
        }
        return *static_cast<ElemType*>(AddressOf(idx));
    }

    /*!
   * \brief Destroy the Inplace Array Base object
   */
    ~InplaceArrayBase() {
        if constexpr (!(std::is_standard_layout_v<ElemType> && std::is_trivial_v<ElemType>) ) {
            size_t size = Self()->GetSize();
            for (size_t i = 0; i < size; ++i) {
                auto* fp = static_cast<ElemType*>(AddressOf(i));
                fp->ElemType::~ElemType();
            }
        }
    }

protected:
    /*!
   * \brief Construct a value in place with the arguments.
   *
   * \tparam Args Type parameters of the arguments.
   * \param idx Index of the element.
   * \param args Arguments to construct the new value.
   *
   * \note Please make sure ArrayType::GetSize returns 0 before first call of
   * EmplaceInit, and increment GetSize by 1 each time EmplaceInit succeeds.
   */
    template<typename... Args>
    void EmplaceInit(size_t idx, Args&&... args) {
        void* field_ptr = AddressOf(idx);
        new (field_ptr) ElemType(std::forward<Args>(args)...);
    }

    /*!
   * \brief Return the self object for the array.
   *
   * \return Pointer to ArrayType.
   */
    ArrayType* Self() const {
        return static_cast<ArrayType*>(const_cast<InplaceArrayBase*>(this));
    }

    /*!
   * \brief Return the raw pointer to the element at idx.
   *
   * \param idx The index of the element.
   * \return Raw pointer to the element.
   */
    NODISCARD void* AddressOf(size_t idx) const {
        static_assert(alignof(ArrayType) % alignof(ElemType) == 0 && sizeof(ArrayType) % alignof(ElemType) == 0,
                      "The size and alignment of ArrayType should respect ElemType's alignment.");

        size_t kDataStart = sizeof(ArrayType);
        ArrayType* self = Self();
        char* data_start = reinterpret_cast<char*>(self) + kDataStart;
        return data_start + idx * sizeof(ElemType);
    }
};

/*!
 * \brief iterator adapter that adapts TIter to return another type.
 * \tparam Converter a struct that contains converting function
 * \tparam TIter the content iterator type.
 */
template<typename Converter, typename TIter>
class IterAdapter {
public:
    using difference_type = typename std::iterator_traits<TIter>::difference_type;
    using value_type = typename Converter::ResultType;
    using pointer = typename Converter::ResultType*;
    using reference = typename Converter::ResultType&;
    using iterator_category = typename std::iterator_traits<TIter>::iterator_category;

    explicit IterAdapter(TIter iter) : iter_(iter) {}

    IterAdapter& operator++() {
        ++iter_;
        return *this;
    }

    IterAdapter& operator--() {
        --iter_;
        return *this;
    }

    IterAdapter operator++(int) {
        IterAdapter copy = *this;
        ++iter_;
        return copy;
    }

    IterAdapter operator--(int) {
        IterAdapter copy = *this;
        --iter_;
        return copy;
    }

    IterAdapter operator+(difference_type offset) const {
        return IterAdapter(iter_ + offset);
    }

    IterAdapter operator-(difference_type offset) const {
        return IterAdapter(iter_ - offset);
    }

    IterAdapter& operator+=(difference_type offset) {
        iter_ += offset;
        return *this;
    }

    IterAdapter& operator-=(difference_type offset) {
        iter_ -= offset;
        return *this;
    }

    template<typename T = IterAdapter>
    std::enable_if_t<std::is_same_v<iterator_category, std::random_access_iterator_tag>,
                     typename T::difference_type>
    operator-(const IterAdapter& rhs) const {
        return iter_ - rhs.iter_;
    }

    bool operator==(IterAdapter other) const {
        return iter_ == other.iter_;
    }

    bool operator!=(IterAdapter other) const {
        return !(*this == other);
    }

    const value_type operator*() const {
        return Converter::convert(*iter_);
    }

private:
    TIter iter_;
};

/*!
 * \brief iterator adapter that adapts TIter to return another type.
 * \tparam Converter a struct that contains converting function
 * \tparam TIter the content iterator type.
 */
template<typename Converter, typename TIter>
class ReverseIterAdapter {
public:
    using difference_type = typename std::iterator_traits<TIter>::difference_type;
    using value_type = typename Converter::ResultType;
    using pointer = typename Converter::ResultType*;
    using reference = typename Converter::ResultType&;// NOLINT(*)
    using iterator_category = typename std::iterator_traits<TIter>::iterator_category;

    explicit ReverseIterAdapter(TIter iter) : iter_(iter) {}
    ReverseIterAdapter& operator++() {
        --iter_;
        return *this;
    }
    ReverseIterAdapter& operator--() {
        ++iter_;
        return *this;
    }
    ReverseIterAdapter operator++(int) {
        ReverseIterAdapter copy = *this;
        --iter_;
        return copy;
    }
    ReverseIterAdapter operator--(int) {
        ReverseIterAdapter copy = *this;
        ++iter_;
        return copy;
    }
    ReverseIterAdapter operator+(difference_type offset) const {
        return ReverseIterAdapter(iter_ - offset);
    }

    template<typename T = ReverseIterAdapter>
    typename std::enable_if<std::is_same<iterator_category, std::random_access_iterator_tag>::value,
                            typename T::difference_type>::type inline
    operator-(const ReverseIterAdapter& rhs) const {
        return rhs.iter_ - iter_;
    }

    bool operator==(ReverseIterAdapter other) const { return iter_ == other.iter_; }
    bool operator!=(ReverseIterAdapter other) const { return !(*this == other); }
    const value_type operator*() const { return Converter::convert(*iter_); }

private:
    TIter iter_;
};

/*!
 * \brief Check if T is compatible with Any.
 *
 * \tparam T The type to check.
 * \return True if T is compatible with Any, false otherwise.
 */
template<typename T>
inline constexpr bool storage_enabled_v = std::is_same_v<T, Any> || TypeTraits<T>::storage_enabled;

/*!
 * \brief Check if all T are compatible with Any.
 *
 * \tparam T The type to check.
 * \return True if T is compatible with Any, false otherwise.
 */
template<typename... T>
inline constexpr bool all_storage_enabled_v = (storage_enabled_v<T> && ...);

/*!
 * \brief Check if all T are compatible with Any.
 *
 * \tparam T The type to check.
 * \return True if T is compatible with Any, false otherwise.
 */
template<typename... T>
inline constexpr bool all_object_ref_v = (std::is_base_of_v<ObjectRef, T> && ...);
/**
 * \brief Check if Any storage of Derived can always be directly used as Base.
 *
 * \tparam Base The base type.
 * \tparam Derived The derived type.
 * \return True if Derived's storage can be used as Base's storage, false otherwise.
 */
template<typename Base, typename Derived>
inline constexpr bool type_contains_v = std::is_base_of_v<Base, Derived> || std::is_same_v<Base, Derived>;
// special case for Any
template<typename Derived>
inline constexpr bool type_contains_v<Any, Derived> = true;

/*!
 * \brief Create a string of the container type.
 * \tparam V The types of the elements in the container.
 * \param name The name of the container type.
 * \return A string of the container type.
 */
template<typename... V>
std::string ContainerTypeStr(const char* name) {
    std::stringstream ss;
    // helper to construct concated string of TypeStr
    class TypeStrHelper {
    public:
        TypeStrHelper(std::stringstream& stream) : stream_(stream) {}// NOLINT(*)

        TypeStrHelper& operator<<(const std::string& str) {
            if (counter_ > 0) {
                stream_ << ", ";
            }
            stream_ << str;
            counter_++;
            return *this;
        }

    private:
        std::stringstream& stream_;// NOLINT(*)
        int counter_ = 0;
    };
    TypeStrHelper helper(ss);
    ss << name << '<';
    (helper << ... << Type2Str<V>::v());
    ss << '>';
    return ss.str();
}

}// namespace details
}// namespace ffi
}// namespace litetvm

#endif//LITETVM_FFI_CONTAINER_CONTAINER_DETAILS_H
