//
// Created by 赵丹 on 25-2-6.
//

#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <random>

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

template<typename T = float,
         typename = std::enable_if_t<std::is_floating_point_v<T>>>
std::vector<T> GenRandomMatrix(size_t m, size_t n, T low = 0, T high = 1) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<T> matrix(m * n);
    std::uniform_real_distribution<T> dist(low, high);
    for (size_t i = 0; i < m * n; ++i) {
        matrix[i] = dist(gen);
    }
    return matrix;
}

template<typename T,
         typename = void,
         typename = std::enable_if_t<std::is_integral_v<T>>>
std::vector<T> GenRandomMatrix(size_t m, size_t n, T low = 0, T high = 10) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<T> matrix(m * n);
    std::uniform_int_distribution<T> dist(low, high);
    for (size_t i = 0; i < m * n; ++i) {
        matrix[i] = dist(gen);
    }
    return matrix;
}

inline void print() {
    std::cout << std::endl;
}

template<typename T, typename... Args>
void print(const T& t, const Args&... args) {
    std::cout << t << (sizeof...(Args) == 0 ? "" : ", ");
    print(args...);
}

#endif//UTILS_H
