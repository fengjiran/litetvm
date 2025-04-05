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



}
}// namespace litetvm