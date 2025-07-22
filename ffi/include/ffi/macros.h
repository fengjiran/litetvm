//
// Created by 赵丹 on 25-7-22.
//

#ifndef LITETVM_FFI_MACROS_H
#define LITETVM_FFI_MACROS_H

#if defined(_MSC_VER)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#ifdef ERROR
#undef ERROR
#endif

#endif

#if defined(_MSC_VER)
#define TVM_FFI_INLINE [[msvc::forceinline]] inline
#else
#define TVM_FFI_INLINE [[gnu::always_inline]] inline
#endif

/*!
 * \brief Macro helper to force a function not to be inlined.
 * It is only used in places that we know not inlining is good,
 * e.g. some logging functions.
 */
#if defined(_MSC_VER)
#define TVM_FFI_NO_INLINE [[msvc::noinline]]
#else
#define TVM_FFI_NO_INLINE [[gnu::noinline]]
#endif

#if defined(_MSC_VER)
#define TVM_FFI_UNREACHABLE() __assume(false)
#else
#define TVM_FFI_UNREACHABLE() __builtin_unreachable()
#endif

// Macros to do weak linking
#ifdef _MSC_VER
#define TVM_FFI_WEAK __declspec(selectany)
#else
#define TVM_FFI_WEAK __attribute__((weak))
#endif

#if !defined(TVM_FFI_DLL) && defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define TVM_FFI_DLL EMSCRIPTEN_KEEPALIVE
#define TVM_FFI_DLL_EXPORT EMSCRIPTEN_KEEPALIVE
#endif
#if !defined(TVM_FFI_DLL) && defined(_MSC_VER)
#ifdef TVM_FFI_EXPORTS
#define TVM_FFI_DLL __declspec(dllexport)
#else
#define TVM_FFI_DLL __declspec(dllimport)
#endif
#define TVM_FFI_DLL_EXPORT __declspec(dllexport)
#endif
#ifndef TVM_FFI_DLL
#define TVM_FFI_DLL __attribute__((visibility("default")))
#define TVM_FFI_DLL_EXPORT __attribute__((visibility("default")))
#endif


/*! \brief helper macro to suppress unused warning */
#define TVM_FFI_ATTRIBUTE_UNUSED [[maybe_unused]]

#define TVM_FFI_STR_CONCAT_(__x, __y) __x##__y
#define TVM_FFI_STR_CONCAT(__x, __y) TVM_FFI_STR_CONCAT_(__x, __y)

#if defined(__GNUC__) || defined(__clang__)
#define TVM_FFI_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define TVM_FFI_FUNC_SIG __FUNCSIG__
#else
#define TVM_FFI_FUNC_SIG __func__
#endif

#define TVM_FFI_STATIC_INIT_BLOCK_VAR_DEF \
    TVM_FFI_ATTRIBUTE_UNUSED static inline int __##TVMFFIStaticInitReg

/*! \brief helper macro to run code once during initialization */
#define TVM_FFI_STATIC_INIT_BLOCK(Body) \
    TVM_FFI_STR_CONCAT(TVM_FFI_STATIC_INIT_BLOCK_VAR_DEF, __COUNTER__) = []() { Body return 0; }()


#ifdef __has_cpp_attribute
#if __has_cpp_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif

#if __has_cpp_attribute(maybe_unused)
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define MAYBE_UNUSED
#endif
#endif

#define UNUSED(expr)   \
    do {               \
        (void) (expr); \
    } while (false)

/*!
 * \brief Marks the API as extra c++ api that is defined in cc files.
 *
 * These APIs are extra features that depend on, but are not required to
 * support essential core functionality, such as function calling and object
 * access.
 *
 * They are implemented in cc files to reduce compile-time overhead.
 * The input/output only uses POD/Any/ObjectRef for ABI stability.
 * However, these extra APIs may have an issue across MSVC/Itanium ABI,
 *
 * Related features are also available through reflection based function
 * that is fully based on C API
 *
 * The project aims to minimize the number of extra C++ APIs and only
 * restrict the use to non-core functionalities.
 */
#ifndef TVM_FFI_EXTRA_CXX_API
#define TVM_FFI_EXTRA_CXX_API TVM_FFI_DLL
#endif


#endif//LITETVM_FFI_MACROS_H
