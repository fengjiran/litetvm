//
// Created by richard on 8/1/25.
//

#ifndef LITETVM_RUNTIME_NVTX_H
#define LITETVM_RUNTIME_NVTX_H

#include "runtime/base.h"
#include <string>

namespace litetvm {
namespace runtime {

/*!
 * \brief A class to create a NVTX range. No-op if TVM is not built against NVTX.
 */
class NVTXScopedRange {
public:
    /*! \brief Enter an NVTX scoped range */
    TVM_DLL explicit NVTXScopedRange(const char* name);
    /*! \brief Enter an NVTX scoped range */
    explicit NVTXScopedRange(const std::string& name) : NVTXScopedRange(name.c_str()) {}
    /*! \brief Exist an NVTX scoped range */
    TVM_DLL ~NVTXScopedRange();
    NVTXScopedRange(const NVTXScopedRange& other) = delete;
    NVTXScopedRange(NVTXScopedRange&& other) = delete;
    NVTXScopedRange& operator=(const NVTXScopedRange& other) = delete;
    NVTXScopedRange& operator=(NVTXScopedRange&& other) = delete;
};

#ifdef _MSC_VER
#define TVM_NVTX_FUNC_SCOPE() NVTXScopedRange _nvtx_func_scope_(__FUNCSIG__);
#else
#define TVM_NVTX_FUNC_SCOPE() NVTXScopedRange _nvtx_func_scope_(__PRETTY_FUNCTION__);
#endif

}// namespace runtime
}// namespace litetvm

#endif//LITETVM_RUNTIME_NVTX_H
