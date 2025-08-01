//
// Created by 赵丹 on 25-5-15.
//
#ifndef _MSC_VER

#include "ffi/traceback.h"
#include "ffi/c_api.h"
#include "ffi/error.h"

#if TVM_FFI_USE_LIBBACKTRACE

#include <backtrace.h>
#include <cxxabi.h>

#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>

#if TVM_FFI_BACKTRACE_ON_SEGFAULT
#include <csignal>
#endif

namespace litetvm {
namespace ffi {
namespace {

void BacktraceCreateErrorCallback(void*, const char* msg, int) {
    std::cerr << "Could not initialize backtrace state: " << msg << std::endl;
}

backtrace_state* BacktraceCreate() {
    return backtrace_create_state(nullptr, 1, BacktraceCreateErrorCallback, nullptr);
}

static backtrace_state* _bt_state = BacktraceCreate();

std::string DemangleName(std::string name) {
    int status = 0;
    size_t length = name.size();
    char* demangled_name = abi::__cxa_demangle(name.c_str(), nullptr, &length, &status);
    if (demangled_name && status == 0 && length > 0) {
        name = demangled_name;
    }
    if (demangled_name) {
        std::free(demangled_name);
    }
    return name;
}

void BacktraceErrorCallback(void*, const char*, int) {
    // do nothing
}

void BacktraceSyminfoCallback(void* data, uintptr_t pc, const char* symname, uintptr_t, uintptr_t) {
    auto str = reinterpret_cast<std::string*>(data);

    if (symname != nullptr) {
        *str = DemangleName(symname);
    } else {
        std::ostringstream s;
        s << "0x" << std::setfill('0') << std::setw(sizeof(uintptr_t) * 2) << std::hex << pc;
        *str = s.str();
    }
}

int BacktraceFullCallback(void* data, uintptr_t pc, const char* filename, int lineno,
                          const char* symbol) {
    auto stack_trace = reinterpret_cast<TracebackStorage*>(data);
    std::string symbol_str = "<unknown>";
    if (symbol) {
        symbol_str = DemangleName(symbol);
    } else {
        // see if syminfo gives anything
        backtrace_syminfo(_bt_state, pc, BacktraceSyminfoCallback, BacktraceErrorCallback, &symbol_str);
    }
    symbol = symbol_str.data();

    if (stack_trace->ExceedTracebackLimit()) {
        return 1;
    }
    if (ShouldStopTraceback(filename, symbol)) {
        return 1;
    }
    if (ShouldExcludeFrame(filename, symbol)) {
        return 0;
    }
    stack_trace->Append(filename, symbol, lineno);
    return 0;
}

std::string Traceback() {
    TracebackStorage traceback;

    if (_bt_state == nullptr) {
        return "";
    }
    // libbacktrace eats memory if run on multiple threads at the same time, so we guard against it
    {
        static std::mutex m;
        std::lock_guard<std::mutex> lock(m);
        backtrace_full(_bt_state, 0, BacktraceFullCallback, BacktraceErrorCallback, &traceback);
    }
    return traceback.GetTraceback();
}

#if TVM_FFI_BACKTRACE_ON_SEGFAULT
void backtrace_handler(int sig) {
    // Technically we shouldn't do any allocation in a signal handler, but
    // Backtrace may allocate. What's the worst it could do? We're already
    // crashing.
    std::cerr << "!!!!!!! TVM FFI encountered a Segfault !!!!!!!\n"
              << Traceback() << std::endl;

    // Re-raise signal with default handler
    struct sigaction act;
    std::memset(&act, 0, sizeof(struct sigaction));
    act.sa_flags = SA_RESETHAND;
    act.sa_handler = SIG_DFL;
    sigaction(sig, &act, nullptr);
    raise(sig);
}

__attribute__((constructor)) void install_signal_handler(void) {
    // this may override already installed signal handlers
    std::signal(SIGSEGV, backtrace_handler);
}
#endif// TVM_FFI_BACKTRACE_ON_SEGFAULT
}// namespace
}// namespace ffi
}// namespace litetvm

const TVMFFIByteArray* TVMFFITraceback(const char*, int, const char*) {
    thread_local std::string traceback_str;
    thread_local TVMFFIByteArray traceback_array;
    traceback_str = ::litetvm::ffi::Traceback();
    traceback_array.data = traceback_str.data();
    traceback_array.size = traceback_str.size();
    return &traceback_array;
}
#else
// fallback implementation simply print out the last trace
const TVMFFIByteArray* TVMFFITraceback(const char* filename, int lineno, const char* func) {
    static thread_local std::string traceback_str;
    static thread_local TVMFFIByteArray traceback_array;
    std::ostringstream traceback_stream;
    // python style backtrace
    traceback_stream << "  File \"" << filename << "\", line " << lineno << ", in " << func << '\n';
    traceback_str = traceback_stream.str();
    traceback_array.data = traceback_str.data();
    traceback_array.size = traceback_str.size();
    return &traceback_array;
}
#endif// TVM_FFI_USE_LIBBACKTRACE

#endif