//
// Created by 赵丹 on 25-2-5.
//

#ifndef LITETVM_RUNTIME_BOXED_PRIMITIVE_H
#define LITETVM_RUNTIME_BOXED_PRIMITIVE_H

#include "runtime/memory.h"
#include "runtime/object.h"

namespace litetvm::runtime {
namespace detail {
/* \brief Provide the BoxNode<T> type traits in templated contexts
 *
 * The Box<T> class is used in many templated contexts, and is easier
 * to have templated over the primitive type.
 *
 * However, much of the TVM type system depends on classes having a
 * unique name.  For example, the use of `Object::IsInstance` depends
 * on `Object::GetOrAllocRuntimeTypeIndex`.  Any duplicate names will
 * result in duplicate indices, and invalid downcasting.  Furthermore,
 * the name must be specified in the Python FFI using
 * `tvm._ffi.register_object`.  This prevents use of
 * `typeid(T)::name()` to build a unique name, as the name is not
 * required to be human-readable or consistent across compilers.
 *
 * This utility struct should be specialized over the primitive type
 * held by the box, to allow explicit listing of the `_type_key` and
 * other similar tratis.
 *
 * Note: This should only contain traits that are required at runtime,
 * and should *not* contain extensions for features that are only
 * available at compile-time.  For integration with compile-time-only
 * functionality (e.g. StructuralHash, StructuralEqual), see
 * `BoxNodeCompileTimeTraits` in `src/node/boxed_primitive.cc`.
 */
template<typename Prim>
struct BoxNodeRuntimeTraits;

}// namespace detail

template<typename Prim>
class BoxNode : public Object {
public:
    /*! \brief Constructor
   *
   * \param value The value to be boxed
   */
    explicit BoxNode(Prim value) : value(value) {}

    /*! \brief The boxed value */
    Prim value;

    static constexpr const char* _type_key = detail::BoxNodeRuntimeTraits<Prim>::_type_key;
    static constexpr bool _type_has_method_visit_attrs = false;
    TVM_DECLARE_FINAL_OBJECT_INFO(BoxNode, Object);
};

template<typename Prim>
class Box : public ObjectRef {
public:
    /*! \brief Constructor
   *
   * \param value The value to be boxed
   */
    Box(Prim value) : ObjectRef(make_object<BoxNode<Prim>>(value)) {}// NOLINT(*)

    operator Prim() const {
        return (*this)->value;
    }

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(Box, ObjectRef, BoxNode<Prim>);
};

/*! \brief Boxed version of C++ int64_t
 *
 * Can be used to store POD integer values as a TVM ObjectRef.  Used
 * for FFI handling, and for storing POD types inside TVM containers.
 */
using Int = Box<int64_t>;

/*! \brief Boxed version of C++ double
 *
 * Can be used to store POD floating-point values as a TVM ObjectRef.
 * Used for FFI handling, and for storing POD types inside TVM
 * containers.
 */
using Float = Box<double>;

/*! \brief Boxed version of C++ bool
 *
 * Can be used to store POD boolean values as a TVM ObjectRef.  Used
 * for FFI handling, and for storing POD types inside TVM containers.
 *
 * When passing from Python to C++, TVM PackedFunc conversion follow
 * C++ conversion rules, and allow bool->int and int->bool
 * conversions.  When passing from C++ to Python, the types are
 * returned as bool or int.  If the C++ function uses ObjectRef to
 * hold the object, a Python to C++ to Python round trip will preserve
 * the distinction between bool and int.
 */
using Bool = Box<bool>;

namespace detail {
template<>
struct BoxNodeRuntimeTraits<int64_t> {
    static constexpr const char* _type_key = "runtime.BoxInt";
};

template<>
struct BoxNodeRuntimeTraits<double> {
    static constexpr const char* _type_key = "runtime.BoxFloat";
};

template<>
struct BoxNodeRuntimeTraits<bool> {
    static constexpr const char* _type_key = "runtime.BoxBool";
};
}// namespace detail


}// namespace litetvm::runtime

#endif//LITETVM_RUNTIME_BOXED_PRIMITIVE_H
