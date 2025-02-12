//
// Created by richard on 2/4/25.
//

#ifndef BASE_H
#define BASE_H

#include "runtime/memory.h"
#include "runtime/object.h"

#include <algorithm>
#include <glog/logging.h>
#include <initializer_list>
#include <iomanip>
#include <stdexcept>
#include <utility>

#include <cstddef>

namespace litetvm {
namespace runtime {

/*! \brief String-aware ObjectRef equal functor */
struct ObjectHash {
    /*!
   * \brief Calculate the hash code of an ObjectRef
   * \param a The given ObjectRef
   * \return Hash code of a, string hash for strings and pointer address otherwise.
   */
    size_t operator()(const ObjectRef& a) const;
};

/*! \brief String-aware ObjectRef hash functor */
struct ObjectEqual {
    /*!
   * \brief Check if the two ObjectRef are equal
   * \param a One ObjectRef
   * \param b The other ObjectRef
   * \return String equality if both are strings, pointer address equality otherwise.
   */
    bool operator()(const ObjectRef& a, const ObjectRef& b) const;
};


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
 * class ArrayObj : public InplaceArrayBase<ArrayObj, Elem> {
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
        CHECK_LT(idx, size) << "Index " << idx << " out of bounds " << size << "\n";
        return *(reinterpret_cast<ElemType*>(AddressOf(idx)));
    }

    /*!
   * \brief Access element at index
   * \param idx The index of the element.
   * \return Reference to ElemType at the index.
   */
    ElemType& operator[](size_t idx) {
        size_t size = Self()->GetSize();
        CHECK_LT(idx, size) << "Index " << idx << " out of bounds " << size << "\n";
        return *(reinterpret_cast<ElemType*>(AddressOf(idx)));
    }

    /*!
   * \brief Destroy the Inplace Array Base object
   */
    ~InplaceArrayBase() {
        if (!(std::is_standard_layout<ElemType>::value && std::is_trivial<ElemType>::value)) {
            size_t size = Self()->GetSize();
            for (size_t i = 0; i < size; ++i) {
                auto* fp = reinterpret_cast<ElemType*>(AddressOf(i));
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
        static_assert(
                alignof(ArrayType) % alignof(ElemType) == 0 && sizeof(ArrayType) % alignof(ElemType) == 0,
                "The size and alignment of ArrayType should respect "
                "ElemType's alignment.");

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

    IterAdapter operator+(difference_type offset) const { return IterAdapter(iter_ + offset); }

    IterAdapter operator-(difference_type offset) const { return IterAdapter(iter_ - offset); }

    template<typename T = IterAdapter>
    typename std::enable_if<std::is_same<iterator_category, std::random_access_iterator_tag>::value,
                            typename T::difference_type>::type inline
    operator-(const IterAdapter& rhs) const {
        return iter_ - rhs.iter_;
    }

    bool operator==(IterAdapter other) const { return iter_ == other.iter_; }
    bool operator!=(IterAdapter other) const { return !(*this == other); }
    const value_type operator*() const { return Converter::convert(*iter_); }

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
 * \brief safely get the beginning address of a vector
 * \param vec input vector
 * \return beginning address of a vector
 */
template<typename T>
T* BeginPtr(std::vector<T>& vec) {// NOLINT(*)
    if (vec.size() == 0) {
        return nullptr;
    }

    return &vec[0];
}

/*!
 * \brief get the beginning address of a const vector
 * \param vec input vector
 * \return beginning address of a vector
 */
template<typename T>
const T* BeginPtr(const std::vector<T>& vec) {
    if (vec.size() == 0) {
        return nullptr;
    }
    return &vec[0];
}

/*!
 * \brief get the beginning address of a string
 * \param str input string
 * \return beginning address of a string
 */
inline char* BeginPtr(std::string& str) {// NOLINT(*)
    if (str.empty()) return nullptr;
    return &str[0];
}

/*!
 * \brief get the beginning address of a const string
 * \param str input string
 * \return beginning address of a string
 */
inline const char* BeginPtr(const std::string& str) {
    if (str.empty()) return nullptr;
    return &str[0];
}

/*!
 * \brief exception class that will be thrown by
 *  default logger if DMLC_LOG_FATAL_THROW == 1
 */
class Error : public std::runtime_error {
public:
    /*!
     * \brief constructor
     * \param s the error message
     */
    explicit Error(const std::string& s) : std::runtime_error(s) {}
};

/*!
 * \brief Error message already set in frontend env.
 *
 *  This error can be thrown by EnvCheckSignals to indicate
 *  that there is an error set in the frontend environment(e.g.
 *  python interpreter). The TVM FFI should catch this error
 *  and return a proper code tell the frontend caller about
 *  this fact.
 */
class EnvErrorAlreadySet : public Error {
public:
    /*!
     * \brief Construct an error.
     * \param s The message to be displayed with the error.
     */
    explicit EnvErrorAlreadySet(const std::string& s) : Error(s) {}
};


/*!
 * \brief Error type for errors from CHECK, ICHECK, and LOG(FATAL). This error
 * contains a backtrace of where it occurred.
 */
// class InternalError : public Error {
//  public:
//   /*! \brief Construct an error. Not recommended to use directly. Instead use LOG(FATAL).
//    *
//    * \param file The file where the error occurred.
//    * \param lineno The line number where the error occurred.
//    * \param message The error message to display.
//    * \param time The time at which the error occurred. This should be in local time.
//    * \param backtrace Backtrace from when the error occurred.
//    */
//   InternalError(std::string file, int lineno, std::string message,
//                 std::time_t time = std::time(nullptr), std::string backtrace = Backtrace())
//       : Error(""),
//         file_(file),
//         lineno_(lineno),
//         message_(message),
//         time_(time),
//         backtrace_(backtrace) {
//     std::ostringstream s;
//     // XXX: Do not change this format, otherwise all error handling in python will break (because it
//     // parses the message to reconstruct the error type).
//     // TODO(tkonolige): Convert errors to Objects, so we can avoid the mess of formatting/parsing
//     // error messages correctly.
//     s << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] " << file << ":" << lineno
//       << ": " << message << std::endl;
//     if (backtrace.size() > 0) {
//       s << backtrace << std::endl;
//     }
//     full_message_ = s.str();
//   }
//   /*! \return The file in which the error occurred. */
//   const std::string& file() const { return file_; }
//   /*! \return The message associated with this error. */
//   const std::string& message() const { return message_; }
//   /*! \return Formatted error message including file, linenumber, backtrace, and message. */
//   const std::string& full_message() const { return full_message_; }
//   /*! \return The backtrace from where this error occurred. */
//   const std::string& backtrace() const { return backtrace_; }
//   /*! \return The time at which this error occurred. */
//   const std::time_t& time() const { return time_; }
//   /*! \return The line number at which this error occurred. */
//   int lineno() const { return lineno_; }
//   virtual const char* what() const noexcept { return full_message_.c_str(); }
//
//  private:
//   std::string file_;
//   int lineno_;
//   std::string message_;
//   std::time_t time_;
//   std::string backtrace_;
//   std::string full_message_;  // holds the full error string
// };


}// namespace runtime

// expose the functions to the root namespace.
// using runtime::Downcast;
// using runtime::IterAdapter;
// using runtime::make_object;
// using runtime::Object;
// using runtime::ObjectEqual;
// using runtime::ObjectHash;
// using runtime::ObjectPtr;
// using runtime::ObjectPtrEqual;
// using runtime::ObjectPtrHash;
// using runtime::ObjectRef;
}// namespace litetvm

#endif//BASE_H
