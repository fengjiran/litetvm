//
// Created by 赵丹 on 2025/8/12.
//

#ifndef LITETVM_IR_SOURCE_MAP_H
#define LITETVM_IR_SOURCE_MAP_H

#include "ffi/reflection/registry.h"
#include "node/node.h"
#include "runtime/object.h"

#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace litetvm {

/*!
 * \brief The source name in the Span
 * \sa SourceNameNode, Span
 */
class SourceName;

/*!
 * \brief The name of a source fragment.
 */
class SourceNameNode : public Object {
public:
    /*! \brief The source name. */
    String name;

    static void RegisterReflection() {
        namespace refl = ffi::reflection;
        refl::ObjectDef<SourceNameNode>().def_ro("name", &SourceNameNode::name);
    }

    static constexpr TVMFFISEqHashKind _type_s_eq_hash_kind = kTVMFFISEqHashKindTreeNode;
    static constexpr const char* _type_key = "ir.SourceName";
    TVM_DECLARE_FINAL_OBJECT_INFO(SourceNameNode, Object);
};

}// namespace litetvm

#endif//LITETVM_IR_SOURCE_MAP_H
