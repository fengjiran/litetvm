//
// Created by 赵丹 on 25-3-20.
//

#include "ir/op.h"
#include "ir/type.h"
#include "runtime/module.h"
#include "runtime/packed_func.h"
#include "node/attr_registry.h"

#include <memory>

namespace litetvm {

using runtime::PackedFunc;
using runtime::TVMArgs;
using runtime::TVMRetValue;

using OpRegistry = AttrRegistry<OpRegEntry, Op>;



}