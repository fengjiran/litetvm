//
// Created by richard on 2/26/25.
//

#include "node/object_path.h"
#include "runtime/memory.h"
#include "runtime/registry.h"

#include <algorithm>
#include <cstring>

using namespace litetvm::runtime;

namespace litetvm {

Optional<ObjectPath> ObjectPathNode::GetParent() const {
    if (parent_ == nullptr) {
        return NullOpt;
    }

    return Downcast<ObjectPath>(parent_);
}


}// namespace litetvm
