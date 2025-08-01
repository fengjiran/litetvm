//
// Created by richard on 2/5/25.
//

#include "workspace_pool.h"

#include <memory>

namespace litetvm::runtime {

// page size, 4KB
constexpr size_t kWorkspacePageSize = 4 << 10;


class WorkspacePool::Pool {
public:
    // constructor
    Pool() {
        // safe guard header on each list.
        Entry e;
        e.data = nullptr;
        e.size = 0;
        free_list_.push_back(e);
        allocated_.push_back(e);
    }
    // allocate from pool
    void* Alloc(Device dev, DeviceAPI* device, size_t nbytes) {
        // Allocate align to page.
        nbytes = (nbytes + (kWorkspacePageSize - 1)) / kWorkspacePageSize * kWorkspacePageSize;
        if (nbytes == 0) nbytes = kWorkspacePageSize;
        Entry e;
        DLDataType type;
        type.code = static_cast<uint8_t>(DLDataTypeCode::kDLUInt);
        type.bits = 8;
        type.lanes = 1;
        if (free_list_.size() == 2) {
            e = free_list_.back();
            free_list_.pop_back();
            if (e.size < nbytes) {
                // resize the page
                device->FreeDataSpace(dev, e.data);
                e.data = device->AllocDataSpace(dev, nbytes, kTempAllocaAlignment, type);
                e.size = nbytes;
            }
        } else if (free_list_.size() == 1) {
            e.data = device->AllocDataSpace(dev, nbytes, kTempAllocaAlignment, type);
            e.size = nbytes;
        } else {
            if (free_list_.back().size >= nbytes) {
                // find smallest fit
                auto it = free_list_.end() - 2;
                for (; it->size >= nbytes; --it) {
                }
                e = *(it + 1);
                free_list_.erase(it + 1);
            } else {
                // resize the page
                e = free_list_.back();
                free_list_.pop_back();
                device->FreeDataSpace(dev, e.data);
                e.data = device->AllocDataSpace(dev, nbytes, kTempAllocaAlignment, type);
                e.size = nbytes;
            }
        }
        allocated_.push_back(e);
        return e.data;
    }
    // free resource back to pool
    void Free(void* data) {
        Entry e;
        if (allocated_.back().data == data) {
            // quick path, last allocated.
            e = allocated_.back();
            allocated_.pop_back();
        } else {
            int index = static_cast<int>(allocated_.size()) - 2;
            for (; index > 0 && allocated_[index].data != data; --index) {
            }
            CHECK_GT(index, 0) << "trying to free things that has not been allocated";
            e = allocated_[index];
            allocated_.erase(allocated_.begin() + index);
        }
        if (free_list_.back().size < e.size) {
            free_list_.push_back(e);
        } else if (free_list_.size() == 2) {
            free_list_.push_back(free_list_.back());
            free_list_[1] = e;
        } else {
            size_t i = free_list_.size() - 1;
            free_list_.resize(free_list_.size() + 1);
            for (; e.size < free_list_[i].size; --i) {
                free_list_[i + 1] = free_list_[i];
            }
            free_list_[i + 1] = e;
        }
    }
    // Release all resources
    void Release(Device dev, DeviceAPI* device) {
        for (size_t i = 1; i < free_list_.size(); ++i) {
            device->FreeDataSpace(dev, free_list_[i].data);
        }
        free_list_.clear();
    }

private:
    /*! \brief a single entry in the pool */
    struct Entry {
        void* data;
        size_t size;
    };
    /*! \brief List of free items, sorted from small to big size */
    std::vector<Entry> free_list_;
    /*! \brief List of allocated items */
    std::vector<Entry> allocated_;
};

WorkspacePool::WorkspacePool(DLDeviceType device_type, DeviceAPI* device)
    : device_type_(device_type), device_(device) {}

WorkspacePool::~WorkspacePool() {
    for (size_t i = 0; i < array_.size(); ++i) {
        if (array_[i] != nullptr) {
            Device dev;
            dev.device_type = device_type_;
            dev.device_id = static_cast<int>(i);
            array_[i]->Release(dev, device_);
            delete array_[i];
        }
    }
}

void* WorkspacePool::AllocWorkspace(Device dev, size_t size) {
    if (static_cast<size_t>(dev.device_id) >= array_.size()) {
        array_.resize(dev.device_id + 1, nullptr);
    }
    if (array_[dev.device_id] == nullptr) {
        array_[dev.device_id] = new Pool();
    }
    return array_[dev.device_id]->Alloc(dev, device_, size);
}

void WorkspacePool::FreeWorkspace(Device dev, void* ptr) {
    CHECK(static_cast<size_t>(dev.device_id) < array_.size() && array_[dev.device_id] != nullptr);
    array_[dev.device_id]->Free(ptr);
}

}// namespace litetvm::runtime