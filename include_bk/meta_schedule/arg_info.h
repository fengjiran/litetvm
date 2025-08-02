//
// Created by 赵丹 on 25-4-17.
//

#ifndef LITETVM_META_SCHEDULE_ARG_INFO_H
#define LITETVM_META_SCHEDULE_ARG_INFO_H

#include "ir/module.h"
#include "node/reflection.h"
#include "runtime/data_type.h"
#include "runtime/object.h"
#include "runtime/shape_tuple.h"
#include "tir/function.h"

namespace litetvm {
namespace meta_schedule {

/*! \brief The argument information. */
class ArgInfoNode : public Object {
public:
    static constexpr const char* _type_key = "meta_schedule.ArgInfo";
    TVM_DECLARE_BASE_OBJECT_INFO(ArgInfoNode, runtime::Object);

    /*! \brief Default destructor. */
    virtual ~ArgInfoNode() = default;
    /*! \brief Converts the ArgInfo to its corresponding JSON representation. */
    virtual ObjectRef AsJSON() const = 0;
};

/*!
 * \brief Managed reference to ArgInfoNode
 * \sa ArgInfoNode
 */
class ArgInfo : public ObjectRef {
public:
    /*!
   * \brief Parse the argument information from a JSON object.
   * \param json_obj The json object to parse.
   * \return The argument information parsed.
   */
    LITETVM_API static ArgInfo FromJSON(const ObjectRef& json_obj);
    /*!
   * \brief Extract a list of the argument information from PrimFunc.
   * \param func The PrimFunc to get argument information from.
   * \return An array of the argument information derived.
   */
    LITETVM_API static Array<ArgInfo, void> FromPrimFunc(const tir::PrimFunc& func);
    /*!
   * \brief Extract a list of the argument information from the entry func of an IRModule
   * \param mod The IRModule to extract argument information from.
   * \param remove_preproc Whether to remove the preprocessing blocks.
   * \return An array of the argument information derived.
   */
    LITETVM_API static Array<ArgInfo, void> FromEntryFunc(const IRModule& mod, bool remove_preproc);

    TVM_DEFINE_MUTABLE_NOTNULLABLE_OBJECT_REF_METHODS(ArgInfo, runtime::ObjectRef, ArgInfoNode);

protected:
    ArgInfo() = default;
};

/*! \brief The tensor argument information. */
class TensorInfoNode : public ArgInfoNode {
public:
    /*! \brief The data type of the tensor. */
    runtime::DataType dtype;
    /*! \brief The shape of the tensor. */
    runtime::ShapeTuple shape;

    void VisitAttrs(AttrVisitor* v) {
        v->Visit("dtype", &dtype);
        v->Visit("shape", &shape);
    }

    static constexpr const char* _type_key = "meta_schedule.TensorInfo";
    TVM_DECLARE_FINAL_OBJECT_INFO(TensorInfoNode, ArgInfoNode);

    ObjectRef AsJSON() const;
};

/*!
 * \brief Managed reference to TensorInfoNode
 * \sa TensorInfoNode
 */
class TensorInfo : public ArgInfo {
public:
    /*!
   * \brief Constructor of TensorInfo.
   * \param dtype The data type of the tensor argument.
   * \param shape The shape tuple of the tensor argument.
   */
    LITETVM_API explicit TensorInfo(runtime::DataType dtype, runtime::ShapeTuple shape);
    /*!
   * \brief Parse the argument information from a JSON object.
   * \param json_obj The json object to parse.
   * \return The argument information parsed.
   */
    LITETVM_API static TensorInfo FromJSON(const ObjectRef& json_obj);
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(TensorInfo, ArgInfo, TensorInfoNode);
};

}// namespace meta_schedule
}// namespace litetvm

#endif//LITETVM_META_SCHEDULE_ARG_INFO_H
