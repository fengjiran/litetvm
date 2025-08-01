//
// Created by richard on 5/15/25.
//

#ifndef LITETVM_FFI_DTYPE_H
#define LITETVM_FFI_DTYPE_H

#include "ffi/error.h"
#include "ffi/function.h"
#include "ffi/string.h"
#include "ffi/type_traits.h"

#include <dlpack/dlpack.h>
#include <string>

namespace litetvm {
namespace ffi {
/*!
 * \brief Extension code beyond the DLDataType.
 *
 * This class is always consistent with the DLPack.
 *
 * TOTO(tvm-team): update to latest DLPack types.
 */
enum DLExtDataTypeCode { kDLExtCustomBegin = 129 };

namespace details {

/*
 * \brief Convert a DLDataTypeCode to a string.
 * \param os The output stream.
 * \param type_code The DLDataTypeCode to convert.
 */
inline const char* DLDataTypeCodeAsCStr(DLDataTypeCode type_code) {// NOLINT(*)
    switch (static_cast<int>(type_code)) {
        case kDLInt: {
            return "int";
        }
        case kDLUInt: {
            return "uint";
        }
        case kDLFloat: {
            return "float";
        }
        case kDLOpaqueHandle: {
            return "handle";
        }
        case kDLBfloat: {
            return "bfloat";
        }
        case kDLFloat8_e3m4: {
            return "float8_e3m4";
        }
        case kDLFloat8_e4m3: {
            return "float8_e4m3";
        }
        case kDLFloat8_e4m3b11fnuz: {
            return "float8_e4m3b11fnuz";
        }
        case kDLFloat8_e4m3fn: {
            return "float8_e4m3fn";
        }
        case kDLFloat8_e4m3fnuz: {
            return "float8_e4m3fnuz";
        }
        case kDLFloat8_e5m2: {
            return "float8_e5m2";
        }
        case kDLFloat8_e5m2fnuz: {
            return "float8_e5m2fnuz";
        }
        case kDLFloat8_e8m0fnu: {
            return "float8_e8m0fnu";
        }
        case kDLFloat6_e2m3fn: {
            return "float6_e2m3fn";
        }
        case kDLFloat6_e3m2fn: {
            return "float6_e3m2fn";
        }
        case kDLFloat4_e2m1fn: {
            return "float4_e2m1fn";
        }
        default: {
            if (static_cast<int>(type_code) >= static_cast<int>(kDLExtCustomBegin)) {
                return "custom";
            }
            TVM_FFI_THROW(ValueError) << "DLDataType contains unknown type_code=" << type_code;
            TVM_FFI_UNREACHABLE();
        }
    }
}
}// namespace details

inline DLDataType StringToDLDataType(const String& str) {
    DLDataType out;
    TVM_FFI_CHECK_SAFE_CALL(TVMFFIDataTypeFromString(str.get(), &out));
    return out;
}

inline String DLDataTypeToString(DLDataType dtype) {
    TVMFFIObjectHandle out;
    TVM_FFI_CHECK_SAFE_CALL(TVMFFIDataTypeToString(&dtype, &out));
    return String(details::ObjectUnsafe::ObjectPtrFromOwned<Object>(static_cast<TVMFFIObject*>(out)));
}

// DLDataType
template<>
struct TypeTraits<DLDataType> : TypeTraitsBase {
    static constexpr int32_t field_static_type_index = kTVMFFIDataType;

    TVM_FFI_INLINE static void CopyToAnyView(const DLDataType& src, TVMFFIAny* result) {
        // clear padding part to ensure the equality check can always check the v_uint64 part
        result->v_uint64 = 0;
        result->type_index = kTVMFFIDataType;
        result->v_dtype = src;
    }

    TVM_FFI_INLINE static void MoveToAny(DLDataType src, TVMFFIAny* result) {
        // clear padding part to ensure the equality check can always check the v_uint64 part
        result->v_uint64 = 0;
        result->type_index = kTVMFFIDataType;
        result->v_dtype = src;
    }

    TVM_FFI_INLINE static bool CheckAnyStrict(const TVMFFIAny* src) {
        return src->type_index == kTVMFFIDataType;
    }

    TVM_FFI_INLINE static DLDataType CopyFromAnyViewAfterCheck(const TVMFFIAny* src) {
        return src->v_dtype;
    }

    TVM_FFI_INLINE static std::optional<DLDataType> TryCastFromAnyView(const TVMFFIAny* src) {
        if (src->type_index == kTVMFFIDataType) {
            return src->v_dtype;
        }
        // enable string to dtype auto conversion
        if (auto opt_str = TypeTraits<std::string>::TryCastFromAnyView(src)) {
            return StringToDLDataType(*opt_str);
        }
        return std::nullopt;
    }

    TVM_FFI_INLINE static std::string TypeStr() {
        return StaticTypeKey::kTVMFFIDataType;
    }
};
}// namespace ffi
}// namespace litetvm

// define DLDataType comparison and printing in root namespace
inline std::ostream& operator<<(std::ostream& os, DLDataType dtype) {// NOLINT(*)
    return os << litetvm::ffi::DLDataTypeToString(dtype);
}

inline bool operator==(const DLDataType& lhs, const DLDataType& rhs) {
    return lhs.code == rhs.code && lhs.bits == rhs.bits && lhs.lanes == rhs.lanes;
}

inline bool operator!=(const DLDataType& lhs, const DLDataType& rhs) { return !(lhs == rhs); }

#endif//LITETVM_FFI_DTYPE_H
