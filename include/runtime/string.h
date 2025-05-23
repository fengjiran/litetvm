//
// Created by richard on 2/4/25.
//

#ifndef STRING_H
#define STRING_H

#include <utility>
#include <cstring>
#include <dmlc/endian.h>

#include "runtime/base.h"
#include "runtime/memory.h"
#include "runtime/object.h"

namespace litetvm::runtime {

// Forward declare TVMArgValue
class TVMArgValue;

/*! \brief An object representing string. It's POD type. */
class StringObj : public Object {
public:
    explicit StringObj(std::string other) : data_container(std::move(other)) {
        data = data_container.data();
        size = data_container.size();
    }

    /*! \brief Container that holds the memory. */
    std::string data_container;
    /*! \brief The pointer to string data. */
    const char* data;

    /*! \brief The length of the string object. */
    uint64_t size;

    static constexpr uint32_t _type_index = static_cast<uint32_t>(TypeIndex::kRuntimeString);
    static constexpr const char* _type_key = "runtime.String";
    TVM_DECLARE_FINAL_OBJECT_INFO(StringObj, Object);
};

/*!
 * \brief Reference to string objects.
 *
 * \code
 *
 * // Example to create runtime String reference object from std::string
 * std::string s = "hello world";
 *
 * // You can create the reference from existing std::string
 * String ref{std::move(s)};
 *
 * // You can rebind the reference to another string.
 * ref = std::string{"hello world2"};
 *
 * // You can use the reference as hash map key
 * std::unordered_map<String, int32_t> m;
 * m[ref] = 1;
 *
 * // You can compare the reference object with other string objects
 * assert(ref == "hello world", true);
 *
 * // You can convert the reference to std::string again
 * string s2 = (string)ref;
 *
 * \endcode
 */
class String : public ObjectRef {
public:
    /*!
   * \brief Construct an empty string.
   */
    String() : String(std::string()) {}

    /*!
   * \brief Construct a new String object
   *
   * \param other The moved/copied std::string object
   *
   * \note If user passes const reference, it will trigger copy. If it's rvalue,
   * it will be moved into other.
   */
    String(std::string other) {
        auto ptr = make_object<StringObj>(std::move(other));
        data_ = std::move(ptr);
    }

    /*!
   * \brief Construct a new String object
   *
   * \param other a char array.
   */
    String(const char* other) : String(std::string(other)) {}

    /*!
   * \brief Construct a new null object
   */
    String(std::nullptr_t) : ObjectRef(nullptr) {}

    /*!
   * \brief Change the value the reference object points to.
   *
   * \param other The value for the new String
   *
   */
    String& operator=(std::string other) {
        String tmp(std::move(other));
        data_.swap(tmp.data_);
        return *this;
    }

    /*!
   * \brief Change the value the reference object points to.
   *
   * \param other The value for the new String
   */
    String& operator=(const char* other) {
        String tmp(other);
        data_.swap(tmp.data_);
        return *this;
    }

    /*!
   * \brief Return the data pointer
   *
   * \return const char* data pointer
   */
    NODISCARD const char* data() const {
        return get()->data;
    }

    /*!
   * \brief Returns a pointer to the char array in the string.
   *
   * \return const char*
   */
    NODISCARD const char* c_str() const {
        return get()->data;
    }

    /*!
   * \brief Return the length of the string
   *
   * \return size_t string length
   */
    NODISCARD size_t size() const {
        const auto* ptr = get();
        return ptr->size;
    }

    /*!
   * \brief Return the length of the string
   *
   * \return size_t string length
   */
    NODISCARD size_t length() const {
        return size();
    }

    /*!
     * \brief Retun if the string is empty
     *
     * \return true if empty, false otherwise.
     */
    NODISCARD bool empty() const {
        return size() == 0;
    }

    /*!
   * \brief Read an element.
   * \param pos The position at which to read the character.
   *
   * \return The char at position
   */
    NODISCARD char at(size_t pos) const {
        if (pos < size()) {
            return data()[pos];
        }

        throw std::out_of_range("String index out of bounds");
    }

    /*!
   * \brief Compares this String object to other
   *
   * \param other The String to compare with.
   *
   * \return zero if both char sequences compare equal. negative if this appear
   * before other, positive otherwise.
   */
    NODISCARD int compare(const String& other) const {
        return memncmp(data(), other.data(), size(), other.size());
    }

    /*!
   * \brief Compares this String object to other
   *
   * \param other The string to compare with.
   *
   * \return zero if both char sequences compare equal. negative if this appear
   * before other, positive otherwise.
   */
    NODISCARD int compare(const std::string& other) const {
        return memncmp(data(), other.data(), size(), other.size());
    }

    /*!
   * \brief Compares this to other
   *
   * \param other The character array to compare with.
   *
   * \return zero if both char sequences compare equal. negative if this appear
   * before other, positive otherwise.
   */
    int compare(const char* other) const {
        return memncmp(data(), other, size(), std::strlen(other));
    }

    /*!
   * \brief Convert String to an std::string object
   *
   * \return std::string
   */
    operator std::string() const {
        return std::string{get()->data, size()};
    }

    /*!
   * \brief Check if a TVMArgValue can be converted to String, i.e. it can be std::string or String
   * \param val The value to be checked
   * \return A boolean indicating if val can be converted to String
   */
    inline static bool CanConvertFrom(const TVMArgValue& val);

    /*!
   * \brief Hash the binary bytes
   * \param data The data pointer
   * \param size The size of the bytes.
   * \return the hash value.
   */
    static uint64_t StableHashBytes(const char* data, size_t size) {
        constexpr uint64_t kMultiplier = 1099511628211ULL;
        constexpr uint64_t kMod = 2147483647ULL;
        union Union {
            uint8_t a[8];
            uint64_t b;
        } u;

        static_assert(sizeof(Union) == sizeof(uint64_t), "sizeof(Union) != sizeof(uint64_t)");
        const char* it = data;
        const char* end = it + size;
        uint64_t result = 0;
        for (; it + 8 <= end; it += 8) {
            if (DMLC_IO_NO_ENDIAN_SWAP) {
                u.a[0] = it[0];
                u.a[1] = it[1];
                u.a[2] = it[2];
                u.a[3] = it[3];
                u.a[4] = it[4];
                u.a[5] = it[5];
                u.a[6] = it[6];
                u.a[7] = it[7];
            } else {
                u.a[0] = it[7];
                u.a[1] = it[6];
                u.a[2] = it[5];
                u.a[3] = it[4];
                u.a[4] = it[3];
                u.a[5] = it[2];
                u.a[6] = it[1];
                u.a[7] = it[0];
            }
            result = (result * kMultiplier + u.b) % kMod;
        }
        if (it < end) {
            u.b = 0;
            uint8_t* a = u.a;
            if (it + 4 <= end) {
                a[0] = it[0];
                a[1] = it[1];
                a[2] = it[2];
                a[3] = it[3];
                it += 4;
                a += 4;
            }
            if (it + 2 <= end) {
                a[0] = it[0];
                a[1] = it[1];
                it += 2;
                a += 2;
            }
            if (it + 1 <= end) {
                a[0] = it[0];
                it += 1;
                a += 1;
            }
            if (!DMLC_IO_NO_ENDIAN_SWAP) {
                std::swap(u.a[0], u.a[7]);
                std::swap(u.a[1], u.a[6]);
                std::swap(u.a[2], u.a[5]);
                std::swap(u.a[3], u.a[4]);
            }
            result = (result * kMultiplier + u.b) % kMod;
        }
        return result;
    }


    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(String, ObjectRef, StringObj);

private:
    /*!
   * \brief Compare two char sequence
   *
   * \param lhs Pointers to the char array to compare
   * \param rhs Pointers to the char array to compare
   * \param lhs_count Length of the char array to compare
   * \param rhs_count Length of the char array to compare
   * \return int zero if both char sequences compare equal. negative if this
   * appear before other, positive otherwise.
   */
    static int memncmp(const char* lhs, const char* rhs, size_t lhs_count, size_t rhs_count);

    /*!
   * \brief Concatenate two char sequences
   *
   * \param lhs Pointers to the lhs char array
   * \param lhs_size The size of the lhs char array
   * \param rhs Pointers to the rhs char array
   * \param rhs_size The size of the rhs char array
   *
   * \return The concatenated char sequence
   */
    static String Concat(const char* lhs, size_t lhs_size, const char* rhs, size_t rhs_size) {
        std::string ret(lhs, lhs_size);
        ret.append(rhs, rhs_size);
        return {ret};
    }

    // Overload + operator
    friend String operator+(const String& lhs, const String& rhs);
    friend String operator+(const String& lhs, const std::string& rhs);
    friend String operator+(const std::string& lhs, const String& rhs);
    friend String operator+(const String& lhs, const char* rhs);
    friend String operator+(const char* lhs, const String& rhs);

    friend struct ObjectEqual;
};

inline String operator+(const String& lhs, const String& rhs) {
    size_t lhs_size = lhs.size();
    size_t rhs_size = rhs.size();
    return String::Concat(lhs.data(), lhs_size, rhs.data(), rhs_size);
}

inline String operator+(const String& lhs, const std::string& rhs) {
    size_t lhs_size = lhs.size();
    size_t rhs_size = rhs.size();
    return String::Concat(lhs.data(), lhs_size, rhs.data(), rhs_size);
}

inline String operator+(const std::string& lhs, const String& rhs) {
    size_t lhs_size = lhs.size();
    size_t rhs_size = rhs.size();
    return String::Concat(lhs.data(), lhs_size, rhs.data(), rhs_size);
}

inline String operator+(const char* lhs, const String& rhs) {
    size_t lhs_size = std::strlen(lhs);
    size_t rhs_size = rhs.size();
    return String::Concat(lhs, lhs_size, rhs.data(), rhs_size);
}

inline String operator+(const String& lhs, const char* rhs) {
    size_t lhs_size = lhs.size();
    size_t rhs_size = std::strlen(rhs);
    return String::Concat(lhs.data(), lhs_size, rhs, rhs_size);
}

// Overload < operator
inline bool operator<(const String& lhs, const std::string& rhs) {
    return lhs.compare(rhs) < 0;
}

inline bool operator<(const std::string& lhs, const String& rhs) {
    return rhs.compare(lhs) > 0;
}

inline bool operator<(const String& lhs, const String& rhs) {
    return lhs.compare(rhs) < 0;
}

inline bool operator<(const String& lhs, const char* rhs) {
    return lhs.compare(rhs) < 0;
}

inline bool operator<(const char* lhs, const String& rhs) {
    return rhs.compare(lhs) > 0;
}

// Overload > operator
inline bool operator>(const String& lhs, const std::string& rhs) {
    return lhs.compare(rhs) > 0;
}

inline bool operator>(const std::string& lhs, const String& rhs) {
    return rhs.compare(lhs) < 0;
}

inline bool operator>(const String& lhs, const String& rhs) {
    return lhs.compare(rhs) > 0;
}

inline bool operator>(const String& lhs, const char* rhs) {
    return lhs.compare(rhs) > 0;
}

inline bool operator>(const char* lhs, const String& rhs) {
    return rhs.compare(lhs) < 0;
}

// Overload <= operator
inline bool operator<=(const String& lhs, const std::string& rhs) {
    return lhs.compare(rhs) <= 0;
}

inline bool operator<=(const std::string& lhs, const String& rhs) {
    return rhs.compare(lhs) >= 0;
}

inline bool operator<=(const String& lhs, const String& rhs) {
    return lhs.compare(rhs) <= 0;
}

inline bool operator<=(const String& lhs, const char* rhs) {
    return lhs.compare(rhs) <= 0;
}

inline bool operator<=(const char* lhs, const String& rhs) {
    return rhs.compare(lhs) >= 0;
}

// Overload >= operator
inline bool operator>=(const String& lhs, const std::string& rhs) {
    return lhs.compare(rhs) >= 0;
}

inline bool operator>=(const std::string& lhs, const String& rhs) {
    return rhs.compare(lhs) <= 0;
}

inline bool operator>=(const String& lhs, const String& rhs) {
    return lhs.compare(rhs) >= 0;
}

inline bool operator>=(const String& lhs, const char* rhs) {
    return lhs.compare(rhs) >= 0;
}

inline bool operator>=(const char* lhs, const String& rhs) {
    return rhs.compare(rhs) <= 0;
}

// Overload == operator
inline bool operator==(const String& lhs, const std::string& rhs) {
    return lhs.compare(rhs) == 0;
}

inline bool operator==(const std::string& lhs, const String& rhs) {
    return rhs.compare(lhs) == 0;
}

inline bool operator==(const String& lhs, const String& rhs) {
    return lhs.compare(rhs) == 0;
}

inline bool operator==(const String& lhs, const char* rhs) {
    return lhs.compare(rhs) == 0;
}

inline bool operator==(const char* lhs, const String& rhs) {
    return rhs.compare(lhs) == 0;
}

// Overload != operator
inline bool operator!=(const String& lhs, const std::string& rhs) {
    return lhs.compare(rhs) != 0;
}

inline bool operator!=(const std::string& lhs, const String& rhs) {
    return rhs.compare(lhs) != 0;
}

inline bool operator!=(const String& lhs, const String& rhs) {
    return lhs.compare(rhs) != 0;
}

inline bool operator!=(const String& lhs, const char* rhs) {
    return lhs.compare(rhs) != 0;
}

inline bool operator!=(const char* lhs, const String& rhs) {
    return rhs.compare(lhs) != 0;
}

inline std::ostream& operator<<(std::ostream& out, const String& input) {
    out.write(input.data(), input.size());
    return out;
}

inline int String::memncmp(const char* lhs, const char* rhs, size_t lhs_count, size_t rhs_count) {
    if (lhs == rhs && lhs_count == rhs_count) {
        return 0;
    }

    for (size_t i = 0; i < std::min(lhs_count, rhs_count); ++i) {
        if (lhs[i] < rhs[i]) {
            return -1;
        }

        if (lhs[i] > rhs[i]) {
            return 1;
        }
    }

    if (lhs_count < rhs_count) {
        return -1;
    }

    if (lhs_count > rhs_count) {
        return 1;
    }

    return 0;
}

inline size_t ObjectHash::operator()(const ObjectRef& a) const {
    if (const auto* str = a.as<StringObj>()) {
        return String::StableHashBytes(str->data, str->size);
    }
    return ObjectPtrHash()(a);
}

inline bool ObjectEqual::operator()(const ObjectRef& a, const ObjectRef& b) const {
    if (a.same_as(b)) {
        return true;
    }
    if (const auto* str_a = a.as<StringObj>()) {
        if (const auto* str_b = b.as<StringObj>()) {
            return String::memncmp(str_a->data, str_b->data, str_a->size, str_b->size) == 0;
        }
    }
    return false;
}

}// namespace litetvm::runtime

namespace std {

template<>
struct hash<::litetvm::runtime::String> {
    std::size_t operator()(const ::litetvm::runtime::String& str) const noexcept {
        return ::litetvm::runtime::String::StableHashBytes(str.data(), str.size());
    }
};
}// namespace std

#endif//STRING_H
