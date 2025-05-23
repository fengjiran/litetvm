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
    PackedArgs::Fill(anys.data(), 1, 1.5, "hello", 5, 3.14);
    for (const auto& it: anys) {
        std::cout << it.GetTypeKey() << std::endl;
    }

    using types = std::tuple<int, float, double, std::string>;
    static_assert(std::is_same_v<float, std::tuple_element_t<1, types>>);
}