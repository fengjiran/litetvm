//
// Created by 赵丹 on 25-7-22.
//

#ifndef LITETVM_FFI_REFLECTION_ACCESS_PATH_H
#define LITETVM_FFI_REFLECTION_ACCESS_PATH_H

#include "ffi/any.h"
#include "ffi/c_api.h"
#include "ffi/container/array.h"
#include "ffi/container/tuple.h"
#include "ffi/reflection/registry.h"

namespace litetvm {
namespace ffi {
namespace reflection {

enum class AccessKind : int32_t {
    kObjectField = 0,
    kArrayIndex = 1,
    kMapKey = 2,
    // the following two are used for error reporting when
    // the supposed access field is not available
    kArrayIndexMissing = 3,
    kMapKeyMissing = 4,
};

/*!
 * \brief Represent a single step in object field, map key, array index access.
 */
class AccessStepObj : public Object {
public:
    /*!
   * \brief The kind of the access pattern.
   */
    AccessKind kind;
    /*!
   * \brief The access key
   * \note for array access, it will always be integer
   *       for field access, it will be string
   */
    Any key;

    AccessStepObj(AccessKind kind, Any key) : kind(kind), key(key) {}

    static void RegisterReflection() {
        namespace refl = reflection;
        refl::ObjectDef<AccessStepObj>()
                .def_ro("kind", &AccessStepObj::kind)
                .def_ro("key", &AccessStepObj::key);
    }

    static constexpr const char* _type_key = "tvm.ffi.reflection.AccessStep";
    static constexpr TVMFFISEqHashKind _type_s_eq_hash_kind = kTVMFFISEqHashKindConstTreeNode;
    TVM_FFI_DECLARE_FINAL_OBJECT_INFO(AccessStepObj, Object);
};

/*!
 * \brief ObjectRef class of AccessStepObj.
 *
 * \sa AccessStepObj
 */
class AccessStep : public ObjectRef {
public:
    AccessStep(AccessKind kind, Any key) : ObjectRef(make_object<AccessStepObj>(kind, key)) {}

    static AccessStep ObjectField(String field_name) {
        return AccessStep(AccessKind::kObjectField, field_name);
    }

    static AccessStep ArrayIndex(int64_t index) {
        return AccessStep(AccessKind::kArrayIndex, index);
    }

    static AccessStep ArrayIndexMissing(int64_t index) {
        return AccessStep(AccessKind::kArrayIndexMissing, index);
    }

    static AccessStep MapKey(Any key) {
        return AccessStep(AccessKind::kMapKey, key);
    }

    static AccessStep MapKeyMissing(Any key) {
        return AccessStep(AccessKind::kMapKeyMissing, key);
    }

    TVM_FFI_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(AccessStep, ObjectRef, AccessStepObj);
};

using AccessPath = Array<AccessStep>;
using AccessPathPair = Tuple<AccessPath, AccessPath>;

}// namespace reflection
}// namespace ffi
}// namespace litetvm

#endif//LITETVM_FFI_REFLECTION_ACCESS_PATH_H
