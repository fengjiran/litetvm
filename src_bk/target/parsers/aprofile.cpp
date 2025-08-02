//
// Created by 赵丹 on 25-3-18.
//

#include "target/parsers/aprofile.h"
#include "node/repr_printer.h"
#include "support/utils.h"

#include <string>

namespace litetvm {
namespace target {
namespace parsers {
namespace aprofile {

double GetArchVersion(Array<String> mattr) {
    for (const String& attr: mattr) {
        std::string attr_string = attr;
        size_t attr_len = attr_string.size();
        if (attr_len >= 4 && attr_string.substr(0, 2) == "+v" && attr_string.back() == 'a') {
            std::string version_string = attr_string.substr(2, attr_string.size() - 2);
            return atof(version_string.data());
        }
    }
    return 0.0;
}

double GetArchVersion(Optional<Array<String>> attr) {
    if (!attr) {
        return false;
    }
    return GetArchVersion(attr.value());
}

bool IsAArch32(Optional<String> mtriple, Optional<String> mcpu) {
    if (mtriple) {
        bool is_mprofile = mcpu && support::StartsWith(mcpu.value(), "cortex-m");
        return support::StartsWith(mtriple.value(), "arm") && !is_mprofile;
    }
    return false;
}

bool IsAArch64(Optional<String> mtriple) {
    if (mtriple) {
        return support::StartsWith(mtriple.value(), "aarch64");
    }
    return false;
}

bool IsArch(TargetJSON attrs) {
    Optional<String> mtriple = Downcast<Optional<String>>(attrs.Get("mtriple"));
    Optional<String> mcpu = Downcast<Optional<String>>(attrs.Get("mcpu"));

    return IsAArch32(mtriple, mcpu) || IsAArch64(mtriple);
}

bool CheckContains(Array<String> array, String predicate) {
    return std::any_of(array.begin(), array.end(), [&](String var) { return var == predicate; });
}

static TargetFeatures GetFeatures(TargetJSON target) {
#ifdef TVM_LLVM_VERSION
    String kind = Downcast<String>(target.Get("kind"));
    ICHECK_EQ(kind, "llvm") << "Expected target kind 'llvm', but got '" << kind << "'";

    Optional<String> mtriple = Downcast<Optional<String>>(target.Get("mtriple"));
    Optional<String> mcpu = Downcast<Optional<String>>(target.Get("mcpu"));

    // Check that LLVM has been compiled with the correct target support
    auto llvm_instance = std::make_unique<codegen::LLVMInstance>();
    codegen::LLVMTargetInfo llvm_backend(*llvm_instance, {{"kind", String("llvm")}});
    Array<String> targets = llvm_backend.GetAllLLVMTargets();
    if ((IsAArch64(mtriple) && !CheckContains(targets, "aarch64")) ||
        (IsAArch32(mtriple, mcpu) && !CheckContains(targets, "arm"))) {
        LOG(WARNING) << "Cannot parse target features for target: " << target
                     << ". LLVM was not compiled with support for Arm(R)-based targets.";
        return {};
    }

    codegen::LLVMTargetInfo llvm_target(*llvm_instance, target);
    Map<String, String> features = llvm_target.GetAllLLVMCpuFeatures();

    auto has_feature = [features](const String& feature) {
        return features.find(feature) != features.end();
    };

    return {{"is_aarch64", Bool(IsAArch64(mtriple))},
            {"has_asimd", Bool(has_feature("neon"))},
            {"has_sve", Bool(has_feature("sve"))},
            {"has_dotprod", Bool(has_feature("dotprod"))},
            {"has_matmul_i8", Bool(has_feature("i8mm"))},
            {"has_fp16_simd", Bool(has_feature("fullfp16"))},
            {"has_sme", Bool(has_feature("sme"))}};
#endif

    LOG(WARNING) << "Cannot parse Arm(R)-based target features for target " << target
                 << " without LLVM support.";
    return {};
}

static Array<String> MergeKeys(Optional<Array<String>> existing_keys) {
    const Array<String> kExtraKeys = {"arm_cpu", "cpu"};

    if (!existing_keys) {
        return kExtraKeys;
    }

    Array<String> keys = existing_keys.value();
    for (String key: kExtraKeys) {
        if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
            keys.push_back(key);
        }
    }
    return keys;
}

TargetJSON ParseTarget(TargetJSON target) {
    target.Set("features", GetFeatures(target));
    target.Set("keys", MergeKeys(Downcast<Optional<Array<String>>>(target.Get("keys"))));

    return target;
}

}// namespace aprofile
}// namespace parsers
}// namespace target
}// namespace litetvm