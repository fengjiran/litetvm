//
// Created by 赵丹 on 25-4-3.
//

#ifndef LITETVM_TIR_BUFFER_H
#define LITETVM_TIR_BUFFER_H

#include "ir/expr.h"
#include "node/script_printer.h"
#include "runtime/array.h"
#include "runtime/string.h"
#include "tir/var.h"

#include <string>

namespace litetvm {
namespace tir {

class Stmt;

#ifndef TVM_INDEX_DEFAULT_I64
#define TVM_INDEX_DEFAULT_I64 1
#endif
/*! \brief if TVM_INDEX_DEFAULT_I64 is set, return int64, otherwise return int32 */
inline DataType DefaultIndexType() {
#if TVM_INDEX_DEFAULT_I64
    return DataType::Int(64);
#else
    return DataType::Int(32);
#endif
}

/*! \brief buffer type */
enum class BufferType : int {
    kDefault = 1,
    // Maps buffer[i][j][k] -> buffer[i][0][k] if dimension i's shape equals 1.
    kAutoBroadcast = 2,
};



}// namespace tir
}// namespace litetvm

#endif//LITETVM_TIR_BUFFER_H
