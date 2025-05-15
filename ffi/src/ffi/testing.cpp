//
// Created by richard on 5/15/25.
//
#include "ffi/function.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace litetvm {
namespace ffi {

void TestRaiseError(String kind, String msg) {
    throw ffi::Error(kind, msg, TVM_FFI_TRACEBACK_HERE);
}

TVM_FFI_REGISTER_GLOBAL("testing.test_raise_error").set_body_typed(TestRaiseError);

TVM_FFI_REGISTER_GLOBAL("testing.nop").set_body_packed([](PackedArgs args, Any* ret) {
    *ret = args[0];
});

TVM_FFI_REGISTER_GLOBAL("testing.echo").set_body_packed([](PackedArgs args, Any* ret) {
    *ret = args[0];
});

void TestApply(Function f, PackedArgs args, Any* ret) { f.CallPacked(args, ret); }

TVM_FFI_REGISTER_GLOBAL("testing.apply").set_body_packed([](PackedArgs args, Any* ret) {
    auto f = args[0].cast<Function>();
    TestApply(f, args.Slice(1), ret);
});

TVM_FFI_REGISTER_GLOBAL("testing.run_check_signal").set_body_typed([](int nsec) {
    for (int i = 0; i < nsec; ++i) {
        if (TVMFFIEnvCheckSignals() != 0) {
            throw ffi::EnvErrorAlreadySet();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Function finished without catching signal" << std::endl;
});

TVM_FFI_REGISTER_GLOBAL("testing.object_use_count").set_body_typed([](const Object* obj) {
    return obj->use_count();
});

}// namespace ffi
}// namespace litetvm