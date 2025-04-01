//
// Created by 赵丹 on 25-4-1.
//
#include "tir/builtin.h"
#include "runtime/registry.h"
#include "tir/op.h"
#include "tir/op_attr_types.h"

namespace litetvm {
namespace tir {
namespace builtin {

#define TIR_DEFINE_BUILTIN_FUNC(OpName)                \
    const Op& OpName() {                               \
        static const Op& op = Op::Get("tir." #OpName); \
        return op;                                     \
    }                                                  \
    TVM_TIR_REGISTER_OP(#OpName)

TIR_DEFINE_BUILTIN_FUNC(ret)
    .set_attr<TCallEffectKind>("TCallEffectKind", Integer(CallEffectKind::kControlJump))
    .set_num_inputs(1);

}// namespace builtin
}// namespace tir
}// namespace litetvm