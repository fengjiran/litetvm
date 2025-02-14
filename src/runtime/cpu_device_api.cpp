//
// Created by richard on 2/5/25.
//

#include "runtime/device_api.h"
#include "runtime/registry.h"
#include "runtime/thread_local.h"
#include "runtime/workspace_pool.h"

#include <iostream>

#ifdef __ANDROID__
#include <android/api-level.h>
#endif

#if defined(__linux__) || defined(__ANDROID__)
#include <sys/sysinfo.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <sys/sysctl.h>
#endif


namespace litetvm::runtime {

class CPUDeviceAPI final : public DeviceAPI {
public:
    static CPUDeviceAPI* Global() {
        // NOTE: explicitly use new to avoid exit-time destruction of global state,
        // Global state will be recycled by OS as the process exits.
        // static auto* inst = new CPUDeviceAPI();
        static CPUDeviceAPI inst;
        return &inst;
    }

    void SetDevice(Device dev) override {}

    void GetAttr(Device dev, DeviceAttrKind kind, TVMRetValue* rv) override {
        if (kind == DeviceAttrKind::kExist) {
            *rv = 1;
        }

        switch (kind) {
            case DeviceAttrKind::kExist:
                break;
            case DeviceAttrKind::kTotalGlobalMemory: {
#if defined(__linux__) || defined(__ANDROID__)
                struct sysinfo info;
                if (sysinfo(&info) == 0) {
                    *rv = static_cast<int64_t>(info.totalram) * info.mem_unit;// Convert to bytes
                } else {
                    *rv = -1;
                }
#elif defined(_WIN32)
                MEMORYSTATUSEX statex;
                statex.dwLength = sizeof(statex);
                if (GlobalMemoryStatusEx(&statex)) {
                    *rv = static_cast<int64_t>(statex.ullTotalPhys);// Total physical memory in bytes
                } else {
                    *rv = -1;
                }
#elif defined(__APPLE__)
                int64_t mem;
                size_t size = sizeof(mem);
                if (sysctlbyname("hw.memsize", &mem, &size, nullptr, 0) == 0) {
                    *rv = mem;
                } else {
                    *rv = -1;
                }
#else
                *rv = -1;
#endif
            }
            default:
                break;
        }
    }

    void* AllocDataSpace(Device dev, size_t nbytes, size_t alignment, DLDataType type_hint) override {
        void* ptr;
#if _MSC_VER
        ptr = _aligned_malloc(nbytes, alignment);
        if (ptr == nullptr) throw std::bad_alloc();
#elif defined(__ANDROID__) && __ANDROID_API__ < 17
        ptr = memalign(alignment, nbytes);
        if (ptr == nullptr) throw std::bad_alloc();
#else
        // posix_memalign is available in android ndk since __ANDROID_API__ >= 17
        int ret = posix_memalign(&ptr, alignment, nbytes);
        if (ret != 0) throw std::bad_alloc();
#endif
        std::cout << "allocate " << nbytes << " bytes memory, alignment = " << alignment << std::endl;
        return ptr;
    }

    void FreeDataSpace(Device dev, void* ptr) override {
#if _MSC_VER
        _aligned_free(ptr);
#else
        free(ptr);
#endif
        std::cout << "free memory.\n";
    }

    void StreamSync(Device dev, TVMStreamHandle stream) override {}

    void* AllocWorkspace(Device dev, size_t size, DLDataType type_hint) override;

    void FreeWorkspace(Device dev, void* data) override;

    bool SupportsDevicePointerArithmeticsOnHost() override { return true; }

    CPUDeviceAPI(const CPUDeviceAPI&) = delete;
    CPUDeviceAPI& operator=(const CPUDeviceAPI&) = delete;

private:
    void CopyDataFromTo(const void* from, size_t from_offset, void* to, size_t to_offset, size_t size,
                        Device dev_from, Device dev_to, DLDataType type_hint,
                        TVMStreamHandle stream) override {
        memcpy(static_cast<char*>(to) + to_offset, static_cast<const char*>(from) + from_offset, size);
    }

    CPUDeviceAPI() = default;
};

struct CPUWorkspacePool : WorkspacePool {
    CPUWorkspacePool() : WorkspacePool(DLDeviceType::kDLCPU, CPUDeviceAPI::Global()) {}
};

void* CPUDeviceAPI::AllocWorkspace(Device dev, size_t size, DLDataType type_hint) {
    return ThreadLocalStore<CPUWorkspacePool>::Get()->AllocWorkspace(dev, size);
}

void CPUDeviceAPI::FreeWorkspace(Device dev, void* data) {
    ThreadLocalStore<CPUWorkspacePool>::Get()->FreeWorkspace(dev, data);
}

TVM_REGISTER_GLOBAL("device_api.cpu").set_body([](TVMArgs args, TVMRetValue* rv) {
    DeviceAPI* ptr = CPUDeviceAPI::Global();
    *rv = ptr;
});

}// namespace litetvm::runtime