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

namespace litetvm {
namespace runtime {

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
    enum TypeCode {
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

        if (code == kBFloat) {
            ICHECK_EQ(bits, 16);
        }

        if (code == kFloat8_e3m4 || code == kFloat8_e4m3 || code == kFloat8_e4m3b11fnuz ||
            code == kFloat8_e4m3fn || code == kFloat8_e4m3fnuz || code == kFloat8_e5m2 ||
            code == kFloat8_e5m2fnuz || code == kFloat8_e8m0fnu) {
            ICHECK_EQ(bits, 8);
        }

        if (code == kFloat6_e2m3fn || code == kFloat6_e3m2fn) {
            ICHECK_EQ(bits, 6);
        }

        if (code == kFloat4_e2m1fn) {
            ICHECK_EQ(bits, 4);
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
        return code() == kUInt && bits() == 1;
    }

    /*! \return whether type is a float type. */
    NODISCARD bool is_float() const {
        return code() == kFloat;
    }

    NODISCARD bool is_bfloat() const {
        return code() == kBFloat;
    }

    /*! \return whether type is a float8 type. */
    NODISCARD bool is_float8() const {
        return bits() == 8 &&
               (code() == kFloat8_e3m4 || code() == kFloat8_e4m3 ||
                code() == kFloat8_e4m3b11fnuz || code() == kFloat8_e4m3fn ||
                code() == kFloat8_e4m3fnuz || code() == kFloat8_e5m2 ||
                code() == kFloat8_e5m2fnuz || code() == kFloat8_e8m0fnu);
    }

    NODISCARD bool is_float6() const {
        return bits() == 6 && (code() == kFloat6_e2m3fn || code() == kFloat6_e3m2fn);
    }

    NODISCARD bool is_float4() const {
        return bits() == 4 && code() == kFloat4_e2m1fn;
    }

    NODISCARD bool is_float8_e3m4() const {
        return bits() == 8 && code() == kFloat8_e3m4;
    }

    NODISCARD bool is_float8_e4m3() const {
        return bits() == 8 && code() == kFloat8_e4m3;
    }

    NODISCARD bool is_float8_e4m3b11fnuz() const {
        return bits() == 8 && code() == kFloat8_e4m3b11fnuz;
    }

    NODISCARD bool is_float8_e4m3fn() const {
        return bits() == 8 && code() == kFloat8_e4m3fn;
    }

    NODISCARD bool is_float8_e4m3fnuz() const {
        return bits() == 8 && code() == kFloat8_e4m3fnuz;
    }

    NODISCARD bool is_float8_e5m2() const {
        return bits() == 8 && code() == kFloat8_e5m2;
    }

    NODISCARD bool is_float8_e5m2fnuz() const {
        return bits() == 8 && code() == kFloat8_e5m2fnuz;
    }

    NODISCARD bool is_float8_e8m0fnu() const {
        return bits() == 8 && code() == kFloat8_e8m0fnu;
    }

    NODISCARD bool is_float6_e2m3fn() const {
        return bits() == 6 && code() == kFloat6_e2m3fn;
    }

    NODISCARD bool is_float6_e3m2fn() const {
        return bits() == 6 && code() == kFloat6_e3m2fn;
    }

    NODISCARD bool is_float4_e2m1fn() const {
        return code() == kFloat4_e2m1fn && bits() == 4;
    }

    /*! \return whether type is a float16 type. */
    NODISCARD bool is_float16() const {
        return is_float() && bits() == 16;
    }
    /*! \return whether type is a bfloat16 type. */
    NODISCARD bool is_bfloat16() const {
        return is_bfloat() && bits() == 16;
    }

    /*! \return whether type is an int type. */
    NODISCARD bool is_int() const {
        return code() == kInt;
    }

    /*! \return whether type is an uint type. */
    NODISCARD bool is_uint() const {
        return code() == kUInt;
    }

    /*! \return whether type is a handle type. */
    NODISCARD bool is_handle() const {
        return code() == kHandle && !is_void();
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
        return code() == kHandle && bits() == 0 && lanes() == 0;
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
        return {kHandle, 0, 0};
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
        return {kInt, bits, lanes};
    }

    /*!
   * \brief Construct an uint type.
   * \param bits The number of bits in the type.
   * \param lanes The number of lanes.
   * \param is_scalable Whether the data type is scalable.
   * \return The constructed data type.
   */
    static DataType UInt(int bits, int lanes = 1, bool is_scalable = false) {
        return {kUInt, bits, lanes, is_scalable};
    }

    /*!
   * \brief Construct an float type.
   * \param bits The number of bits in the type.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Float(int bits, int lanes = 1) {
        return {kFloat, bits, lanes};
    }

    /*!
   * \brief Construct an bfloat type.
   * \param bits The number of bits in the type.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType BFloat(int bits, int lanes = 1) {
        return {kBFloat, bits, lanes};
    }

    /*!
   * \brief Construct float8 e3m4 datatype.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Float8E3M4(int lanes = 1) {
        return {kFloat8_e3m4, 8, lanes};
    }

    /*!
   * \brief Construct float8 e4m3 datatype.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Float8E4M3(int lanes = 1) {
        return {kFloat8_e4m3, 8, lanes};
    }

    /*!
   * \brief Construct float8 e4m3b11fnuz datatype.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Float8E4M3B11FNUZ(int lanes = 1) {
        return {kFloat8_e4m3b11fnuz, 8, lanes};
    }

    /*!
   * \brief Construct float8 e4m3fn datatype.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Float8E4M3FN(int lanes = 1) {
        return {kFloat8_e4m3fn, 8, lanes};
    }

    /*!
   * \brief Construct float8 e4m3fnuz datatype.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Float8E4M3FNUZ(int lanes = 1) {
        return {kFloat8_e4m3fnuz, 8, lanes};
    }

    /*!
     * \brief Construct float8 e5m2 datatype.
     * \param lanes The number of lanes
     * \return The constructed data type.
     */
    static DataType Float8E5M2(int lanes = 1) {
        return {kFloat8_e5m2, 8, lanes};
    }

    /*!
   * \brief Construct float8 e5m2fnuz datatype.
   * \param lanes The number of lanes
   * \return The constructed data type.
   */
    static DataType Float8E5M2FNUZ(int lanes = 1) {
        return {kFloat8_e5m2fnuz, 8, lanes};
    }

    /*!
     * \brief Construct float8 e8m0fnu datatype.
     * \param lanes The number of lanes
     * \return The constructed data type.
     */
    static DataType Float8E8M0FNU(int lanes = 1) {
        return {kFloat8_e8m0fnu, 8, lanes};
    }

    /*!
     * \brief Construct float6 e2m3fn datatype.
     * \param lanes The number of lanes
     * \return The constructed data type.
     */
    static DataType Float6E2M3FN(int lanes = 1) {
        return {kFloat6_e2m3fn, 6, lanes};
    }

    /*!
     * \brief Construct float6 e3m2fn datatype.
     * \param lanes The number of lanes
     * \return The constructed data type.
     */
    static DataType Float6E3M2FN(int lanes = 1) {
        return {kFloat6_e3m2fn, 6, lanes};
    }

    /*!
     * \brief Construct float4 e2m1fn datatype.
     * \param lanes The number of lanes
     * \return The constructed data type.
     */
    static DataType Float4E2M1FN(int lanes = 1) {
        return {kFloat4_e2m1fn, 4, lanes};
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
        return {kHandle, bits, lanes};
    }

    /*!
   * \brief Get the corresponding type of TVMShapeIndex.
   * \return The type of TVM shape index.
   */
    static DataType ShapeIndex() {
        if (std::is_signed_v<tvm_index_t>) {
            return Int(sizeof(tvm_index_t) * 8);
        }
        return UInt(sizeof(tvm_index_t) * 8);
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
    // allow bool to exist
    if (dtype == DataType::Bool() || dtype == DataType::Int(4) || dtype == DataType::UInt(4) ||
        dtype == DataType::Int(1) || dtype == DataType::Float4E2M1FN() ||
        dtype == DataType::Float6E2M3FN() || dtype == DataType::Float6E3M2FN()) {
        return 1;
    }

    int bits = dtype.bits() * dtype.lanes();
    ICHECK_EQ(bits % 8, 0U) << "Need to load/store by multiple of bytes";
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

using ffi::DLDataTypeToString;
using ffi::StringToDLDataType;

inline std::ostream& operator<<(std::ostream& os, const DataType& dtype) {// NOLINT(*)
    return os << dtype.operator DLDataType();
}

}// namespace runtime

using DataType = runtime::DataType;

namespace ffi {

template<>
struct TypeTraits<DataType> : TypeTraitsBase {
    static constexpr int32_t field_static_type_index = kTVMFFIDataType;

    TVM_FFI_INLINE static void CopyToAnyView(const DataType& src, TVMFFIAny* result) {
        // clear padding part to ensure the equality check can always check the v_uint64 part
        result->v_uint64 = 0;
        result->type_index = kTVMFFIDataType;
        result->v_dtype = src;
    }

    TVM_FFI_INLINE static void MoveToAny(DataType src, TVMFFIAny* result) {
        // clear padding part to ensure the equality check can always check the v_uint64 part
        result->v_uint64 = 0;
        result->type_index = kTVMFFIDataType;
        result->v_dtype = src;
    }

    TVM_FFI_INLINE static std::optional<DataType> TryCastFromAnyView(const TVMFFIAny* src) {
        auto opt_dtype = TypeTraits<DLDataType>::TryCastFromAnyView(src);
        if (opt_dtype) {
            return DataType(opt_dtype.value());
        }
        return std::nullopt;
    }

    TVM_FFI_INLINE static bool CheckAnyStrict(const TVMFFIAny* src) {
        return TypeTraits<DLDataType>::CheckAnyStrict(src);
    }

    TVM_FFI_INLINE static DataType CopyFromAnyViewAfterCheck(const TVMFFIAny* src) {
        return DataType(TypeTraits<DLDataType>::CopyFromAnyViewAfterCheck(src));
    }

    TVM_FFI_INLINE static std::string TypeStr() {
        return StaticTypeKey::kTVMFFIDataType;
    }
};

}// namespace ffi
}// namespace litetvm

namespace std {
template<>
struct hash<litetvm::DataType> {
    NODISCARD int cantor_pairing_function(int a, int b) const {
        return (a + b) * (a + b + 1) / 2 + b;
    }

    std::size_t operator()(litetvm::DataType const& dtype) const {
        int a = dtype.code();
        int b = dtype.bits();
        int c = dtype.lanes();
        int d = cantor_pairing_function(a, b);
        return cantor_pairing_function(c, d);
    }
};
}// namespace std

#endif//LITETVM_RUNTIME_DATA_TYPE_H
