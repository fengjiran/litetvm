//
// Created by 赵丹 on 25-2-26.
//

#ifndef CLOSURE_H
#define CLOSURE_H

#include "runtime/base.h"

namespace litetvm {
namespace runtime {

/*!
 * \brief An object representing a closure. This object is used by both the
 * Relay VM and interpreter.
 */
class ClosureObj : public Object {
public:
    static constexpr uint32_t _type_index = static_cast<uint32_t>(TypeIndex::kRuntimeClosure);
    static constexpr const char* _type_key = "runtime.Closure";
    TVM_DECLARE_BASE_OBJECT_INFO(ClosureObj, Object);
};

/*! \brief reference to closure. */
class Closure : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(Closure, ObjectRef, ClosureObj);
};

}// namespace runtime
}// namespace litetvm

#endif//CLOSURE_H
