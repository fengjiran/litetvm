//
// Created by richard on 7/3/24.
//

#include "runtime/object.h"
#include "runtime/data_type.h"
#include "runtime/type_context.h"

#include <string>

namespace litetvm::runtime {

uint32_t Object::GetOrAllocRuntimeTypeIndex(const std::string& key, uint32_t static_tindex,
                                            uint32_t parent_tindex, uint32_t type_child_slots,
                                            bool type_child_slots_can_overflow) {
    return TypeContext::Global().GetOrAllocRuntimeTypeIndex(
            key, static_tindex, parent_tindex, type_child_slots, type_child_slots_can_overflow);
}

bool Object::DerivedFrom(uint32_t parent_tindex) const {
    return TypeContext::Global().DerivedFrom(type_index_, parent_tindex);
}

std::string Object::TypeIndex2Key(uint32_t tindex) {
    return TypeContext::Global().TypeIndex2Key(tindex);
}

size_t Object::TypeIndex2KeyHash(uint32_t tindex) {
    return TypeContext::Global().TypeIndex2KeyHash(tindex);
}

uint32_t Object::TypeKey2Index(const std::string& key) {
    return TypeContext::Global().TypeKey2Index(key);
}

}// namespace litetvm::runtime