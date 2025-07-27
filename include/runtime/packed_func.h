//
// Created by richard on 1/30/25.
//

#ifndef LITETVM_PACKED_FUNC_H
#define LITETVM_PACKED_FUNC_H

#include "ffi/any.h"
#include "ffi/function.h"

namespace litetvm {
namespace runtime {

#define TVM_DLL_EXPORT_TYPED_FUNC TVM_FFI_DLL_EXPORT_TYPED_FUNC

using ffi::Any;
using ffi::AnyView;

}// namespace runtime
}// namespace litetvm

#endif//LITETVM_PACKED_FUNC_H
