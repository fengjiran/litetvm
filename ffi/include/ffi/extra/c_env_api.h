//
// Created by 赵丹 on 2025/8/19.
//

#ifndef LITETVM_FFI_EXTRA_C_ENV_API_H
#define LITETVM_FFI_EXTRA_C_ENV_API_H

#include "ffi/c_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
     * \brief FFI function to lookup a function from a module's imports.
     *
     * This is a helper function that is used by generated code.
     *
     * \param library_ctx The library context module handle.
     * \param func_name The name of the function.
     * \param out The result function.
     * \note The returned function is a weak reference that is cached/owned by the module.
     * \return 0 when no error is thrown, -1 when failure happens
     */
TVM_FFI_DLL int TVMFFIEnvLookupFromImports(TVMFFIObjectHandle library_ctx, const char* func_name,
                                           TVMFFIObjectHandle* out);

/*
     * \brief Register a symbol value that will be initialized when a library with the symbol is loaded.
     *
     * This function can be used to make context functions to be available in the library
     * module that wants to avoid an explicit link dependency
     *
     * \param name The name of the symbol.
     * \param symbol The symbol to register.
     * \return 0 when success, nonzero when failure happens
     */
TVM_FFI_DLL int TVMFFIEnvRegisterContextSymbol(const char* name, void* symbol);

/*!
     * \brief Register a symbol that will be initialized when a system library is loaded.
     *
     * \param name The name of the symbol.
     * \param symbol The symbol to register.
     * \return 0 when success, nonzero when failure happens
     */
TVM_FFI_DLL int TVMFFIEnvRegisterSystemLibSymbol(const char* name, void* symbol);

#ifdef __cplusplus
}// extern "C"
#endif

#endif//LITETVM_FFI_EXTRA_C_ENV_API_H
