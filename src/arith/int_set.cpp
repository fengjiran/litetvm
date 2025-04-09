//
// Created by richard on 4/5/25.
//

#include "arith/int_set.h"
#include "arith/interval_set.h"
#include "runtime/registry.h"
#include "tir/expr.h"
#include "tir/expr_functor.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace litetvm {
namespace arith {

using tir::is_one;
using tir::is_zero;
using tir::make_const;
using tir::make_zero;

}
}