//
// Created by 赵丹 on 25-3-4.
//

/*!
 * \file ir/attrs.h
 * \brief Helpers for attribute objects.
 *
 *  This module enables declaration of named attributes
 *  which support default value setup and bound checking.
 *
 * \code
 *   struct MyAttrs : public tvm::AttrsNode<MyAttrs> {
 *     float learning_rate;
 *     int num_hidden;
 *     String name;
 *     // declare attribute fields in header file
 *     TVM_DECLARE_ATTRS(MyAttrs, "attrs.MyAttrs") {
 *       TVM_ATTR_FIELD(num_hidden).set_lower_bound(1);
 *       TVM_ATTR_FIELD(learning_rate).set_default(0.01f);
 *       TVM_ATTR_FIELD(name).set_default("hello");
 *     }
 *   };
 *   // register it in cc file
 *   TVM_REGISTER_NODE_TYPE(MyAttrs);
 * \endcode
 *
 * \sa AttrsNode, TVM_DECLARE_ATTRS, TVM_ATTR_FIELD
 */
#ifndef ATTRS_H
#define ATTRS_H

#include "node/reflection.h"
#include "node/structural_equal.h"
#include "node/structural_hash.h"
#include "runtime/packed_func.h"

#include <dmlc/common.h>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace litetvm {

using runtime::PackedFunc;

/*!
 * \brief Declare an attribute function.
 * \param ClassName The name of the class.
 * \param TypeKey The type key to be used by the TVM node system.
 */
#define TVM_DECLARE_ATTRS(ClassName, TypeKey)                          \
    static constexpr const char* _type_key = TypeKey;                  \
    TVM_DECLARE_FINAL_OBJECT_INFO(ClassName, ::litetvm::BaseAttrsNode) \
    template<typename FVisit>                                          \
    void _tvm_VisitAttrs(FVisit& _tvm_fvisit)// NOLINT(*)

/*!
 * \brief Declare an attribute field.
 * \param FieldName The field name.
 */
#define TVM_ATTR_FIELD(FieldName) _tvm_fvisit(#FieldName, &FieldName)


/*!
 * \brief Create a NodeRef type that represents null.
 * \tparam TObjectRef the type to be created.
 * \return A instance that will represent None.
 */
template<typename TObjectRef>
TObjectRef NullValue() {
    static_assert(TObjectRef::_type_is_nullable, "Can only get NullValue for nullable types");
    return TObjectRef(ObjectPtr<Object>(nullptr));
}

template<>
inline DataType NullValue<DataType>() {
    return DataType(static_cast<int>(DataType::TypeCode::kHandle), 0, 0);
}

/*! \brief Error thrown during attribute checking. */
struct AttrError : Error {
    /*!
     * \brief constructor
     * \param msg error message
     */
    explicit AttrError(std::string msg) : Error("AttributeError:" + msg) {}
};

/*!
 * \brief Information about attribute fields in string representations.
 */
class AttrFieldInfoNode : public Object {
public:
    /*! \brief name of the field */
    String name;
    /*! \brief type docstring information in str. */
    String type_info;
    /*! \brief detailed description of the type */
    String description;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("name", &name);
        v->Visit("type_info", &type_info);
        v->Visit("description", &description);
    }

    static constexpr const char* _type_key = "AttrFieldInfo";
    static constexpr bool _type_has_method_sequal_reduce = false;
    static constexpr bool _type_has_method_shash_reduce = false;
    TVM_DECLARE_FINAL_OBJECT_INFO(AttrFieldInfoNode, Object);
};


/*! \brief AttrFieldInfo */
class AttrFieldInfo : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(AttrFieldInfo, ObjectRef, AttrFieldInfoNode);
};

/*!
 * \brief Base class of all attribute class
 * \note Do not subclass AttrBaseNode directly,
 *       subclass AttrsNode instead.
 * \sa AttrsNode
 */
class BaseAttrsNode : public Object {
public:
    using TVMArgs = runtime::TVMArgs;
    using TVMRetValue = runtime::TVMRetValue;
    /*! \brief virtual destructor */
    virtual ~BaseAttrsNode() {}
    // visit function
    virtual void VisitAttrs(AttrVisitor* v) {}
    /*!
     * \brief Initialize the attributes by sequence of arguments
     * \param args The positional arguments in the form
     *        [key0, value0, key1, value1, ..., key_n, value_n]
     */
    template<typename... Args>
    void InitBySeq(Args&&... args) {
        PackedFunc pf([this](const TVMArgs& args, TVMRetValue* rv) { this->InitByPackedArgs(args); });
        pf(std::forward<Args>(args)...);
    }

    /*!
     * \brief Print readible docstring to ostream, add newline.
     * \param os the stream to print the docstring to.
     */
    void PrintDocString(std::ostream& os) const {
        Array<AttrFieldInfo> entry = this->ListFieldInfo();
        for (AttrFieldInfo info: entry) {
            os << info->name << " : " << info->type_info << '\n';
            if (info->description.length() != 0) {
                os << "    " << info->description << '\n';
            }
        }
    }

    /*!
     * \brief Visit attributes that do not equal the default value.
     *
     * \note This is useful to extract fields for concise printing.
     * \param v The visitor
     */
    LITETVM_API virtual void VisitNonDefaultAttrs(AttrVisitor* v) = 0;
    /*!
     * \brief Get the field information
     * \return The fields in the Attrs.
     */
    LITETVM_API virtual Array<AttrFieldInfo> ListFieldInfo() const = 0;
    /*!
     * \brief Initialize the attributes by arguments.
     * \param kwargs The key value pairs for initialization.
     *        [key0, value0, key1, value1, ..., key_n, value_n]
     * \param allow_unknown Whether allow additional unknown fields.
     * \note This function throws when the required field is not present.
     */
    LITETVM_API virtual void InitByPackedArgs(const TVMArgs& kwargs, bool allow_unknown = false) = 0;

    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    static constexpr const char* _type_key = "Attrs";
    TVM_DECLARE_BASE_OBJECT_INFO(BaseAttrsNode, Object);
};

/*!
 * \brief Managed reference to BaseAttrsNode.
 * \sa AttrsNode, BaseAttrsNode
 */
class Attrs : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(Attrs, ObjectRef, BaseAttrsNode);
};

}// namespace litetvm

#endif//ATTRS_H
