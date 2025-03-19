//
// Created by 赵丹 on 25-2-5.
//

#include "runtime/boxed_primitive.h"
#include "runtime/registry.h"

namespace litetvm {
namespace runtime {


TVM_REGISTER_OBJECT_TYPE(BoxNode<int64_t>);
TVM_REGISTER_OBJECT_TYPE(BoxNode<double>);
TVM_REGISTER_OBJECT_TYPE(BoxNode<bool>);

/* \brief Allow explicit construction of Box<bool>
 *
 * Convert a `bool` to `Box<bool>`.  For use in FFI handling, to
 * provide an umambiguous representation between `bool(true)` and
 * `int(1)`.  Will be automatically unboxed in the case where a
 * `Box<bool>` is provided to a PackedFunc that requires `int` input,
 * mimicking C++'s default conversions.
 *
 * This is only needed for Box<bool>, as Box<double> and Box<int64_t>
 * can be converted in C++ as part of `TVMArgValue::operator
 * ObjectRef()` without ambiguity, postponing conversions until
 * required.
 */
TVM_REGISTER_GLOBAL("runtime.BoxBool").set_body_typed([](bool value) { return Box(value); });

/* \brief Return the underlying boolean object.
 *
 * Used while unboxing a boolean return value during FFI handling.
 * The return type is intentionally `int` and not `bool`, to avoid
 * recursive unwrapping of boolean values.
 *
 * This is only needed for Box<bool>, as Box<double> and Box<int64_t>
 * can be unambiguously unboxed as part of
 * `TVMRetValue::operator=(ObjectRef)`.
 */
TVM_REGISTER_GLOBAL("runtime.UnBoxBool").set_body_typed([](Box<bool> obj) -> int {
    return obj->value;
});

}// namespace runtime
}// namespace litetvm
