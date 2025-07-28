//
// Created by richard on 6/8/25.
//

#ifndef LITETVM_RUNTIME_LOGGING_H
#define LITETVM_RUNTIME_LOGGING_H

#include "ffi/error.h"
#include "ffi/macros.h"
#include "runtime/base.h"

#include <ctime>
#include <dmlc/common.h>
#include <dmlc/thread_local.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/*!
 * \brief Whether or not enable backtrace logging during a
 *        fatal error.
 *
 * \note TVM won't depend on LIBBACKTRACE or other exec_info
 *       library when this option is disabled.
 */
#ifndef TVM_LOG_STACK_TRACE
#define TVM_LOG_STACK_TRACE 1
#endif

/*!
 * \brief Whether or not use libbacktrace library
 *        for getting backtrace information
 */
#ifndef TVM_USE_LIBBACKTRACE
#define TVM_USE_LIBBACKTRACE 0
#endif

/*!
 * \brief Whether or not customize the logging output.
 *  If log customize is enabled, the user must implement
 *  tvm::runtime::detail::LogFatalImpl and tvm::runtime::detail::LogMessageImpl.
 */
#ifndef TVM_LOG_CUSTOMIZE
#define TVM_LOG_CUSTOMIZE 0
#endif


namespace litetvm {
namespace runtime {

}
}// namespace litetvm
#endif//LITETVM_RUNTIME_LOGGING_H
