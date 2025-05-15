//
// Created by richard on 5/15/25.
//

#include "ffi/function.h"
#include "ffi/any.h"
#include "ffi/c_api.h"
#include "ffi/cast.h"
#include "ffi/container/array.h"
#include "ffi/error.h"
#include "ffi/string.h"

#include <unordered_map>

namespace litetvm {
namespace ffi {

/*!
 * \brief Global function table.
 *

 * \note We do not use mutex to guard updating of GlobalFunctionTable
 *
 * The assumption is that updating of GlobalFunctionTable will be done
 * in the main thread during initialization or loading, or
 * explicitly locked from the caller.
 *
 * Then the followup code will leverage the information
 */
class GlobalFunctionTable {
public:
    void Update(const String& name, Function func, bool can_override) {
        if (table_.count(name)) {
            if (!can_override) {
                TVM_FFI_THROW(RuntimeError) << "Global Function `" << name << "` is already registered";
            }
        }
        table_[name] = new Function(func);
    }

    bool Remove(const String& name) {
        auto it = table_.find(name);
        if (it == table_.end()) return false;
        table_.erase(it);
        return true;
    }

    const Function* Get(const String& name) {
        auto it = table_.find(name);
        if (it == table_.end()) return nullptr;
        return it->second;
    }

    Array<String> ListNames() const {
        Array<String> names;
        names.reserve(table_.size());
        for (const auto& kv: table_) {
            names.push_back(kv.first);
        }
        return names;
    }

    static GlobalFunctionTable* Global() {
        // We deliberately create a new instance via raw new
        // This is because GlobalFunctionTable can contain callbacks into
        // the host language (Python) and the resource can become invalid
        // indeterministic order of destruction and forking.
        // The resources will only be recycled during program exit.
        static GlobalFunctionTable* inst = new GlobalFunctionTable();
        return inst;
    }

private:
    // deliberately track function pointer without recycling
    // to avoid
    std::unordered_map<String, Function*> table_;
};

/*!
 * \brief Execution environment specific API registry.
 *
 *  This registry stores C API function pointers about
 *  execution environment(e.g. python) specific API function that
 *  we need for specific low-level handling(e.g. signal checking).
 *
 *  We only stores the C API function when absolutely necessary (e.g. when signal handler
 *  cannot trap back into python). Always consider use the Function FFI when possible
 *  in other cases.
 */
class EnvCAPIRegistry {
public:
    /*!
   * \brief Callback to check if signals have been sent to the process and
   *        if so invoke the registered signal handler in the frontend environment.
   *
   *  When running FFI in another language (Python), the signal handler
   *  may not be immediately executed, but instead the signal is marked
   *  in the interpreter state (to ensure non-blocking of the signal handler).
   *
   * \return 0 if no error happens, -1 if error happens.
   */
    typedef int (*F_PyErr_CheckSignals)();

    /*! \brief Callback to increment/decrement the python ref count */
    typedef void (*F_Py_IncDefRef)(void*);

    /*!
   * \brief PyErr_CheckSignal function
   */
    F_PyErr_CheckSignals pyerr_check_signals = nullptr;

    /*!
    \brief PyGILState_Ensure function
   */
    void* (*py_gil_state_ensure)() = nullptr;

    /*!
    \brief PyGILState_Release function
   */
    void (*py_gil_state_release)(void*) = nullptr;

    static EnvCAPIRegistry* Global() {
        static EnvCAPIRegistry* inst = new EnvCAPIRegistry();
        return inst;
    }

    // register environment(e.g. python) specific api functions
    void Register(const std::string& symbol_name, void* fptr) {
        if (symbol_name == "PyErr_CheckSignals") {
            Update(symbol_name, &pyerr_check_signals, fptr);
        } else if (symbol_name == "PyGILState_Ensure") {
            Update(symbol_name, &py_gil_state_ensure, fptr);
        } else if (symbol_name == "PyGILState_Release") {
            Update(symbol_name, &py_gil_state_release, fptr);
        } else {
            TVM_FFI_THROW(ValueError) << "Unknown env API " + symbol_name;
        }
    }

    // implementation of tvm::runtime::EnvCheckSignals
    int EnvCheckSignals() {
        // check python signal to see if there are exception raised
        if (pyerr_check_signals != nullptr) {
            // The C++ env comes without gil, so we need to grab gil here
            WithGIL context(this);
            if ((*pyerr_check_signals)() != 0) {
                // The error will let FFI know that the frontend environment
                // already set an error.
                return -1;
            }
        }
        return 0;
    }

private:
    // update the internal API table
    template<typename FType>
    void Update(const String& symbol_name, FType* target, void* ptr) {
        FType ptr_casted = reinterpret_cast<FType>(ptr);
        target[0] = ptr_casted;
    }

    struct WithGIL {
        explicit WithGIL(EnvCAPIRegistry* self) : self(self) {
            TVM_FFI_ICHECK(self->py_gil_state_ensure);
            TVM_FFI_ICHECK(self->py_gil_state_release);
            gil_state = self->py_gil_state_ensure();
        }
        ~WithGIL() {
            if (self && gil_state) {
                self->py_gil_state_release(gil_state);
            }
        }
        WithGIL(const WithGIL&) = delete;
        WithGIL(WithGIL&&) = delete;
        WithGIL& operator=(const WithGIL&) = delete;
        WithGIL& operator=(WithGIL&&) = delete;

        EnvCAPIRegistry* self = nullptr;
        void* gil_state = nullptr;
    };
};
}// namespace ffi
}// namespace litetvm

int TVMFFIFunctionCreate(void* self, TVMFFISafeCallType safe_call, void (*deleter)(void* self),
                         TVMFFIObjectHandle* out) {
    TVM_FFI_SAFE_CALL_BEGIN();
    litetvm::ffi::Function func = litetvm::ffi::Function::FromExternC(self, safe_call, deleter);
    *out = litetvm::ffi::details::ObjectUnsafe::MoveObjectRefToTVMFFIObjectPtr(std::move(func));
    TVM_FFI_SAFE_CALL_END();
}

int TVMFFIAnyViewToOwnedAny(const TVMFFIAny* any_view, TVMFFIAny* out) {
    TVM_FFI_SAFE_CALL_BEGIN();
    litetvm::ffi::Any result(*reinterpret_cast<const litetvm::ffi::AnyView*>(any_view));
    *out = litetvm::ffi::details::AnyUnsafe::MoveAnyToTVMFFIAny(std::move(result));
    TVM_FFI_SAFE_CALL_END();
}

int TVMFFIFunctionSetGlobal(const TVMFFIByteArray* name, TVMFFIObjectHandle f, int override) {
    using namespace litetvm::ffi;
    TVM_FFI_SAFE_CALL_BEGIN();
    String name_str(name->data, name->size);
    GlobalFunctionTable::Global()->Update(name_str, GetRef<Function>(static_cast<FunctionObj*>(f)),
                                          override != 0);
    TVM_FFI_SAFE_CALL_END();
}

int TVMFFIFunctionGetGlobal(const TVMFFIByteArray* name, TVMFFIObjectHandle* out) {
    using namespace litetvm::ffi;
    TVM_FFI_SAFE_CALL_BEGIN();
    String name_str(name->data, name->size);
    const Function* fp = GlobalFunctionTable::Global()->Get(name_str);
    if (fp != nullptr) {
        litetvm::ffi::Function func(*fp);
        *out = litetvm::ffi::details::ObjectUnsafe::MoveObjectRefToTVMFFIObjectPtr(std::move(func));
    } else {
        *out = nullptr;
    }
    TVM_FFI_SAFE_CALL_END();
}

int TVMFFIFunctionCall(TVMFFIObjectHandle func, TVMFFIAny* args, int32_t num_args,
                       TVMFFIAny* result) {
    using namespace litetvm::ffi;
    // NOTE: this is a tail call
    return reinterpret_cast<FunctionObj*>(func)->safe_call(func, args, num_args, result);
}

int TVMFFIEnvCheckSignals() { return litetvm::ffi::EnvCAPIRegistry::Global()->EnvCheckSignals(); }

/*!
 * \brief Register a symbol into the from the surrounding env.
 * \param name The name of the symbol.
 * \param symbol The symbol to register.
 * \return 0 when success, nonzero when failure happens
 */
int TVMFFIEnvRegisterCAPI(const TVMFFIByteArray* name, void* symbol) {
    TVM_FFI_SAFE_CALL_BEGIN();
    std::string s_name(name->data, name->size);
    litetvm::ffi::EnvCAPIRegistry::Global()->Register(s_name, symbol);
    TVM_FFI_SAFE_CALL_END();
}

TVM_FFI_REGISTER_GLOBAL("ffi.FunctionRemoveGlobal")
        .set_body_typed([](const litetvm::ffi::String& name) -> bool {
            return litetvm::ffi::GlobalFunctionTable::Global()->Remove(name);
        });

TVM_FFI_REGISTER_GLOBAL("ffi.FunctionListGlobalNamesFunctor").set_body_typed([]() {
    // NOTE: we return functor instead of array
    // so list global function names do not need to depend on array
    // this is because list global function names usually is a core api that happens
    // before array ffi functions are available.
    litetvm::ffi::Array<litetvm::ffi::String> names = litetvm::ffi::GlobalFunctionTable::Global()->ListNames();
    auto return_functor = [names](int64_t i) -> litetvm::ffi::Any {
        if (i < 0) {
            return names.size();
        } else {
            return names[i];
        }
    };
    return litetvm::ffi::Function::FromTyped(return_functor);
});

TVM_FFI_REGISTER_GLOBAL("ffi.String").set_body_typed([](litetvm::ffi::String val) -> litetvm::ffi::String {
    return val;
});

TVM_FFI_REGISTER_GLOBAL("ffi.Bytes").set_body_typed([](litetvm::ffi::Bytes val) -> litetvm::ffi::Bytes {
    return val;
});
