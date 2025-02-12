//
// Created by 赵丹 on 25-2-12.
//

#ifndef BACK_TRACE_H
#define BACK_TRACE_H

#include <dmlc/common.h>
#include <dmlc/thread_local.h>
#include <iomanip>

namespace litetvm {
namespace runtime {

/*!
 * \brief Generate a backtrace when called.
 * \return A multiline string of the backtrace. There will be either one or two lines per frame.
 */
std::string Backtrace();

/*! \brief Base error type for TVM. Wraps a string message. */
class Error : public ::dmlc::Error {// for backwards compatibility
public:
    /*!
   * \brief Construct an error.
   * \param s The message to be displayed with the error.
   */
    explicit Error(const std::string& s) : ::dmlc::Error(s) {}
};

/*!
 * \brief Error message already set in frontend env.
 *
 *  This error can be thrown by EnvCheckSignals to indicate
 *  that there is an error set in the frontend environment(e.g.
 *  python interpreter). The TVM FFI should catch this error
 *  and return a proper code tell the frontend caller about
 *  this fact.
 */
class EnvErrorAlreadySet : public ::dmlc::Error {
public:
    /*!
   * \brief Construct an error.
   * \param s The message to be displayed with the error.
   */
    explicit EnvErrorAlreadySet(const std::string& s) : ::dmlc::Error(s) {}
};

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
   * \param time The time at which the error occurred. This should be in local time.
   * \param backtrace Backtrace from when the error occurred.
   */
    InternalError(std::string file, int lineno, std::string message,
                  std::time_t time = std::time(nullptr), std::string backtrace = Backtrace())
        : Error(""),
          file_(file),
          lineno_(lineno),
          message_(message),
          time_(time),
          backtrace_(backtrace) {
        std::ostringstream s;
        // XXX: Do not change this format, otherwise all error handling in python will break (because it
        // parses the message to reconstruct the error type).
        // TODO(tkonolige): Convert errors to Objects, so we can avoid the mess of formatting/parsing
        // error messages correctly.
        s << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] " << file << ":" << lineno
          << ": " << message << std::endl;
        if (backtrace.size() > 0) {
            s << backtrace << std::endl;
        }
        full_message_ = s.str();
    }
    /*! \return The file in which the error occurred. */
    const std::string& file() const { return file_; }
    /*! \return The message associated with this error. */
    const std::string& message() const { return message_; }
    /*! \return Formatted error message including file, linenumber, backtrace, and message. */
    const std::string& full_message() const { return full_message_; }
    /*! \return The backtrace from where this error occurred. */
    const std::string& backtrace() const { return backtrace_; }
    /*! \return The time at which this error occurred. */
    const std::time_t& time() const { return time_; }
    /*! \return The line number at which this error occurred. */
    int lineno() const { return lineno_; }
    virtual const char* what() const noexcept { return full_message_.c_str(); }

private:
    std::string file_;
    int lineno_;
    std::string message_;
    std::time_t time_;
    std::string backtrace_;
    std::string full_message_;// holds the full error string
};

}// namespace runtime
}// namespace litetvm

#endif//BACK_TRACE_H
