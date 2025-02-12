//
// Created by 赵丹 on 25-1-22.
//

#ifndef TYPE_CONTEXT_H
#define TYPE_CONTEXT_H

#include "data_type.h"
#include "object.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace litetvm::runtime {

/**
 * @brief Type information.
 */
struct TypeInfo {
    /// \brief name of the type.
    std::string name;

    /// \brief hash of the name.
    size_t name_hash{0};

    /// \brief The current type index in type table.
    uint32_t index{0};

    /// \brief Parent index in type table.
    uint32_t parent_index{0};

    // NOTE: the indices in [index, index + num_reserved_slots) are
    // reserved for the child-class of this type.
    /// \brief Total number of slots reserved for the type and its children.
    uint32_t num_slots{0};

    /// \brief number of allocated child slots.
    uint32_t allocated_slots{0};

    /// \brief Whether child can overflow.
    bool child_slots_can_overflow{true};
};

class TypeContext {
public:
    static TypeContext& Global() {
        static TypeContext instance;
        return instance;
    }

    uint32_t GetOrAllocRuntimeTypeIndex(const std::string& skey, uint32_t static_tindex,
                                        uint32_t parent_tindex, uint32_t num_child_slots,
                                        bool child_slots_can_overflow);

    // NOTE: this is a relatively slow path for child checking
    // Most types are already checked by the fast-path via reserved slot checking.
    bool DerivedFrom(uint32_t child_tindex, uint32_t parent_tindex);

    void Dump(int min_children_count);

    uint32_t TypeKey2Index(const std::string& skey) {
        CHECK(type_key2index_.find(skey) != type_key2index_.end())
                << "Cannot find type " << skey
                << ". Did you forget to register the node by TVM_REGISTER_NODE_TYPE ?";
        return type_key2index_[skey];
    }

    std::string TypeIndex2Key(uint32_t tindex) {
        std::lock_guard lock(mutex_);
        if (tindex != 0) {
            // always return the right type key for root
            // for non-root type nodes, allocated slots should not equal 0
            CHECK(tindex < type_table_.size() && type_table_[tindex].allocated_slots != 0)
                    << "Unknown type index " << tindex;
        }
        return type_table_[tindex].name;
    }

    size_t TypeIndex2KeyHash(uint32_t tindex) {
        std::lock_guard lock(mutex_);
        CHECK(tindex < type_table_.size() && type_table_[tindex].allocated_slots != 0)
                << "Unknown type index " << tindex;
        return type_table_[tindex].name_hash;
    }

private:
    TypeContext()
        : type_table_(static_cast<uint32_t>(TypeIndex::kStaticIndexEnd), TypeInfo()) {
        type_table_[0].name = "runtime.Object";
    }

    std::vector<TypeInfo> type_table_;
    std::unordered_map<std::string, uint32_t> type_key2index_;
    std::mutex mutex_{};// mutex to avoid registration from multiple threads.
    std::atomic<uint32_t> type_counter_ = static_cast<uint32_t>(TypeIndex::kStaticIndexEnd);
};

}// namespace litetvm::runtime

#endif//TYPE_CONTEXT_H
