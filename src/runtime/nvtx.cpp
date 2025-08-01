//
// Created by richard on 8/1/25.
//

#include "runtime/nvtx.h"

#ifndef TVM_NVTX_ENABLED
#define TVM_NVTX_ENABLED 0
#endif

#if TVM_NVTX_ENABLED
#include <nvtx3/nvToolsExt.h>
#endif

namespace litetvm {
namespace runtime {

#if TVM_NVTX_ENABLED
NVTXScopedRange::NVTXScopedRange(const char* name) { nvtxRangePush(name); }
NVTXScopedRange::~NVTXScopedRange() { nvtxRangePop(); }
#else
NVTXScopedRange::NVTXScopedRange(const char* name) {}
NVTXScopedRange::~NVTXScopedRange() {}
#endif// TVM_NVTX_ENABLED

}// namespace runtime
}// namespace litetvm