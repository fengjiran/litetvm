//
// Created by richard on 2/9/25.
//
#include "runtime/ndarray.h"
#include "runtime/utils.h"

#include <dmlc/memory_io.h>
#include <gtest/gtest.h>

#include <runtime/device_api.h>

using namespace litetvm::runtime;

TEST(NDArrayTest, Empty) {
    auto array = NDArray::Empty({5, 10}, DataType::Float(32), {DLDeviceType::kDLCPU});
    CHECK_EQ(array.use_count(), 1);
    CHECK(array.IsContiguous());
    std::cout << array.Shape() << std::endl;

    std::string bytes;
    dmlc::MemoryStringStream stream(&bytes);
    array.Save(&stream);

    auto* t = array.ToDLPack();
    CHECK_EQ(array.use_count(), 2);
    std::vector<int64_t> strides {10, 1};
    t->dl_tensor.strides = strides.data();
    CHECK(IsContiguous(t->dl_tensor));
    std::cout << GetDataSize(t->dl_tensor) << std::endl;

    t->deleter(t);
}

TEST(NDArrayTest, View) {
    auto array = NDArray::Empty({5, 10}, DataType::Float(32), {DLDeviceType::kDLCPU});
    CHECK_EQ(array.use_count(), 1);
    CHECK(array.IsContiguous());
    std::cout << array.Shape() << std::endl;

    auto t = array.CreateView({10, 5}, DataType::Float(32));
    CHECK_EQ(t.use_count(), 1);
    CHECK_EQ(array.use_count(), 2);
}

TEST(NDArrayTest, FromExternalDLTensor) {
    int m = 5;
    int n = 10;
    auto array = NDArray::Empty({m, n}, DataType::Float(32), {DLDeviceType::kDLCPU});
    const DLTensor* dptr = array.operator->();
    auto t = NDArray::FromExternalDLTensor(*dptr);
    CHECK(array->data == t->data);
}