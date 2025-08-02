//
// Created by richard on 8/2/25.
//

#ifndef LITETVM_NODE_NODE_H
#define LITETVM_NODE_NODE_H

#include "ffi/memory.h"
#include "node/reflection.h"
#include "node/repr_printer.h"
#include "runtime/base.h"
#include "runtime/object.h"

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace litetvm {

using ffi::Any;
using ffi::AnyView;
using ffi::Object;
using ffi::ObjectPtr;
using ffi::ObjectPtrEqual;
using ffi::ObjectPtrHash;
using ffi::ObjectRef;
using ffi::PackedArgs;
using ffi::TypeIndex;
using runtime::Downcast;
using runtime::GetRef;

}// namespace litetvm

#endif//LITETVM_NODE_NODE_H
