//
// Created by 赵丹 on 25-2-19.
//

#include "runtime/array.h"
#include "runtime/map.h"
#include "runtime/string.h"
#include "runtime/shape_tuple.h"

namespace litetvm::runtime {

// Array
TVM_REGISTER_OBJECT_TYPE(ArrayNode);
// String
TVM_REGISTER_OBJECT_TYPE(StringObj);
// Map
TVM_REGISTER_OBJECT_TYPE(MapNode);
// ShapeTuple
TVM_REGISTER_OBJECT_TYPE(ShapeTupleNode);

}
