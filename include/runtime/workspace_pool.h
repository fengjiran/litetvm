//
// Created by richard on 2/5/25.
//

#ifndef WORKSPACE_POOL_H
#define WORKSPACE_POOL_H

#include "runtime/device_api.h"

#include <vector>

namespace litetvm::runtime {

/*!
 * \brief A workspace pool to manage
 *
 *  \note We have the following assumption about backend temporal
 *   workspace allocation, and will optimize for such assumption,
 *   some of these assumptions can be enforced by the compiler.
 *
 *  - Only a few allocation will happen, and space will be released after use.
 *  - The release order is usually in reverse order of allocate
 *  - Repeative pattern of same allocations over different runs.
 */
class WorkspacePool {
public:
    /*!
   * \brief Create pool with specific device type and device.
   * \param device_type The device type.
   * \param device_api The device API.
   */
    WorkspacePool(DLDeviceType device_type, DeviceAPI* device_api);
    /*! \brief destructor */
    ~WorkspacePool();
    /*!
   * \brief Allocate temporal workspace.
   * \param dev The device of allocation.
   * \param size The size to be allocated.
   */
    void* AllocWorkspace(Device dev, size_t size);
    /*!
   * \brief Free temporal workspace in backend execution.
   *
   * \param dev The device of allocation.
   * \param ptr The pointer to be freed.
   */
    void FreeWorkspace(Device dev, void* ptr);

private:
    class Pool;
    /*! \brief pool of device local array */
    std::vector<Pool*> array_;
    /*! \brief device type this pool support */
    DLDeviceType device_type_;
    /*! \brief The device API */
    DeviceAPI* device_;
};

}// namespace litetvm::runtime

#endif//WORKSPACE_POOL_H
