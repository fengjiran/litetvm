//
// Created by 赵丹 on 25-2-6.
//

#ifndef UTILS_H
#define UTILS_H

#if defined(__APPLE__)
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#ifdef __has_cpp_attribute
#if __has_cpp_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif
#endif

/*!
 * \brief Macro helper to force a function not to be inlined.
 * It is only used in places that we know not inlining is good,
 * e.g. some logging functions.
 */
#if defined(_MSC_VER)
#define TVM_NO_INLINE __declspec(noinline)
#else
#define TVM_NO_INLINE __attribute__((noinline))
#endif

/*!
 * \brief Macro helper to force a function to be inlined.
 * It is only used in places that we know inline is important,
 * e.g. some template expansion cases.
 */
#ifdef _MSC_VER
#define TVM_ALWAYS_INLINE __forceinline
#else
#define TVM_ALWAYS_INLINE inline __attribute__((always_inline))
#endif

/*!
 * \brief Use little endian for binary serialization
 *  if this is set to 0, use big endian.
 */
#ifndef IO_USE_LITTLE_ENDIAN
#define IO_USE_LITTLE_ENDIAN 1
#endif

// #define LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)

/*! \brief whether serialize using little endian */
#define IO_NO_ENDIAN_SWAP (LITTLE_ENDIAN == IO_USE_LITTLE_ENDIAN)


#endif //UTILS_H
