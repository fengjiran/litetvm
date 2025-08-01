//
// Created by 赵丹 on 25-8-1.
//

#ifndef LITETVM_RUNTIME_INT_TUPLE_H
#define LITETVM_RUNTIME_INT_TUPLE_H

#include "ffi/container/shape.h"

namespace litetvm {
namespace runtime {

// We simply redirects to ffi::Shape, and ffi::ShapeObj
using IntTuple = ffi::Shape;
using IntTupleObj = ffi::ShapeObj;

}// namespace runtime
}// namespace litetvm

#endif//LITETVM_RUNTIME_INT_TUPLE_H
