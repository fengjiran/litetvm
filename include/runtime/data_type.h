//
// Created by 赵丹 on 25-1-13.
//

#ifndef LITETVM_RUNTIME_DATA_TYPE_H
#define LITETVM_RUNTIME_DATA_TYPE_H

#include "ffi/container/shape.h"
#include "ffi/dtype.h"
#include "runtime/logging.h"

#include <cstring>
#include <string>
#include <type_traits>

namespace litetvm::runtime {

using tvm_index_t = ffi::Shape::index_type;
/*!
 * \brief Runtime primitive data type.
 *
 *  This class is a thin wrapper of DLDataType.
 *  We also make use of DataType in compiler to store quick hint
 */
class DataType {
public:
    /*!
   * \brief Type code for the DataType.
   *
   * DLPack consistency:
   * 1) kInt is consistent with kDLInt
   * 2) kUInt is consistent with kDLUInt
   * 3) kFloat is consistent with kDLFloat
   */
    enum class TypeCode {
        kInt = kDLInt,
        kUInt = kDLUInt,
        kFloat = kDLFloat,
        kHandle = kDLOpaqueHandle,
        kBFloat = kDLBfloat,
        kFloat8_e3m4 = kDLFloat8_e3m4,
        kFloat8_e4m3 = kDLFloat8_e4m3,
        kFloat8_e4m3b11fnuz = kDLFloat8_e4m3b11fnuz,
        kFloat8_e4m3fn = kDLFloat8_e4m3fn,
        kFloat8_e4m3fnuz = kDLFloat8_e4m3fnuz,
        kFloat8_e5m2 = kDLFloat8_e5m2,
        kFloat8_e5m2fnuz = kDLFloat8_e5m2fnuz,
        kFloat8_e8m0fnu = kDLFloat8_e8m0fnu,
        kFloat6_e2m3fn = kDLFloat6_e2m3fn,
        kFloat6_e3m2fn = kDLFloat6_e3m2fn,
        kFloat4_e2m1fn = kDLFloat4_e2m1fn,
        kCustomBegin = 129
    };

    /*! \brief default constructor */
    DataType() : dtype_(Void()) {}

    /*!
     * \brief Constructor
     * \param dtype The DLDataType
     */
    explicit DataType(DLDataType dtype) : dtype_(dtype) {}

    /*!
     * \brief Constructor
     * \param code The type code.
     * \param bits The number of bits in the type.
     * \param lanes The number of lanes.
     * \param is_scalable Whether the data type is scalable (dynamic array)
     */
    DataType(int code, int bits, int lanes, bool is_scalable = false) {
        dtype_.code = static_cast<uint8_t>(code);
        dtype_.bits = static_cast<uint8_t>(bits);
        if (is_scalable) {
            ICHECK(lanes > 1) << "Invalid value for vscale factor: " << lanes;
        }
        
        dtype_.lanes = is_scalable ? static_cast<uint16_t>(-lanes) : static_cast<uint16_t>(lanes);

        if (code == static_cast<int>(TypeCode::kBFloat)) {
            CHECK_EQ(bits, 16);
        }

        if (code == static_cast<int>(TypeCode::kFloat8_e4m3fn) || code == static_cast<int>(TypeCode::kFloat8_e5m2)) {
            CHECK_EQ(bits, 8);
        }
    }

    /*! \return The type code. */
    NODISCARD int code() const {
        return dtype_.code;
    }

    /*! \return number of bits in the data. */
    NODISCARD int bits() const {
        return dtype_.bits;
    }

    /*! \return number of bytes to store each scalar. */
    NODISCARD int bytes() const {
        return (dtype_.bits + 7) / 8;
    }

    /*! \return number of lanes in the data. */
    NODISCARD int lanes() const {
        int lanes = static_cast<int16_t>(dtype_.lanes);
        if (lanes < 0) {
            LOG(FATAL) << "Can't fetch the lanes of a scalable vector at a compile time.";
        }
        return lanes;
    }

    /*! \return the integer multiplier of vscale in a scalable vector. */
    NODISCARD int vscale_factor() const {
        int lanes = static_cast<int16_t>(dtype_.lanes);
        if (lanes >= -1) {
            LOG(FATAL) << "A fixed length vector doesn't have a vscale factor.";
        }
        return -lanes;
    }

    /*! \return get vscale factor or lanes depending on scalability of the vector. */
    NODISCARD int get_lanes_or_vscale_factor() const {
        return is_scalable_vector() ? vscale_factor() : lanes();
    }

    /*! \return whether type is a scalar type. */
    NODISCARD bool is_scalar() const {
        return !is_scalable_vector() && lanes() == 1;
    }
    /*! \return whether type is a scalar type. */
    NODISCARD bool is_bool() const {
        return code() == static_cast<int>(TypeCode::kUInt) && bits() == 1;
    }
    /*! \return whether type is a float type. */
    NODISCARD bool is_float() const {
        return code() == static_cast<int>(TypeCode::kFloat);
    }
    /*! \return whether type is a float8 type. */
    NODISCARD bool is_float8() const {
        return (code() == static_cast<int>(TypeCode::kFloat) ||
                code() == static_cast<int>(TypeCode::kFloat8_e4m3fn) ||
                code() == static_cast<int>(TypeCode::kFloat8_e5m2)) &&
               bits() == 8;
    }

    NODISCARD bool is_e4m3_float8() const {
        return code() == static_cast<int>(TypeCode::kFloat8_e4m3fn) && bits() == 8;
    }

    NODISCARD bool is_e5m2_float8() const {
        return code() == static_cast<int>(TypeCode::kFloat8_e5m2) && bits() == 8;
    }

    /*! \return whether type is a float4 type. */
    bool is_float4() const { return code() == static_cast<int>(TypeCode::kFloat4_e2m1fn) && bits() == 4; }
    bool is_float8_e4m3fn() const { return code() == static_cast<int>(TypeCode::kFloat8_e4m3fn) && bits() == 8; }
    bool is_float8_e5m2() const { return code() == static_cast<int>(TypeCode::kFloat8_e5m2) && bits() == 8; }
    bool is_float4_e2m1fn() const { return code() == static_cast<int>(TypeCode::kFloat4_e2m1fn) && bits() == 4; }

    /*! \return whether type is a bfloat type. */
    bool is_bfloat() const { return code() == static_cast<int>(TypeCode::kBFloat); }

    /*! \return whether type is a float16 type. */
    NODISCARD bool is_float16() const {
        return is_float() && bits() == 16;
    }
    /*! \return whether type is a bfloat16 type. */
    NODISCARD bool is_bfloat16() const {
        return code() == static_cast<int>(TypeCode::kBFloat) && bits() == 16;
    }

    /*! \return whether type is an int type. */
    NODISCARD bool is_int() const {
        return code() == (int) TypeCode::kInt;
    }

    /*! \return whether type is an uint type. */
    NODISCARD bool is_uint() const {
        return code() == static_cast<int>(TypeCode::kUInt);
    }

    /*! \return whether type is a handle type. */
    NODISCARD bool is_handle() const {
        return code() == static_cast<int>(TypeCode::kHandle) && !is_void();
    }

    /*! \return whether type is a vector type. */
    NODISCARD bool is_scalable_or_fixed_length_vector() const {
        int encoded_lanes = static_cast<int16_t>(dtype_.lanes);
        return encoded_lanes < -1 || 1 < encoded_lanes;
    }

    /*! \return Whether the type is a fixed length vector. */
    NODISCARD bool is_fixed_length_vector() const {
        return static_cast<int16_t>(dtype_.lanes) > 1;
    }

    /*! \return Whether the type is a scalable vector. */
    NODISCARD bool is_scalable_vector() const {
        return static_cast<int16_t>(dtype_.lanes) < -1;
    }

    /*! \return whether type is a vector type. */
    NODISCARD bool is_vector() const {
        return lanes() > 1;
    }

    /*! \return whether type is a bool vector type. */
    NODISCARD bool is_vector_bool() const {
        return is_scalable_or_fixed_length_vector() && bits() == 1;
    }

    /*! \return whether type is a Void type. */
    NODISCARD bool is_void() const {
        return code() == static_cast<int>(TypeCode::kHandle) && bits() == 0 && lanes() == 0;
    }

    /*!
   * \brief Create a new data type by change lanes to a specified value.
   * \param lanes The target number of lanes.
   * \return the result type.
   */
    NODISCARD DataType with_lanes(int lanes) const {
        return {code(), bits(), lanes};
    }

    /*!
   * \brief Create a new scalable vector data type by changing the vscale multiplier to a specified
   * value. We'll use the data_.lanes field for this value.
   * \param vscale_factor The vscale multiplier.
   * \return A copy of the old DataType with the number of scalable lanes.
   */
    NODISCARD DataType with_scalable_vscale_factor(int vscale_factor) const {
        return {code(), bits(), -vscale_factor};
    }

    /*!
   * \brief Create a new data type by change bits to a specified value.
   * \param bits The target number of bits.
   * \return the result type.
   */
    NODISCARD DataType with_bits(int bits) const {
        return {code(), bits, dtype_.lanes};
    }

    /*!
   * \brief Get the scalar version of the type.
   * \return the result type.
   */
    NODISCARD DataType element_of() const {
        return with_lanes(1);
    }

    /*!
   * \brief Assignment operator.
   */
    DataType& operator=(const DataType& rhs) {
        DataType tmp(rhs);
        std::swap(tmp, *this);
        return *this;
    }

    /*!
   * \brief Equal comparator.
   * \param other The data type to compare against.
   * \return The comparison result.
   */
    bool operator==(const DataType& other) const {
        return dtype_.code == other.dtype_.code &&
               dtype_.bits == other.dtype_.bits &&
               dtype_.lanes == other.dtype_.lanes;
    }

    /*!
   * \brief NotEqual comparator.
   * \param other The data type to compare against.
   * \return The comparison result.
   */
    bool operator!=(const DataType& other) const {
        // return !operator==(other);
        return !(*this == other);
    }

    /*!
     * \brief Construct a Void type.
     * \return The constructed data type.
     */
    static DataType Void() {
        return {static_cast<uint8_t>(TypeCode::kHandle), 0, 0};
    }

    /*!
     * \brief Converter to DLDataType
     * \return the result.
     */
    operator DLDataType() const {
        return dtype_;
    }

    /*!
   * \brief Construct an int type.
   * \param bits The number of bits in the type.
   * \param lanes The number of lanes.
   * \return The constructed data type.
   */
    static DataType Int(int bits, int lanes = 1) {
        return {static_cast<uint8_t>(TypeCode::kInt), bits, lanes};
    }

    /*!
   * \brief Construct an uint type.
   * \param bits The number of bits in the type.
   * \param lanes The number of lanes.
   * \param is_scalable Whether the data type is scalable.
   * \return The constructed data type.
   */
    static DataType UInt(int bits, int lanes = 1, bool is_scalable = false) {
        return {static_cast<uint8_t>(TypeCode::kUInt), bits, lanes, is_scalable};
    }

    /*!
   * \brief Construct an float type.
   * \param bits The number of bits in the type.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Float(int bits, int lanes = 1) {
        return {static_cast<uint8_t>(TypeCode::kFloat), bits, lanes};
    }

    /*!
   * \brief Construct an bfloat type.
   * \param bits The number of bits in the type.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType BFloat(int bits, int lanes = 1) {
        return {static_cast<uint8_t>(TypeCode::kBFloat), bits, lanes};
    }

    /*!
   * \brief Construct NV float8 e4m3 datatype.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType NVFloat8E4M3(int lanes = 1) {
        return {static_cast<uint8_t>(TypeCode::kFloat8_e4m3fn), 8, lanes};
    }

    /*!
   * \brief Construct NV float8 e5m2 datatype.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType NVFloat8E5M2(int lanes = 1) {
        return {static_cast<uint8_t>(TypeCode::kFloat8_e5m2), 8, lanes};
    }

    /*!
   * \brief Construct a bool type.
   * \param lanes The number of lanes.
   * \param is_scalable Whether the data type is scalable.
   * \return The constructed data type.
   */
    static DataType Bool(int lanes = 1, bool is_scalable = false) {
        return UInt(1, lanes, is_scalable);
    }

    /*!
   * \brief Construct a handle type.
   * \param bits The number of bits in the type.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Handle(int bits = 64, int lanes = 1) {
        return {static_cast<uint8_t>(TypeCode::kHandle), bits, lanes};
    }

    /*!
   * \brief Get the corresponding type of TVMShapeIndex.
   * \return The type of TVM shape index.
   */
    static DataType ShapeIndex() {
        if (std::is_signed_v<tvm_index_t>) {
            return Int(sizeof(tvm_index_t) * 8);
        } else {
            return UInt(sizeof(tvm_index_t) * 8);
        }
    }

private:
    DLDataType dtype_{};
};

/*!
 * \brief Get the number of bytes needed in a vector.
 * \param dtype The data type.
 * \return Number of bytes needed.
 */
inline int GetVectorBytes(DataType dtype) {
    if (dtype == DataType::Bool() ||
        dtype == DataType::Int(1) ||
        dtype == DataType::Int(4) ||
        dtype == DataType::UInt(4)) {
        return 1;
    }

    int bits = dtype.bits() * dtype.lanes();
    CHECK_EQ(bits % 8, 0U) << "Need to load/store by multiple of bytes";
    return bits / 8;
}

/*!
 * \brief Check whether type matches the given spec.
 * \param t The type
 * \param code The type code.
 * \param bits The number of bits to be matched.
 * \param lanes The number of lanes in the type.
 */
inline bool TypeMatch(DLDataType t, int code, int bits, int lanes = 1) {
    return t.code == code && t.bits == bits && t.lanes == lanes;
}

/*!
 * \brief Check whether two types are equal .
 * \param lhs The left operand.
 * \param rhs The right operand.
 */
inline bool TypeEqual(DLDataType lhs, DLDataType rhs) {
    return lhs.code == rhs.code && lhs.bits == rhs.bits && lhs.lanes == rhs.lanes;
}

/*!
 * \brief Runtime utility for getting custom type name from code
 * \param type_code Custom type code
 * \return Custom type name
 */
std::string GetCustomTypeName(uint8_t type_code);

/*!
 * \brief Runtime utility for checking whether custom type is registered
 * \param type_code Custom type code
 * \return Bool representing whether type is registered
 */
bool GetCustomTypeRegistered(uint8_t type_code);

/*!
 * \brief Runtime utility for parsing string of the form "custom[<typename>]"
 * \param s String to parse
 * \param scan pointer to parsing pointer, which is scanning across s
 * \return type code of custom type parsed
 */
uint8_t ParseCustomDatatype(const std::string& s, const char** scan);

/*!
 * \brief Convert type code to its name
 * \param type_code The type code .
 * \return The name of type code.
 */
inline const char* DLDataTypeCode2Str(DLDataTypeCode type_code);

/*!
 * \brief convert a string to TVM type.
 * \param s The string to be converted.
 * \return The corresponding tvm type.
 */
inline DLDataType String2DLDataType(const std::string& s);

/*!
 * \brief convert a TVM type to string.
 * \param t The type to be converted.
 * \return The corresponding tvm type in string.
 */
inline std::string DLDataType2String(DLDataType t);

// implementation details
inline const char* DLDataTypeCode2Str(DLDataTypeCode type_code) {
    switch (static_cast<int>(type_code)) {
        case static_cast<int>(DLDataTypeCode::kDLInt):
            return "int";
        case static_cast<int>(DLDataTypeCode::kDLUInt):
            return "uint";
        case static_cast<int>(DLDataTypeCode::kDLFloat):
            return "float";
        case static_cast<int>(DataType::TypeCode::kHandle):
            return "handle";
        case static_cast<int>(DLDataTypeCode::kDLBfloat):
            return "bfloat";
        case static_cast<int>(DataType::TypeCode::kFloat8_e4m3fn):
            return "e4m3_float";
        case static_cast<int>(DataType::TypeCode::kFloat8_e5m2):
            return "e5m2_float";
        default:
            LOG(FATAL) << "unknown type_code=" << static_cast<int>(type_code);
    }
    throw;
}

inline std::ostream& operator<<(std::ostream& os, DLDataType t) {// NOLINT(*)
    if (t.bits == 1 && t.lanes == 1 && t.code == static_cast<int>(DLDataTypeCode::kDLUInt)) {
        os << "bool";
        return os;
    }
    if (DataType(t).is_void()) {
        return os << "void";
    }
    if (t.code < static_cast<uint8_t>(DataType::TypeCode::kCustomBegin)) {
        os << DLDataTypeCode2Str(static_cast<DLDataTypeCode>(t.code));
    } else {
        os << "custom[" << GetCustomTypeName(t.code) << "]";
    }

    if (t.code == static_cast<uint8_t>(TVMArgTypeCode::kTVMOpaqueHandle))
        return os;

    auto lanes = static_cast<int16_t>(t.lanes);
    os << static_cast<int>(t.bits);
    if (lanes > 1) {
        os << 'x' << lanes;
    } else if (lanes < -1) {
        os << "xvscalex" << -lanes;
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const DataType& dtype) {// NOLINT(*)
    return os << dtype.operator DLDataType();
}

inline std::string DLDataType2String(DLDataType t) {
    if (t.bits == 0) return "";
    std::ostringstream os;
    os << t;
    return os.str();
}

inline DLDataType String2DLDataType(const std::string& s) {
    DLDataType t;
    // handle void type
    if (s.empty() || s == "void") {
        t = DataType::Void();
        return t;
    }
    t.bits = 32;
    t.lanes = 1;
    const char* scan;
    if (s.substr(0, 3) == "int") {
        t.code = static_cast<uint8_t>(DLDataTypeCode::kDLInt);
        scan = s.c_str() + 3;
    } else if (s.substr(0, 4) == "uint") {
        t.code = static_cast<uint8_t>(DLDataTypeCode::kDLUInt);
        scan = s.c_str() + 4;
    } else if (s.substr(0, 5) == "float") {
        t.code = static_cast<uint8_t>(DLDataTypeCode::kDLFloat);
        scan = s.c_str() + 5;
    } else if (s.substr(0, 6) == "handle") {
        t.code = static_cast<uint8_t>(TVMArgTypeCode::kTVMOpaqueHandle);
        t.bits = 64;// handle uses 64 bit by default.
        scan = s.c_str() + 6;
    } else if (s == "bool") {
        t.code = static_cast<uint8_t>(DLDataTypeCode::kDLUInt);
        t.bits = 1;
        t.lanes = 1;
        return t;
    } else if (s.substr(0, 6) == "bfloat") {
        t.code = static_cast<uint8_t>(DataType::TypeCode::kBFloat);
        t.bits = 16;
        scan = s.c_str() + 6;
    } else if (s.substr(0, 10) == "e4m3_float") {
        t.code = static_cast<uint8_t>(DataType::TypeCode::kFloat8_e4m3fn);
        t.bits = 8;
        scan = s.c_str() + 10;
    } else if (s.substr(0, 10) == "e5m2_float") {
        t.code = static_cast<uint8_t>(DataType::TypeCode::kFloat8_e5m2);
        t.bits = 8;
        scan = s.c_str() + 10;
    } else if (s.substr(0, 6) == "custom") {
        t.code = ParseCustomDatatype(s, &scan);
    } else {
        scan = s.c_str();
        LOG(FATAL) << "unknown type " << s;
    }
    char* xdelim;// emulate sscanf("%ux%u", bits, lanes)
    uint8_t bits = static_cast<uint8_t>(strtoul(scan, &xdelim, 10));
    if (bits != 0) t.bits = bits;
    int scalable_multiplier = 1;
    if (strncmp(xdelim, "xvscale", 7) == 0) {
        scalable_multiplier = -1;
        xdelim += 7;
    }
    char* endpt = xdelim;
    if (*xdelim == 'x') {
        t.lanes = static_cast<uint16_t>(scalable_multiplier * strtoul(xdelim + 1, &endpt, 10));
    }
    CHECK(endpt == s.c_str() + s.length()) << "unknown type " << s;
    return t;
}

}// namespace litetvm::runtime


#endif//LITETVM_RUNTIME_DATA_TYPE_H
