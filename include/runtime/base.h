//
// Created by richard on 2/4/25.
//

#ifndef LITETVM_RUNTIME_BASE_H
#define LITETVM_RUNTIME_BASE_H

#include "ffi/c_api.h"

// TVM version
#define TVM_VERSION "0.21.dev0"

// define extra macros for TVM DLL exprt
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define TVM_DLL EMSCRIPTEN_KEEPALIVE
#endif

// helper macro to suppress unused warning
#if defined(__GNUC__)
#define TVM_ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define TVM_ATTRIBUTE_UNUSED
#endif

#ifndef TVM_DLL
#ifdef _WIN32
#ifdef TVM_EXPORTS
#define TVM_DLL __declspec(dllexport)
#else
#define TVM_DLL __declspec(dllimport)
#endif
#else
#define TVM_DLL __attribute__((visibility("default")))
#endif
#endif


#endif//LITETVM_RUNTIME_BASE_H
