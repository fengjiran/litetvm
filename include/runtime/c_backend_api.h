//
// Created by 赵丹 on 25-2-13.
//

#ifndef LITETVM_RUNTIME_C_BACKEND_API_H
#define LITETVM_RUNTIME_C_BACKEND_API_H

#include "runtime/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Backend function for modules to get function
 *  from its environment mod_node (its imports and global function).
 *  The user do should not call TVMFuncFree on func.
 *
 * \param mod_node The module handle.
 * \param func_name The name of the function.
 * \param out The result function.
 * \return 0 when no error is thrown, -1 when failure happens
 */
TVM_DLL int TVMBackendGetFuncFromEnv(void* mod_node, const char* func_name, TVMFFIObjectHandle* out);


/*!
 * \brief Backend function to register system-wide library symbol.
 *
 * \param name The name of the symbol
 * \param ptr The symbol address.
 * \return 0 when no error is thrown, -1 when failure happens
 */
TVM_DLL int TVMBackendRegisterSystemLibSymbol(const char* name, void* ptr);

/*!
     * \brief Backend function to allocate temporal workspace.
     *
     * \note The result allocated space is ensured to be aligned to kTempAllocaAlignment.
     *
     * \param nbytes The size of the space requested.
     * \param device_type The device type which the space will be allocated.
     * \param device_id The device id which the space will be allocated.
     * \param dtype_code_hint The type code of the array elements. Only used in
     * certain backends such as OpenGL.
     * \param dtype_bits_hint The type bits of the array elements. Only used in
     * certain backends such as OpenGL.
     * \return nullptr when error is thrown, a valid ptr if success
     */
TVM_DLL void* TVMBackendAllocWorkspace(int device_type, int device_id, uint64_t nbytes,
                                       int dtype_code_hint, int dtype_bits_hint);

/*!
     * \brief Backend function to free temporal workspace.
     *
     * \param ptr The result allocated space pointer.
     * \param device_type The device type which the space will be allocated.
     * \param device_id The device id which the space will be allocated.
     * \return 0 when no error is thrown, -1 when failure happens
     *
     * \sa TVMBackendAllocWorkspace
     */
TVM_DLL int TVMBackendFreeWorkspace(int device_type, int device_id, void* ptr);

/*!
 * \brief Environment for TVM parallel task.
 */
typedef struct {
    /*!
   * \brief Auxiliary used for synchronization
   */
    void* sync_handle;
    /*! \brief total amount of task */
    int32_t num_task;
} TVMParallelGroupEnv;


/*!
 * \brief The callback function to execute a parallel lambda
 * \param task_id the task id of the function.
 * \param penv The parallel environment backs the execution.
 * \param cdata The supporting closure data.
 */
typedef int (*FTVMParallelLambda)(int task_id, TVMParallelGroupEnv* penv, void* cdata);

/*!
     * \brief Backend function for running parallel jobs.
     *
     * \param flambda The parallel function to be launched.
     * \param cdata The closure data.
     * \param num_task Number of tasks to launch, can be 0, means launch
     *           with all available threads.
     *
     * \return 0 when no error is thrown, -1 when failure happens
     */
TVM_DLL int TVMBackendParallelLaunch(FTVMParallelLambda flambda, void* cdata, int num_task);

/*!
     * \brief BSP barrrier between parallel threads
     * \param task_id the task id of the function.
     * \param penv The parallel environment backs the execution.
     * \return 0 when no error is thrown, -1 when failure happens
     */
TVM_DLL int TVMBackendParallelBarrier(int task_id, TVMParallelGroupEnv* penv);

/*!
     * \brief Simple static initialization function.
     *  Run f once and set handle to be not null.
     *  This function is mainly used for test purpose.
     *
     * \param handle A global address to indicate f
     * \param f The function to be run
     * \param cdata The closure data to pass to the function.
     * \param nbytes Number of bytes in the closure data.
     * \return 0 when no error is thrown, -1 when failure happens
     */
TVM_DLL int TVMBackendRunOnce(void** handle, int (*f)(void*), void* cdata, int nbytes);

#ifdef __cplusplus
}// TVM_EXTERN_C
#endif
#endif//LITETVM_RUNTIME_C_BACKEND_API_H
