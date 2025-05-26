//
// Created by 赵丹 on 25-5-20.
//

// #include "testing_object.h"
#include "ffi/object.h"
#include "ffi/memory.h"
#include "ffi/function.h"

#include <gtest/gtest.h>

using namespace litetvm::ffi;

// const std::ostream& operator<<(const std::ostream& os, const AnyView& src) {
//     std::cout << "type key: " << src.GetTypeKey() ;
//     return os;
// }

TEST(Function, for_each) {
    int n = 5;
    std::vector<AnyView> anys(n);
    // using types = std::tuple<int, double, const char*, int, double>;

    PackedArgs::Fill(anys.data(), 1, 1.5, "hello", 5, 3.14);
    for (const auto& it: anys) {
        std::cout << it.GetTypeKey() << std::endl;
    }

    EXPECT_EQ(anys[0].cast<int>(), 1);
    EXPECT_EQ(anys[1].cast<float>(), 1.5);
    EXPECT_EQ(anys[2].cast<const char*>(), "hello");
    EXPECT_EQ(anys[3].cast<int>(), 5);
    EXPECT_EQ(anys[4].cast<double>(), 3.14);
}