//
// Created by 赵丹 on 25-4-3.
//

#include "tir/buffer.h"
#include "runtime/device_api.h"
#include "runtime/registry.h"
#include "tir/builtin.h"
#include "tir/expr.h"
#include "tir/op.h"

#include <iterator>
#include <stack>

namespace litetvm {
namespace tir {

using IndexMod = tir::FloorModNode;
using IndexDiv = tir::FloorDivNode;

Buffer decl_buffer(Array<PrimExpr> shape, DataType dtype, String name, String storage_scope,
                   Array<IntImm> axis_separators) {
    DataType storage_dtype = (dtype == DataType::Bool() ? DataType::Int(8) : dtype);
    return Buffer(Var(name, PointerType(PrimType(storage_dtype), storage_scope)), dtype, shape,
                  Array<PrimExpr>(), PrimExpr(), name, 0, 0, BufferType::kDefault, axis_separators);
}

}// namespace tir
}// namespace litetvm