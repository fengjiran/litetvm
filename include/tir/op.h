//
// Created by 赵丹 on 25-3-24.
//

#ifndef LITETVM_TIR_OP_H
#define LITETVM_TIR_OP_H

#include "ir/expr.h"
#include "ir/op.h"
#include "ir/type.h"

#include "tir/expr.h"

#include <limits>
#include <type_traits>

namespace litetvm {

#define TVM_TIR_REGISTER_OP(OpName) \
    TVM_REGISTER_OP("tir." OpName).set_attr<TScriptPrinterName>("TScriptPrinterName", OpName)


}// namespace litetvm

#endif//LITETVM_TIR_OP_H
