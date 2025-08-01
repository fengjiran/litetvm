//
// Created by 赵丹 on 25-8-1.
//

#ifndef LITETVM_RUNTIME_BUILTIN_FP16_H
#define LITETVM_RUNTIME_BUILTIN_FP16_H

#include "runtime/base.h"

#include <cstdint>

extern "C" {
TVM_DLL uint16_t __gnu_f2h_ieee(float);
TVM_DLL float __gnu_h2f_ieee(uint16_t);
TVM_DLL uint16_t __truncsfhf2(float v);
TVM_DLL uint16_t __truncdfhf2(double v);
TVM_DLL float __extendhfsf2(uint16_t v);
}

#endif//LITETVM_RUNTIME_BUILTIN_FP16_H
