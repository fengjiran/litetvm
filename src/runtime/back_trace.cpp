//
// Created by 赵丹 on 25-2-12.
//

#include "runtime/back_trace.h"

#include <backtrace.h>
#include <cxxabi.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace litetvm {
namespace runtime {
namespace {

struct BacktraceInfo {
    std::vector<std::string> lines;
    size_t max_size;
    std::string error_message;
};

void BacktraceCreateErrorCallback(void* data, const char* msg, int errnum) {
    std::cerr << "Could not initialize backtrace state: " << msg << std::endl;
}

backtrace_state* BacktraceCreate() {
    return backtrace_create_state(nullptr, 1, BacktraceCreateErrorCallback, nullptr);
}

static backtrace_state* _bt_state = BacktraceCreate();

std::string DemangleName(std::string name) {
    int status = 0;
    size_t length = name.size();
    std::unique_ptr<char, void (*)(void* __ptr)> demangled_name = {
            abi::__cxa_demangle(name.c_str(), nullptr, &length, &status), &std::free};
    if (demangled_name && status == 0 && length > 0) {
        return demangled_name.get();
    } else {
        return name;
    }
}

void BacktraceErrorCallback(void* data, const char* msg, int errnum) {
    // do nothing
}

void BacktraceSyminfoCallback(void* data, uintptr_t pc, const char* symname, uintptr_t symval,
                              uintptr_t symsize) {
    auto str = reinterpret_cast<std::string*>(data);

    if (symname != nullptr) {
        std::string tmp(symname, symsize);
        *str = DemangleName(tmp.c_str());
    } else {
        std::ostringstream s;
        s << "0x" << std::setfill('0') << std::setw(sizeof(uintptr_t) * 2) << std::hex << pc;
        *str = s.str();
    }
}

int BacktraceFullCallback(void* data, uintptr_t pc, const char* filename, int lineno,
                          const char* symbol) {
    auto stack_trace = reinterpret_cast<BacktraceInfo*>(data);

    std::unique_ptr<std::string> symbol_str = std::make_unique<std::string>("<unknown>");
    if (symbol) {
        *symbol_str = DemangleName(symbol);
    } else {
        // see if syminfo gives anything
        backtrace_syminfo(_bt_state, pc, BacktraceSyminfoCallback, BacktraceErrorCallback,
                          symbol_str.get());
    }
    symbol = symbol_str->data();

    // TVMFuncCall denotes the API boundary so we stop there. Exceptions
    // should be caught there.  This is before any frame suppressions,
    // as it would otherwise be suppressed.
    bool should_stop_collecting =
            (*symbol_str == "TVMFuncCall" || stack_trace->lines.size() >= stack_trace->max_size);
    if (should_stop_collecting) {
        return 1;
    }

    // Exclude frames that contain little useful information for most
    // debugging purposes
    bool should_exclude = [&]() -> bool {
        if (filename) {
            // Stack frames for TVM FFI
            if (strstr(filename, "include/tvm/runtime/packed_func.h") ||
                strstr(filename, "include/tvm/runtime/registry.h") ||
                strstr(filename, "src/runtime/c_runtime_api.cc")) {
                return true;
            }
            // Stack frames for nested tree recursion.
            // tir/ir/stmt_functor.cc and tir/ir/expr_functor.cc define
            // Expr/Stmt Visitor/Mutator, which should be suppressed, but
            // also Substitute which should not be suppressed.  Therefore,
            // they are suppressed based on the symbol name.
            if (strstr(filename, "include/tvm/node/functor.h") ||      //
                strstr(filename, "include/tvm/relax/expr_functor.h") ||//
                strstr(filename, "include/tvm/tir/stmt_functor.h") ||  //
                strstr(filename, "include/tvm/tir/expr_functor.h") ||  //
                strstr(filename, "include/tvm/node/reflection.h") ||   //
                strstr(filename, "src/node/structural_equal.cc") ||    //
                strstr(filename, "src/ir/transform.cc") ||             //
                strstr(filename, "src/relax/ir/expr_functor.cc") ||    //
                strstr(filename, "src/relax/ir/py_expr_functor.cc")) {
                return true;
            }
            // Python interpreter stack frames
            if (strstr(filename, "/python-") || strstr(filename, "/Python/ceval.c") ||
                strstr(filename, "/Modules/_ctypes")) {
                return true;
            }
            // C++ stdlib frames
            if (strstr(filename, "include/c++/")) {
                return true;
            }
        }
        if (symbol) {
            // C++ stdlib frames
            if (strstr(symbol, "__libc_")) {
                return true;
            }
            // Stack frames for nested tree visiting
            if (strstr(symbol, "tvm::tir::StmtMutator::VisitStmt_") ||
                strstr(symbol, "tvm::tir::ExprMutator::VisitExpr_") ||
                strstr(symbol, "tvm::tir::IRTransformer::VisitExpr") ||
                strstr(symbol, "tvm::tir::IRTransformer::VisitStmt") ||
                strstr(symbol, "tvm::tir::IRTransformer::BaseVisitExpr") ||
                strstr(symbol, "tvm::tir::IRTransformer::BaseVisitStmt")) {
                return true;
            }
            // Python interpreter stack frames
            if (strstr(symbol, "_Py") == symbol || strstr(symbol, "PyObject")) {
                return true;
            }
        }

        // libffi.so stack frames.  These may also show up as numeric
        // addresses with no symbol name.  This could be improved in the
        // future by using dladdr() to check whether an address is contained
        // in libffi.so
        if (filename == nullptr && strstr(symbol, "ffi_call_")) {
            return true;
        }

        // Skip tvm::backtrace and tvm::LogFatal::~LogFatal at the beginning
        // of the trace as they don't add anything useful to the backtrace.
        if (stack_trace->lines.size() == 0 && (strstr(symbol, "tvm::runtime::Backtrace") ||
                                               strstr(symbol, "tvm::runtime::detail::LogFatal"))) {
            return true;
        }

        return false;
    }();
    if (should_exclude) {
        return 0;
    }

    std::stringstream frame_str;
    frame_str << *symbol_str;

    if (filename) {
        frame_str << std::endl
                  << "        at " << filename;
        if (lineno != 0) {
            frame_str << ":" << lineno;
        }
    }
    stack_trace->lines.push_back(frame_str.str());

    return 0;
}

}// namespace

std::string Backtrace() {
    BacktraceInfo bt;

    // Limit backtrace length based on TVM_BACKTRACE_LIMIT env variable
    auto user_limit_s = getenv("TVM_BACKTRACE_LIMIT");
    const auto default_limit = 500;

    if (user_limit_s == nullptr) {
        bt.max_size = default_limit;
    } else {
        // Parse out the user-set backtrace limit
        try {
            bt.max_size = std::stoi(user_limit_s);
        } catch (const std::invalid_argument& e) {
            bt.max_size = default_limit;
        }
    }

    if (_bt_state == nullptr) {
        return "";
    }
    // libbacktrace eats memory if run on multiple threads at the same time, so we guard against it
    {
        static std::mutex m;
        std::lock_guard<std::mutex> lock(m);
        backtrace_full(_bt_state, 0, BacktraceFullCallback, BacktraceErrorCallback, &bt);
    }

    std::ostringstream s;
    s << "Stack trace:\n";
    for (size_t i = 0; i < bt.lines.size(); i++) {
        s << "  " << i << ": " << bt.lines[i] << "\n";
    }

    return s.str();
}

}// namespace runtime
}// namespace litetvm
