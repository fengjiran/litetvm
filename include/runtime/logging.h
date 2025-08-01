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
 * \brief Whether enable backtrace logging during a
 *        fatal error.
 *
 * \note TVM won't depend on LIBBACKTRACE or other exec_info
 *       library when this option is disabled.
 */
#ifndef TVM_LOG_STACK_TRACE
#define TVM_LOG_STACK_TRACE 1
#endif

/*!
 * \brief Whether use libbacktrace library
 *        for getting backtrace information
 */
#ifndef TVM_USE_LIBBACKTRACE
#define TVM_USE_LIBBACKTRACE 0
#endif

/*!
 * \brief Whether customize the logging output.
 *  If log customize is enabled, the user must implement
 *  tvm::runtime::detail::LogFatalImpl and tvm::runtime::detail::LogMessageImpl.
 */
#ifndef TVM_LOG_CUSTOMIZE
#define TVM_LOG_CUSTOMIZE 0
#endif

// a technique that enables overriding macro names on the number of parameters. This is used
// to define other macros below
#define GET_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME

#define COND_CHECK_GE(...) \
    GET_MACRO(__VA_ARGS__, COND_CHECK_GE_5, COND_CHECK_GE_4, COND_CHECK_GE_3)(__VA_ARGS__)
#define COND_CHECK_EQ(...) \
    GET_MACRO(__VA_ARGS__, COND_CHECK_EQ_5, COND_CHECK_EQ_4, COND_CHECK_EQ_3)(__VA_ARGS__)
#define COND_CHECK(...) \
    GET_MACRO(__VA_ARGS__, COND_CHECK_5, COND_CHECK_4, COND_CHECK_3, COND_CHECK_2)(__VA_ARGS__)
#define COND_LOG(...) \
    GET_MACRO(__VA_ARGS__, COND_LOG_5, COND_LOG_4, COND_LOG_3, COND_LOG_2)(__VA_ARGS__)


// Not supposed to be used by users directly.
#define COND_CHECK_OP(quit_on_assert, x, y, what, op) \
    if (!quit_on_assert) {                            \
        if (!((x) op(y))) what;                       \
    } else /* NOLINT(*) */                            \
        CHECK_##op(x, y)

#define COND_CHECK_EQ_4(quit_on_assert, x, y, what) COND_CHECK_OP(quit_on_assert, x, y, what, ==)
#define COND_CHECK_GE_4(quit_on_assert, x, y, what) COND_CHECK_OP(quit_on_assert, x, y, what, >=)

#define COND_CHECK_3(quit_on_assert, x, what) \
    if (!quit_on_assert) {                    \
        if (!(x)) what;                       \
    } else /* NOLINT(*) */                    \
        CHECK(x)

#define COND_LOG_3(quit_on_assert, x, what) \
    if (!quit_on_assert) {                  \
        what;                               \
    } else /* NOLINT(*) */                  \
        LOG(x)

#define COND_CHECK_EQ_3(quit_on_assert, x, y) COND_CHECK_EQ_4(quit_on_assert, x, y, return false)
#define COND_CHECK_GE_3(quit_on_assert, x, y) COND_CHECK_GE_4(quit_on_assert, x, y, return false)
#define COND_CHECK_2(quit_on_assert, x) COND_CHECK_3(quit_on_assert, x, return false)
#define COND_LOG_2(quit_on_assert, x) COND_LOG_3(quit_on_assert, x, return false)


namespace litetvm {
namespace runtime {

using ffi::EnvErrorAlreadySet;
using ffi::Error;

/*!
 * \brief Error type for errors from CHECK, ICHECK, and LOG(FATAL). This error
 * contains a backtrace of where it occurred.
 */
class InternalError : public Error {
public:
    /*! \brief Construct an error. Not recommended to use directly. Instead use LOG(FATAL).
     *
     * \param file The file where the error occurred.
     * \param lineno The line number where the error occurred.
     * \param message The error message to display.
     */
    InternalError(const std::string& file, int lineno, const std::string& message)
        : Error(DetectKind(message), DetectMessage(message), TVMFFITraceback(file.c_str(), lineno, "")) {}

private:
    // try to detect the kind of error from the message when the error type
    // is folded into the text message
    static std::string DetectKind(const std::string& message) {
        size_t pos = message.find("Error:");
        if (pos != std::string::npos) {
            size_t end = pos + 6;
            size_t begin = pos;
            for (; begin != 0 && message[begin - 1] != ' '; --begin) {
            }
            return message.substr(begin, end - begin - 1);
        }
        return "InternalError";
    }

    static std::string DetectMessage(const std::string& message) {
        size_t pos = message.find("Error:");
        if (pos != std::string::npos) {
            size_t end = pos + 6;
            size_t begin = pos;
            for (; begin != 0 && message[begin - 1] != ' '; --begin) {
            }
            if (end < message.size() && message[end] == ' ') {
                end += 1;
            }
            return message.substr(0, begin) + message.substr(end);
        }
        return message;
    }
};

/*! \brief Internal implementation */
namespace detail {
// Provide support for customized logging.
#if TVM_LOG_CUSTOMIZE
/*!
 * \brief Custom implementations of LogFatal.
 *
 * \sa TVM_LOG_CUSTOMIZE
 */
[[noreturn]] TVM_DLL void LogFatalImpl(const std::string& file, int lineno,
                                       const std::string& message);

/*!
 * \brief Custom implementations of LogMessage.
 *
 * \sa TVM_LOG_CUSTOMIZE
 */
TVM_DLL void LogMessageImpl(const std::string& file, int lineno, int level,
                            const std::string& message);

/*!
 * \brief Class to accumulate an error message and throw it. Do not use
 * directly, instead use LOG(FATAL).
 */
class LogFatal {
public:
    LogFatal(const std::string& file, int lineno) : file_(file), lineno_(lineno) {}
#ifdef _MSC_VER
#pragma disagnostic push
#pragma warning(disable : 4722)
#endif
    [[noreturn]] ~LogFatal() TVM_THROW_EXCEPTION { LogFatalImpl(file_, lineno_, stream_.str()); }
#ifdef _MSC_VER
#pragma disagnostic pop
#endif
    std::ostringstream& stream() { return stream_; }

private:
    std::ostringstream stream_;
    std::string file_;
    int lineno_;
};

/*!
 * \brief Class to accumulate an log message. Do not use directly, instead use
 * LOG(INFO), LOG(WARNING), LOG(ERROR).
 */
class LogMessage {
public:
    LogMessage(const std::string& file, int lineno, int level)
        : file_(file), lineno_(lineno), level_(level) {}
    ~LogMessage() { LogMessageImpl(file_, lineno_, level_, stream_.str()); }
    std::ostringstream& stream() { return stream_; }

private:
    std::string file_;
    int lineno_;
    int level_;
    std::ostringstream stream_;
};

#else

/*!
 * \brief Class to accumulate an error message and throw it. Do not use
 * directly, instead use LOG(FATAL).
 * \note The `LogFatal` class is designed to be an empty class to reduce stack size usage.
 * To play this trick, we use the thread-local storage to store its internal data.
 */
class LogFatal {
public:
    TVM_NO_INLINE LogFatal(const char* file, int lineno) {
        GetEntry().Init(file, lineno);
    }
#ifdef _MSC_VER
#pragma disagnostic push
#pragma warning(disable : 4722)
#endif
    [[noreturn]] ~LogFatal() TVM_THROW_EXCEPTION {
        GetEntry().Finalize();
        throw;
    }
#ifdef _MSC_VER
#pragma disagnostic pop
#endif
    std::ostringstream& stream() {
        return GetEntry().stream_;
    }

private:
    struct Entry {
        void Init(const char* file, int lineno) {
            this->stream_.str("");
            this->file_ = file;
            this->lineno_ = lineno;
        }
        [[noreturn]] TVM_NO_INLINE dmlc::Error Finalize() TVM_THROW_EXCEPTION {
            InternalError error(file_, lineno_, stream_.str());
#if DMLC_LOG_BEFORE_THROW
            std::cerr << error.what() << std::endl;
#endif
            throw error;
        }
        std::ostringstream stream_;
        std::string file_;
        int lineno_;
    };

    TVM_DLL TVM_NO_INLINE static Entry& GetEntry();
};

/*!
 * \brief Class to accumulate an log message. Do not use directly, instead use
 * LOG(INFO), LOG(WARNING), LOG(ERROR).
 */
class LogMessage {
public:
    LogMessage(const std::string& file, int lineno, int level) {
        std::time_t t = std::time(nullptr);
        stream_ << "[" << std::put_time(std::localtime(&t), "%H:%M:%S") << "] " << file << ":" << lineno
                << level_strings_[level];
    }
    TVM_NO_INLINE ~LogMessage() { std::cerr << stream_.str() << std::endl; }
    std::ostringstream& stream() { return stream_; }

private:
    std::ostringstream stream_;
    TVM_DLL static const char* level_strings_[];
};

#endif

// Below is from dmlc-core
// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
public:
    LogMessageVoidify() {}
    // This has to be an operator with a precedence lower than << but
    // higher than "?:". See its usage.
    void operator&(std::ostream&) {}
};

/*! \brief Captures the state of the \p TVM_LOG_DEBUG environment flag. */
class TvmLogDebugSettings {
public:
    /*!
   * \brief Parses the \p TVM_LOG_DEBUG environment flag as per the specification given by
   * \p DebugLoggingEnabled and \p VerboseLoggingEnabled, and caches the result.
   */
    inline static const TvmLogDebugSettings& FromFlag() {
        // Parse and cache the verbosity level map.
        static const auto* settings =
                new TvmLogDebugSettings(TvmLogDebugSettings::ParseSpec(std::getenv("TVM_LOG_DEBUG")));
        return *settings;
    }

    /*!
   * \brief Parses \p opt_spec as per specification for \p TVM_LOG_DEBUG given by
   * \p DebugLoggingEnabled and \p VerboseLoggingEnabled. Throws if specification is ill-formed.
   */
    static TvmLogDebugSettings ParseSpec(const char* opt_spec);

    /*!
   * \brief Implements \p VerboseLoggingEnabled below w.r.t. the already parsed \p TVM_LOG_DEBUG
   * environment variable.
   */
    inline bool VerboseEnabled(const char* opt_filename, int level) const {
        if (opt_filename == nullptr || level < 0 || vlog_level_map_.empty()) {
            return false;
        }
        return VerboseEnabledImpl(opt_filename, level);
    }

    /*! \brief Returns true if \p DLOG statements should be executed. */
    bool dlog_enabled() const { return dlog_enabled_; }

private:
    // Slow path for VerboseEnabled.
    bool VerboseEnabledImpl(const std::string& filename, int level) const;

    /*! \brief If true, DLOG statements are enabled. */
    bool dlog_enabled_ = false;
    /*!
   * \brief A map from canonicalized filenames to the maximum VLOG verbosity level for that file.
   * May also contain the 'wildcard' entry \p "DEFAULT" representing the level for all other files.
   */
    std::unordered_map<std::string, int> vlog_level_map_;
};

/*!
 * \brief Returns true if a DLOG statement is enabled by the \p TVM_LOG_DEBUG environment
 * variable. Requires:
 * \code
 *   TVM_LOG_DEBUG=1
 * \endcode
 * or a valid setting as described by \p VerboseLoggingEnabled below.
 */
// Also from dmlc-core
inline bool DebugLoggingEnabled() {
    static int state = 0;
    if (state == 0) {
        state = TvmLogDebugSettings::FromFlag().dlog_enabled() ? 1 : -1;
    }
    return state == 1;
}

/*!
 * \brief Returns true if a VLOG statement in \p filename is enabled by the \p TVM_LOG_DEBUG
 * environment variable for logging at verbosity \p level. Levels should be non-negative.
 *
 * Filenames are canonicalized to be w.r.t. the src/ dir of the TVM tree. (VLOG's should not
 * appear under include/).
 *
 * To enable file \p ir/bar.cc for level 0 only set:
 * \code
 * TVM_LOG_DEBUG="ir/bar.cc=0"
 * \endcode
 *
 * To enable all files up to level 3 but disable \p ir/bar.cc set:
 * \code
 * TVM_LOG_DEBUG="DEFAULT=2,ir/bar.cc=-1"
 * \endcode
 *
 * Any of these settings will also enable DLOG statements.
 */
inline bool VerboseLoggingEnabled(const char* opt_filename, int level) {
    return TvmLogDebugSettings::FromFlag().VerboseEnabled(opt_filename, level);
}

/*!
 * \brief A stack of VLOG context messages.
 *
 * For use by \p VLOG_CONTEXT macro only.
 */
class VLogContext {
public:
    void Push(std::stringstream* stream) { context_stack_.push_back(stream); }
    void Pop() {
        if (!context_stack_.empty()) {
            context_stack_.pop_back();
        }
    }

    std::string str() const;

private:
    std::vector<std::stringstream*> context_stack_;
};

/*! \brief Thread local \p VLogContext for tracking a stack of VLOG context messages. */
using ThreadLocalVLogContext = dmlc::ThreadLocalStore<VLogContext>;

/*!
 * \brief A RAII class to push/pos a VLOG context message onto the thread-local stack.
 *
 * For use by \p VLOG_CONTEXT macro only.
 */
class VLogContextEntry {
public:
    VLogContextEntry() { ThreadLocalVLogContext::Get()->Push(&sstream_); }
    ~VLogContextEntry() { ThreadLocalVLogContext::Get()->Pop(); }
    std::ostream& stream() { return sstream_; }

private:
    std::stringstream sstream_;
};

template<typename X, typename Y>
std::unique_ptr<std::string> LogCheckFormat(const X& x, const Y& y) {
    std::ostringstream os;
    os << " (" << x << " vs. " << y << ") ";// CHECK_XX(x, y) requires x and y can be serialized to
                                            // string. Use CHECK(x OP y) otherwise.
    return std::make_unique<std::string>(os.str());
}

// Inline _Pragma in macros does not work reliably on old version of MSVC and
// GCC. We wrap all comparisons in a function so that we can use #pragma to
// silence bad comparison warnings.
#define TVM_CHECK_FUNC(name, op)                                                            \
    template<typename X, typename Y>                                                        \
    TVM_ALWAYS_INLINE std::unique_ptr<std::string> LogCheck##name(const X& x, const Y& y) { \
        if (x op y) return nullptr;                                                         \
        return LogCheckFormat(x, y);                                                        \
    }                                                                                       \
    TVM_ALWAYS_INLINE std::unique_ptr<std::string> LogCheck##name(int x, int y) {           \
        return LogCheck##name<int, int>(x, y);                                              \
    }

#if defined(__GNUC__) || defined(__clang__)// GCC and Clang
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#elif defined(_MSC_VER)// MSVC
#pragma warning(push)
#pragma warning(disable : 4389)// '==' : signed/unsigned mismatch
#endif

TVM_CHECK_FUNC(_LT, <)
TVM_CHECK_FUNC(_GT, >)
TVM_CHECK_FUNC(_LE, <=)
TVM_CHECK_FUNC(_GE, >=)
TVM_CHECK_FUNC(_EQ, ==)
TVM_CHECK_FUNC(_NE, !=)

#if defined(__GNUC__) || defined(__clang__)// GCC and Clang
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)// MSVC
#pragma warning(pop)
#endif

}// namespace detail

#define TVM_LOG_LEVEL_DEBUG 0
#define TVM_LOG_LEVEL_INFO 1
#define TVM_LOG_LEVEL_WARNING 2
#define TVM_LOG_LEVEL_ERROR 3
#define TVM_LOG_LEVEL_FATAL 4
#define LOG(level) LOG_##level
#define LOG_DEBUG ::litetvm::runtime::detail::LogMessage(__FILE__, __LINE__, TVM_LOG_LEVEL_DEBUG).stream()
#define LOG_FATAL ::litetvm::runtime::detail::LogFatal(__FILE__, __LINE__).stream()
#define LOG_INFO ::litetvm::runtime::detail::LogMessage(__FILE__, __LINE__, TVM_LOG_LEVEL_INFO).stream()
#define LOG_ERROR ::litetvm::runtime::detail::LogMessage(__FILE__, __LINE__, TVM_LOG_LEVEL_ERROR).stream()
#define LOG_WARNING ::litetvm::runtime::detail::LogMessage(__FILE__, __LINE__, TVM_LOG_LEVEL_WARNING).stream()

#define TVM_CHECK_BINARY_OP(name, op, x, y)                                      \
    if (auto __tvm__log__err = ::litetvm::runtime::detail::LogCheck##name(x, y)) \
    ::litetvm::runtime::detail::LogFatal(__FILE__, __LINE__).stream()            \
            << "Check failed: " << #x " " #op " " #y << *__tvm__log__err << ": "

#define CHECK(x)                                                      \
    if (!(x))                                                         \
    ::litetvm::runtime::detail::LogFatal(__FILE__, __LINE__).stream() \
            << "Check failed: (" #x << ") is false: "


#define CHECK_LT(x, y) TVM_CHECK_BINARY_OP(_LT, <, x, y)
#define CHECK_GT(x, y) TVM_CHECK_BINARY_OP(_GT, >, x, y)
#define CHECK_LE(x, y) TVM_CHECK_BINARY_OP(_LE, <=, x, y)
#define CHECK_GE(x, y) TVM_CHECK_BINARY_OP(_GE, >=, x, y)
#define CHECK_EQ(x, y) TVM_CHECK_BINARY_OP(_EQ, ==, x, y)
#define CHECK_NE(x, y) TVM_CHECK_BINARY_OP(_NE, !=, x, y)
#define CHECK_NOTNULL(x)                                                                \
    ((x) == nullptr ? ::litetvm::runtime::detail::LogFatal(__FILE__, __LINE__).stream() \
                              << "Check not null: " #x << ' ',                          \
     (x) : (x))// NOLINT(*)

#define LOG_IF(severity, condition) \
    !(condition) ? (void) 0 : ::litettvm::runtime::detail::LogMessageVoidify() & LOG(severity)


#if TVM_LOG_DEBUG

#define LOG_DFATAL LOG_FATAL
#define DFATAL FATAL
#define DLOG(severity) LOG_IF(severity, ::litetvm::runtime::detail::DebugLoggingEnabled())
#define DLOG_IF(severity, condition) \
    LOG_IF(severity, ::litetvm::runtime::detail::DebugLoggingEnabled() && (condition))

/*!
 * \brief If the \p TVM_LOG_DEBUG build flag is enabled, push a context message onto an internal
 * stack. All VLOG messages will include this stack in their prefix to help with debugging. E.g.:
 * \code
 *   VLOG_CONTEXT << "my context";
 *   VLOG(1) << "my log message";
 * \endcode
 * Thread safe. No-op with no execution overhead if the \p TVM_LOG_DEBUG build flag is not enabled.
 */
#define VLOG_CONTEXT                                          \
    ::litetvm::runtime::detail::VLogContextEntry vlog_entry_; \
    vlog_entry_.stream()

#else

#define LOG_DFATAL LOG_ERROR
#define DFATAL ERROR
#define DLOG(severity) true ? (void) 0 : ::litetvm::runtime::detail::LogMessageVoidify() & LOG(severity)
#define DLOG_IF(severity, condition) \
    (true || !(condition)) ? (void) 0 : ::litetvm::runtime::detail::LogMessageVoidify() & LOG(severity)
#define VLOG_CONTEXT true ? (void) 0 : ::litetvm::runtime::detail::LogMessageVoidify() & LOG(INFO)
#endif

/*!
 * \brief If the \p TVM_LOG_DEBUG build flag is enabled, and the containing file has been enabled
 * at \p level or greater in the \p TVM_LOG_DEBUG environment variable, then log a message at
 * \p INFO severity.
 *
 * See \p VerboseLoggingEnabled for the format of the \p TVM_LOG_DEBUG environment variable.
 * Thread safe. No-op with no execution overhead if the \p TVM_LOG_DEBUG build flag is not enabled.
 * No-op with some execution overhead if the \p TVM_LOG_DEBUG build flag is enabled but the
 * containing file is not enabled.
 */
#define VLOG(level)                                                                     \
    DLOG_IF(INFO, ::litetvm::runtime::detail::VerboseLoggingEnabled(__FILE__, (level))) \
            << ::litetvm::runtime::detail::ThreadLocalVLogContext::Get()->str()

#if TVM_LOG_DEBUG
#define DCHECK(x) CHECK(x)
#define DCHECK_LT(x, y) CHECK((x) < (y))
#define DCHECK_GT(x, y) CHECK((x) > (y))
#define DCHECK_LE(x, y) CHECK((x) <= (y))
#define DCHECK_GE(x, y) CHECK((x) >= (y))
#define DCHECK_EQ(x, y) CHECK((x) == (y))
#define DCHECK_NE(x, y) CHECK((x) != (y))
#else
#define DCHECK(x) \
    while (false) CHECK(x)
#define DCHECK_LT(x, y) \
    while (false) CHECK((x) < (y))
#define DCHECK_GT(x, y) \
    while (false) CHECK((x) > (y))
#define DCHECK_LE(x, y) \
    while (false) CHECK((x) <= (y))
#define DCHECK_GE(x, y) \
    while (false) CHECK((x) >= (y))
#define DCHECK_EQ(x, y) \
    while (false) CHECK((x) == (y))
#define DCHECK_NE(x, y) \
    while (false) CHECK((x) != (y))
#endif

#define TVM_ICHECK_INDENT "  "

#define ICHECK_BINARY_OP(name, op, x, y)                                     \
    if (auto __tvm__log__err = ::litetvm::runtime::detail::LogCheck##name(x, y)) \
    ::litetvm::runtime::detail::LogFatal(__FILE__, __LINE__).stream()            \
            << "InternalError: Check failed: " << #x " " #op " " #y << *__tvm__log__err << ": "

#define ICHECK(x)                                                 \
    if (!(x))                                                     \
    ::litetvm::runtime::detail::LogFatal(__FILE__, __LINE__).stream() \
            << "InternalError: Check failed: (" #x << ") is false: "

#define ICHECK_LT(x, y) ICHECK_BINARY_OP(_LT, <, x, y)
#define ICHECK_GT(x, y) ICHECK_BINARY_OP(_GT, >, x, y)
#define ICHECK_LE(x, y) ICHECK_BINARY_OP(_LE, <=, x, y)
#define ICHECK_GE(x, y) ICHECK_BINARY_OP(_GE, >=, x, y)
#define ICHECK_EQ(x, y) ICHECK_BINARY_OP(_EQ, ==, x, y)
#define ICHECK_NE(x, y) ICHECK_BINARY_OP(_NE, !=, x, y)
#define ICHECK_NOTNULL(x)                                                           \
    ((x) == nullptr ? ::litetvm::runtime::detail::LogFatal(__FILE__, __LINE__).stream() \
                              << "InternalError: Check not null: " #x << ' ',       \
     (x) : (x))// NOLINT(*)

}// namespace runtime
}// namespace litetvm
#endif//LITETVM_RUNTIME_LOGGING_H
