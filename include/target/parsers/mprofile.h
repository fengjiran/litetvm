//
// Created by 赵丹 on 25-3-18.
//

#ifndef LITETVM_TARGET_PARSERS_MPROFILE_H
#define LITETVM_TARGET_PARSERS_MPROFILE_H

#include "target/target_kind.h"

namespace litetvm {
namespace target {
namespace parsers {
namespace mprofile {

bool IsArch(TargetJSON target);
TargetJSON ParseTarget(TargetJSON target);

}
}
}
}

#endif //LITETVM_TARGET_PARSERS_MPROFILE_H
