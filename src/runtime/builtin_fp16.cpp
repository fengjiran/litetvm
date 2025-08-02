//
// Created by richard on 8/2/25.
//
#include "builtin_fp16.h"
#include "runtime/base.h"

extern "C" {

// disable under msvc
#ifndef _MSC_VER

TVM_DLL TVM_FFI_WEAK uint16_t __gnu_f2h_ieee(float a) {
    return __truncXfYf2__<float, uint32_t, 23, uint16_t, uint16_t, 10>(a);
}

TVM_DLL TVM_FFI_WEAK float __gnu_h2f_ieee(uint16_t a) {
    return __extendXfYf2__<uint16_t, uint16_t, 10, float, uint32_t, 23>(a);
}

TVM_DLL TVM_FFI_WEAK uint16_t __truncdfhf2(double a) {
    return __truncXfYf2__<double, uint64_t, 52, uint16_t, uint16_t, 10>(a);
}

#else

TVM_DLL uint16_t __gnu_f2h_ieee(float a) {
    return __truncXfYf2__<float, uint32_t, 23, uint16_t, uint16_t, 10>(a);
}

TVM_DLL float __gnu_h2f_ieee(uint16_t a) {
    return __extendXfYf2__<uint16_t, uint16_t, 10, float, uint32_t, 23>(a);
}

TVM_DLL uint16_t __truncdfhf2(double a) {
    return __truncXfYf2__<double, uint64_t, 52, uint16_t, uint16_t, 10>(a);
}

#endif
}