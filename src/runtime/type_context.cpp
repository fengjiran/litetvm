//
// Created by 赵丹 on 25-1-22.
//

#include "runtime/type_context.h"
#include <iostream>
#include <ranges>

namespace litetvm::runtime {

uint32_t TypeContext::GetOrAllocRuntimeTypeIndex(const std::string& skey,
                                                 uint32_t static_tindex,
                                                 uint32_t parent_tindex,
                                                 uint32_t num_child_slots,
                                                 bool child_slots_can_overflow) {
    std::lock_guard lock(mutex_);
    if (type_key2index_.find(skey) != type_key2index_.end()) {
        return type_key2index_[skey];
    }

    // try to allocate from parent's type table.
    CHECK_LT(parent_tindex, type_table_.size()) << " skey=" << skey << ", static_tindex=" << static_tindex;
    auto& pinfo = type_table_[parent_tindex];
    CHECK_EQ(pinfo.index, parent_tindex);

    bool parent_child_slots_can_overflow = pinfo.child_slots_can_overflow;

    // total number of slots include the type itself.
    uint32_t num_slots = num_child_slots + 1;
    uint32_t allocated_tindex;
    if (static_tindex != static_cast<uint32_t>(TypeIndex::kDynamic)) {
        // statically assigned type
        VLOG(3) << "TypeIndex[" << static_tindex << "]: static: " << skey << ", parent "
                << type_table_[parent_tindex].name;
        allocated_tindex = static_tindex;
        CHECK_LT(allocated_tindex, type_table_.size());
        CHECK_EQ(type_table_[allocated_tindex].allocated_slots, 0U)
                << "Conflicting static index " << static_tindex << " between "
                << type_table_[allocated_tindex].name << " and " << skey;
    } else if (pinfo.allocated_slots + num_slots <= pinfo.num_slots) {
        // allocate the slot from parent's reserved pool
        allocated_tindex = parent_tindex + pinfo.allocated_slots;
        VLOG(3) << "TypeIndex[" << allocated_tindex << "]: dynamic: " << skey << ", parent "
                << type_table_[parent_tindex].name;
        // update parent's state
        pinfo.allocated_slots += num_slots;
    } else {
        VLOG(3) << "TypeIndex[" << type_counter_ << "]: dynamic (overflow): " << skey << ", parent "
                << type_table_[parent_tindex].name;
        CHECK(pinfo.child_slots_can_overflow)
                << "Reach maximum number of sub-classes for " << pinfo.name;
        // allocate new entries.
        allocated_tindex = type_counter_;
        type_counter_ += num_slots;
        CHECK_LE(type_table_.size(), type_counter_);
        type_table_.resize(type_counter_, TypeInfo());
    }

    CHECK_GT(allocated_tindex, parent_tindex);
    // initialize the slot.
    type_table_[allocated_tindex].index = allocated_tindex;
    type_table_[allocated_tindex].parent_index = parent_tindex;
    type_table_[allocated_tindex].num_slots = num_slots;
    type_table_[allocated_tindex].allocated_slots = 1;
    // if parent cannot overflow, then this class cannot.
    type_table_[allocated_tindex].child_slots_can_overflow = parent_child_slots_can_overflow && child_slots_can_overflow;
    type_table_[allocated_tindex].name = skey;
    type_table_[allocated_tindex].name_hash = std::hash<std::string>()(skey);

    // update the key2index mapping.
    type_key2index_[skey] = allocated_tindex;

    return allocated_tindex;
}

bool TypeContext::DerivedFrom(uint32_t child_tindex, uint32_t parent_tindex) {
    // invariance: child's type index is always bigger than its parent.
    if (child_tindex < parent_tindex) {
        return false;
    }

    if (child_tindex == parent_tindex) {
        return true;
    }

    {
        std::lock_guard lock(mutex_);
        CHECK(child_tindex < type_table_.size());
        while (child_tindex > parent_tindex) {
            child_tindex = type_table_[child_tindex].parent_index;
        }
    }
    return child_tindex == parent_tindex;
}

void TypeContext::Dump(int min_children_count) {
    // reverse accumulation so we can get total counts in a bottom-up manner.
    std::vector<int> num_children(type_table_.size(), 0);
    // for (auto it = type_table_.rbegin(); it != type_table_.rend(); ++it) {
    //     if (it->index != 0) {
    //         num_children[it->parent_index] += num_children[it->index] + 1;
    //     }
    // }

    for (auto& it: std::ranges::reverse_view(type_table_)) {
        if (it.index != 0) {
            num_children[it.parent_index] += num_children[it.index] + 1;
        }
    }

    for (const auto& info: type_table_) {
        if (info.index != 0 && num_children[info.index] >= min_children_count) {
            std::cerr << std::format("[{:02}] {:<30}\tparent={:25}\tnum_child_slots={:<5}\tnum_children={}\n",
                                     info.index, info.name, type_table_[info.parent_index].name, info.num_slots - 1, num_children[info.index]);
            // std::cerr << '[' << info.index << "] " << info.name
            //           << "\tparent=" << type_table_[info.parent_index].name
            //           << "\tnum_child_slots=" << info.num_slots - 1
            //           << "\tnum_children=" << num_children[info.index] << std::endl;
        }
    }
}

}// namespace litetvm::runtime