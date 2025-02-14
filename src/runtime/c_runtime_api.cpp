//
// Created by richard on 2/4/25.
//

#include "runtime/c_runtime_api.h"
#include "runtime/c_backend_api.h"
#include "runtime/device_api.h"
#include "runtime/packed_func.h"
#include "runtime/registry.h"

#include <array>
#include <dmlc/thread_local.h>

namespace litetvm::runtime {

std::string GetCustomTypeName(uint8_t type_code) {
    auto f = RegistryManager::Global().Get("runtime._datatype_get_type_name");
    CHECK(f) << "Function runtime._datatype_get_type_name not found";
    return (*f)(type_code).operator std::string();
}

uint8_t GetCustomTypeCode(const std::string& type_name) {
    auto f = RegistryManager::Global().Get("runtime._datatype_get_type_code");
    CHECK(f) << "Function runtime._datatype_get_type_code not found";
    return (*f)(type_name).operator int();
}
//
bool GetCustomTypeRegistered(uint8_t type_code) {
    auto f = runtime::RegistryManager::Global().Get("runtime._datatype_get_type_registered");
    CHECK(f) << "Function runtime._datatype_get_type_registered not found";
    return (*f)(type_code).operator bool();
}

uint8_t ParseCustomDatatype(const std::string& s, const char** scan) {
    CHECK(s.substr(0, 6) == "custom") << "Not a valid custom datatype string";

    auto tmp = s.c_str();

    CHECK(s.c_str() == tmp);
    *scan = s.c_str() + 6;
    CHECK(s.c_str() == tmp);
    if (**scan != '[') LOG(FATAL) << "expected opening brace after 'custom' type in" << s;
    CHECK(s.c_str() == tmp);
    *scan += 1;
    CHECK(s.c_str() == tmp);
    size_t custom_name_len = 0;
    CHECK(s.c_str() == tmp);
    while (*scan + custom_name_len <= s.c_str() + s.length() && *(*scan + custom_name_len) != ']')
        ++custom_name_len;
    CHECK(s.c_str() == tmp);
    if (*(*scan + custom_name_len) != ']')
        LOG(FATAL) << "expected closing brace after 'custom' type in" << s;
    CHECK(s.c_str() == tmp);
    *scan += custom_name_len + 1;
    CHECK(s.c_str() == tmp);

    auto type_name = s.substr(7, custom_name_len);
    CHECK(s.c_str() == tmp);
    return GetCustomTypeCode(type_name);
}

DeviceAPI* DeviceAPI::Get(Device dev, bool allow_missing) {
    return DeviceAPIManager::Get(static_cast<int>(dev.device_type), allow_missing);
}

void* DeviceAPI::AllocWorkspace(Device dev, size_t size, DLDataType type_hint) {
    return AllocDataSpace(dev, size, kTempAllocaAlignment, type_hint);
}

static size_t GetDataAlignment(const DLDataType dtype) {
    size_t align = dtype.bits / 8 * dtype.lanes;
    if (align < kAllocAlignment) return kAllocAlignment;
    return align;
}

size_t DeviceAPI::GetDataSize(const DLTensor& arr, const Optional<String>& mem_scope) {
    if (!mem_scope.defined() || mem_scope.value().empty() || mem_scope.value() == "global") {
        size_t size = 1;
        for (tvm_index_t i = 0; i < arr.ndim; ++i) {
            size *= static_cast<size_t>(arr.shape[i]);
        }
        size *= (arr.dtype.bits * arr.dtype.lanes + 7) / 8;
        return size;
    }
    LOG(FATAL) << "Device does not support physical mem computation with "
               << "specified memory scope: " << mem_scope.value();
    return 0;
}

void* DeviceAPI::AllocDataSpace(Device dev, int ndim, const int64_t* shape, DLDataType dtype,
                                const Optional<String>& mem_scope) {
    if (!mem_scope.defined() || mem_scope.value().empty() || mem_scope.value() == "global") {
        // by default, we can always redirect to the flat memory allocations
        DLTensor temp;
        temp.data = nullptr;
        temp.device = dev;
        temp.ndim = ndim;
        temp.dtype = dtype;
        temp.shape = const_cast<int64_t*>(shape);
        temp.strides = nullptr;
        temp.byte_offset = 0;
        size_t size = GetDataSize(temp);
        size_t alignment = GetDataAlignment(temp.dtype);
        return AllocDataSpace(dev, size, alignment, dtype);
    }
    LOG(FATAL) << "Device does not support allocate data space with "
               << "specified memory scope: " << mem_scope.value();
    return nullptr;
}

void DeviceAPI::CopyDataFromTo(DLTensor* from, DLTensor* to, TVMStreamHandle stream) {
    // by default, we can always redirect to the flat memory copy operation.
    size_t nbytes = GetDataSize(*from);
    CHECK_EQ(nbytes, GetDataSize(*to));

    CHECK(IsContiguous(*from) && IsContiguous(*to))
            << "CopyDataFromTo only support contiguous array for now";

    CopyDataFromTo(from->data, from->byte_offset,
                   to->data, to->byte_offset,
                   nbytes, from->device,
                   to->device, from->dtype, stream);
}

void DeviceAPI::CopyDataFromTo(const void* from, size_t from_offset, void* to, size_t to_offset,
                               size_t num_bytes, Device dev_from, Device dev_to,
                               DLDataType type_hint, TVMStreamHandle stream) {
    LOG(FATAL) << "Device does not support CopyDataFromTo.";
}

void DeviceAPI::FreeWorkspace(Device dev, void* ptr) { FreeDataSpace(dev, ptr); }

TVMStreamHandle DeviceAPI::CreateStream(Device dev) { return nullptr; }

void DeviceAPI::FreeStream(Device dev, TVMStreamHandle stream) {}

TVMStreamHandle DeviceAPI::GetCurrentStream(Device dev) { return nullptr; }

void DeviceAPI::SyncStreamFromTo(Device dev, TVMStreamHandle event_src, TVMStreamHandle event_dst) {
}

//--------------------------------------------------------
// Error handling mechanism
// -------------------------------------------------------
// Standard error message format, {} means optional
//--------------------------------------------------------
// {error_type:} {message0}
// {message1}
// {message2}
// {Stack trace:}    // stack traces follow by this line
//   {trace 0}       // two spaces in the beginning.
//   {trace 1}
//   {trace 2}
//--------------------------------------------------------
/*!
 * \brief Normalize error message
 *
 *  Parse them header generated by LOG(FATAL) and ICHECK
 *  and reformat the message into the standard format.
 *
 *  This function will also merge all the stack traces into
 *  one trace and trim them.
 *
 * \param err_msg The error message.
 * \return normalized message.
 */
std::string NormalizeError(std::string err_msg) {
    // ------------------------------------------------------------------------
    // log with header, {} indicates optional
    //-------------------------------------------------------------------------
    // [timestamp] file_name:line_number: {check_msg:} {error_type:} {message0}
    // {message1}
    // Stack trace:
    //   {stack trace 0}
    //   {stack trace 1}
    //-------------------------------------------------------------------------
    // Normalzied version
    //-------------------------------------------------------------------------
    // error_type: check_msg message0
    // {message1}
    // Stack trace:
    //   File file_name, line lineno
    //   {stack trace 0}
    //   {stack trace 1}
    //-------------------------------------------------------------------------
    int line_number = 0;
    std::istringstream is(err_msg);
    std::string line, file_name, error_type, check_msg;

    // Parse log header and set the fields,
    // Return true if it the log is in correct format,
    // return false if something is wrong.
    auto parse_log_header = [&]() {
        // skip timestamp
        if (is.peek() != '[') {
            getline(is, line);
            return true;
        }
        if (!(is >> line)) return false;
        // get filename
        while (is.peek() == ' ') is.get();
#ifdef _MSC_VER// handle volume separator ":" in Windows path
        std::string drive;
        if (!getline(is, drive, ':')) return false;
        if (!getline(is, file_name, ':')) return false;
        file_name = drive + ":" + file_name;
#else
        if (!getline(is, file_name, ':')) return false;
#endif
        // get line number
        if (!(is >> line_number)) return false;
        // get rest of the message.
        while (is.peek() == ' ' || is.peek() == ':') is.get();
        if (!getline(is, line)) return false;
        // detect check message, rewrite to remote extra :
        if (line.compare(0, 13, "Check failed:") == 0) {
            std::string ending = ": ";
            size_t end_pos = line.find(ending, 13);
            if (end_pos == std::string::npos) return false;
            check_msg = line.substr(0, end_pos + ending.size());
            line = line.substr(end_pos + ending.size());
        }
        return true;
    };
    // if not in correct format, do not do any rewrite.
    if (!parse_log_header()) return err_msg;
    // Parse error type.
    {
        size_t start_pos = 0, end_pos;
        for (; start_pos < line.length() && line[start_pos] == ' '; ++start_pos) {
        }
        for (end_pos = start_pos; end_pos < line.length(); ++end_pos) {
            char ch = line[end_pos];
            if (ch == ':') {
                error_type = line.substr(start_pos, end_pos - start_pos);
                break;
            }
            // [A-Z0-9a-z_.]
            if (!std::isalpha(ch) && !std::isdigit(ch) && ch != '_' && ch != '.') break;
        }
        if (error_type.length() != 0) {
            // if we successfully detected error_type: trim the following space.
            for (start_pos = end_pos + 1; start_pos < line.length() && line[start_pos] == ' ';
                 ++start_pos) {
            }
            line = line.substr(start_pos);
        } else {
            // did not detect error_type, use default value.
            line = line.substr(start_pos);
            error_type = "TVMError";
        }
    }
    // Separate out stack trace.
    std::ostringstream os;
    os << error_type << ": " << check_msg << line << '\n';

    bool trace_mode = true;
    std::vector<std::string> stack_trace;
    while (getline(is, line)) {
        if (trace_mode) {
            if (line.compare(0, 2, "  ") == 0) {
                stack_trace.push_back(line);
            } else {
                trace_mode = false;
                // remove EOL trailing stacktrace.
                if (line.length() == 0) continue;
            }
        }
        if (!trace_mode) {
            if (line.compare(0, 11, "Stack trace") == 0) {
                trace_mode = true;
            } else {
                os << line << '\n';
            }
        }
    }
    if (stack_trace.size() != 0 || file_name.length() != 0) {
        os << "Stack trace:\n";
        if (file_name.length() != 0) {
            os << "  File \"" << file_name << "\", line " << line_number << "\n";
        }
        // Print out stack traces, optionally trim the c++ traces
        // about the frontends (as they will be provided by the frontends).
        bool ffi_boundary = false;
        for (const auto& line: stack_trace) {
            // Heuristic to detect python ffi.
            if (line.find("libffi.so") != std::string::npos ||
                line.find("core.cpython") != std::string::npos) {
                ffi_boundary = true;
            }
            // If the backtrace is not c++ backtrace with the prefix "  [bt]",
            // then we can stop trimming.
            if (ffi_boundary && line.compare(0, 6, "  [bt]") != 0) {
                ffi_boundary = false;
            }
            if (!ffi_boundary) {
                os << line << '\n';
            }
            // The line after TVMFuncCall cound be in FFI.
            if (line.find("(TVMFuncCall") != std::string::npos) {
                ffi_boundary = true;
            }
        }
    }
    return os.str();
}

}// namespace litetvm::runtime

using namespace litetvm::runtime;

struct WrappedPythonError : Error {
    WrappedPythonError() : Error("") {}
    explicit WrappedPythonError(WrappedPythonObject obj)
        : Error(""), obj(std::move(obj)), cpp_backtrace(litetvm::runtime::Backtrace()) {}

    WrappedPythonObject obj;
    std::string cpp_backtrace;
};

struct TVMRuntimeEntry {
    std::string ret_str;
    TVMByteArray ret_bytes;

    std::variant<WrappedPythonError, InternalError, std::string> last_error;
    std::string last_error_formatted;
};

typedef dmlc::ThreadLocalStore<TVMRuntimeEntry> TVMAPIRuntimeStore;

const char* TVMGetLastError() {
    auto* store = TVMAPIRuntimeStore::Get();
    const auto& last_error = store->last_error;
    if (const auto* message = std::get_if<std::string>(&last_error)) {
        return message->c_str();
    } else if (const auto* internal = std::get_if<InternalError>(&last_error)) {
        // Use last_error_formatted to store the formatted error message, to avoid
        // dangling pointer.
        store->last_error_formatted = NormalizeError(internal->full_message());
        return store->last_error_formatted.c_str();
    } else {
        return nullptr;
    }
}

int TVMBackendGetFuncFromEnv(void* mod_node, const char* func_name, TVMFunctionHandle* func) {
    // API_BEGIN();
    *func = (TVMFunctionHandle)(static_cast<ModuleNode*>(mod_node)->GetFuncFromEnv(func_name))->get();
    // API_END();
    return 0;
}

int TVMFuncCall(TVMFunctionHandle func, TVMValue* args, int* arg_type_codes, int num_args,
                TVMValue* ret_val, int* ret_type_code) {
    // API_BEGIN();
    TVMRetValue rv;
    (static_cast<const PackedFuncObj*>(func))
        ->CallPacked(TVMArgs(args, arg_type_codes, num_args), &rv);
    // handle return string.
    if (rv.type_code() == static_cast<int>(TVMArgTypeCode::kTVMStr) ||
        rv.type_code() == static_cast<int>(TVMArgTypeCode::kTVMDataType) ||
        rv.type_code() == static_cast<int>(TVMArgTypeCode::kTVMBytes)) {
        TVMRuntimeEntry* e = TVMAPIRuntimeStore::Get();
        if (rv.type_code() != static_cast<int>(TVMArgTypeCode::kTVMDataType)) {
            e->ret_str = *rv.ptr<std::string>();
        } else {
            e->ret_str = rv.operator std::string();
        }
        if (rv.type_code() == static_cast<int>(TVMArgTypeCode::kTVMBytes)) {
            e->ret_bytes.data = e->ret_str.c_str();
            e->ret_bytes.size = e->ret_str.length();
            *ret_type_code = static_cast<int>(TVMArgTypeCode::kTVMBytes);
            ret_val->v_handle = &(e->ret_bytes);
        } else {
            *ret_type_code = static_cast<int>(TVMArgTypeCode::kTVMStr);
            ret_val->v_str = e->ret_str.c_str();
        }
    } else {
        rv.MoveToCHost(ret_val, ret_type_code);
    }
    // API_END();
    return 0;
}