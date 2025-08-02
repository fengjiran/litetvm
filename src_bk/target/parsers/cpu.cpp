//
// Created by 赵丹 on 25-3-18.
//

#include "target/parsers/cpu.h"
#include "target/parsers/aprofile.h"
#include "target/parsers/mprofile.h"

namespace litetvm {
namespace target {
namespace parsers {
namespace cpu {

Optional<String> DetectSystemTriple() {
#ifdef TVM_LLVM_VERSION
    auto pf = tvm::runtime::Registry::Get("target.llvm_get_system_triple");
    ICHECK(pf != nullptr) << "The target llvm_get_system_triple was not found, "
                             "please compile with USE_LLVM = ON";
    return (*pf)();
#endif
    return {};
}

TargetJSON ParseTarget(TargetJSON target) {
    String kind = Downcast<String>(target.Get("kind"));
    Optional<String> mtriple = Downcast<Optional<String>>(target.Get("mtriple"));
    Optional<String> mcpu = Downcast<Optional<String>>(target.Get("mcpu"));

    // Try to fill in the blanks by detecting target information from the system
    if (kind == "llvm" && !mtriple.defined() && !mcpu.defined()) {
        String system_triple = DetectSystemTriple().value_or("");
        target.Set("mtriple", system_triple);
    }

    if (mprofile::IsArch(target)) {
        return mprofile::ParseTarget(target);
    }

    if (aprofile::IsArch(target)) {
        return aprofile::ParseTarget(target);
    }

    return target;
}

}
}
}
}
