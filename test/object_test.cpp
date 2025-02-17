//
// Created by 赵丹 on 25-1-21.
//

#include <gtest/gtest.h>

#include "runtime/meta_data.h"
#include "runtime/packed_func.h"
#include "runtime/registry.h"
#include "runtime/shape_tuple.h"
#include "runtime/type_context.h"

using namespace litetvm::runtime;

TEST(ObjectTest, object) {
    IntTuple t1 = {10, 3, 256, 256};
    CHECK(!t1.empty());
    CHECK_EQ(t1.size(), 4);
    CHECK_EQ(t1.front(), t1[0]);
    CHECK_EQ(t1.front(), t1.at(0));
    CHECK_EQ(t1.back(), t1[t1.size() - 1]);
    CHECK_EQ(t1.use_count(), 1);

    // std::cout << t1 << std::endl;
    print(t1);

    ShapeTuple t2;
    CHECK(t2.empty());
    CHECK_EQ(t2.size(), 0);
    CHECK_EQ(t2.use_count(), 1);

    ShapeTuple t3(t1);
    CHECK_EQ(t1.use_count(), t3.use_count());
    CHECK_EQ(t1.use_count(), 2);
    CHECK(metadata::MetadataBaseNode::_type_child_slots_can_overflow);

    TypeContext::Global().Dump(0);
}

TEST(ObjectTest, ListAllPackedFunc) {
    std::cout << std::format("List all registered packed functions:") << std::endl;
    for (const auto& name: RegistryManager::Global().ListNames()) {
        std::cout << name << std::endl;
    }
}
