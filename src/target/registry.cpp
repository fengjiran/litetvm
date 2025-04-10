//
// Created by 赵丹 on 25-4-10.
//

#include "target/registry.h"

namespace litetvm {
namespace datatype {

using runtime::TVMArgs;
using runtime::TVMRetValue;
using DataType = runtime::DataType;

TVM_REGISTER_GLOBAL("runtime._datatype_register").set_body([](TVMArgs args, TVMRetValue* ret) {
    Registry::Global()->Register(args[0], static_cast<uint8_t>(args[1].operator int()));
});

TVM_REGISTER_GLOBAL("runtime._datatype_get_type_code").set_body([](TVMArgs args, TVMRetValue* ret) {
    *ret = Registry::Global()->GetTypeCode(args[0]);
});

TVM_REGISTER_GLOBAL("runtime._datatype_get_type_name").set_body([](TVMArgs args, TVMRetValue* ret) {
    *ret = Registry::Global()->GetTypeName(args[0].operator int());
});

TVM_REGISTER_GLOBAL("runtime._datatype_get_type_registered")
        .set_body([](TVMArgs args, TVMRetValue* ret) {
            *ret = Registry::Global()->GetTypeRegistered(args[0].operator int());
        });

Registry* Registry::Global() {
    static Registry inst;
    return &inst;
}

void Registry::Register(const std::string& type_name, uint8_t type_code) {
    CHECK(type_code >= static_cast<int>(DataType::TypeCode::kCustomBegin))
            << "Please choose a type code >= DataType::kCustomBegin for custom types";
    code_to_name_[type_code] = type_name;
    name_to_code_[type_name] = type_code;
}

uint8_t Registry::GetTypeCode(const std::string& type_name) {
    CHECK(name_to_code_.find(type_name) != name_to_code_.end())
            << "Type name " << type_name << " not registered";
    return name_to_code_[type_name];
}

std::string Registry::GetTypeName(uint8_t type_code) {
    CHECK(code_to_name_.find(type_code) != code_to_name_.end())
            << "Type code " << static_cast<unsigned>(type_code) << " not registered";
    return code_to_name_[type_code];
}

const runtime::PackedFunc* GetCastLowerFunc(const std::string& target, uint8_t type_code,
                                            uint8_t src_type_code) {
    std::ostringstream ss;
    ss << "tvm.datatype.lower.";
    ss << target << ".";
    ss << "Cast"
       << ".";

    if (Registry::Global()->GetTypeRegistered(type_code)) {
        ss << Registry::Global()->GetTypeName(type_code);
    } else {
        ss << runtime::DLDataTypeCode2Str(static_cast<DLDataTypeCode>(type_code));
    }

    ss << ".";

    if (Registry::Global()->GetTypeRegistered(src_type_code)) {
        ss << Registry::Global()->GetTypeName(src_type_code);
    } else {
        ss << runtime::DLDataTypeCode2Str(static_cast<DLDataTypeCode>(src_type_code));
    }
    return runtime::RegistryManager::Global().Get(ss.str());
}

const runtime::PackedFunc* GetMinFunc(uint8_t type_code) {
    std::ostringstream ss;
    ss << "tvm.datatype.min.";
    ss << Registry::Global()->GetTypeName(type_code);
    return runtime::RegistryManager::Global().Get(ss.str());
}

const runtime::PackedFunc* GetFloatImmLowerFunc(const std::string& target, uint8_t type_code) {
    std::ostringstream ss;
    ss << "tvm.datatype.lower.";
    ss << target;
    ss << ".FloatImm.";
    ss << Registry::Global()->GetTypeName(type_code);
    return runtime::RegistryManager::Global().Get(ss.str());
}

const runtime::PackedFunc* GetIntrinLowerFunc(const std::string& target, const std::string& name,
                                              uint8_t type_code) {
    std::ostringstream ss;
    ss << "tvm.datatype.lower.";
    ss << target;
    ss << ".Call.intrin.";
    ss << name;
    ss << ".";
    ss << Registry::Global()->GetTypeName(type_code);
    return runtime::RegistryManager::Global().Get(ss.str());
}

uint64_t ConvertConstScalar(uint8_t type_code, double value) {
    std::ostringstream ss;
    ss << "tvm.datatype.convertconstscalar.float.";
    ss << Registry::Global()->GetTypeName(type_code);
    auto make_const_scalar_func = runtime::RegistryManager::Global().Get(ss.str());
    return (*make_const_scalar_func)(value).operator uint64_t();
}
}// namespace datatype
}// namespace litetvm
