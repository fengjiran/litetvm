//
// Created by 赵丹 on 25-4-15.
//

#ifndef LITETVM_IR_GLOBAL_INFO_H
#define LITETVM_IR_GLOBAL_INFO_H

#include "ir/expr.h"
#include "target/target.h"

namespace litetvm {

/*!
 * \brief Abstract label for an area of memory.
 */
using MemoryScope = String;

/*!
 * \brief GlobalInfo are globally static object that are referred by the IR itself.
 *        Base node for all global info that can appear in the IR
 */
class GlobalInfoNode : public Object {
public:
    static constexpr const char* _type_key = "GlobalInfo";
    static constexpr bool _type_has_method_sequal_reduce = true;
    static constexpr bool _type_has_method_shash_reduce = true;
    TVM_DECLARE_BASE_OBJECT_INFO(GlobalInfoNode, Object);
};

/*!
 * \brief Managed reference to GlobalInfoNode.
 * \sa GlobalInfoNode
 */
class GlobalInfo : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(GlobalInfo, ObjectRef, GlobalInfoNode);
};

/*!
 * \brief A global info subclass for virtual devices.
 */
class VDeviceNode : public GlobalInfoNode {
public:
    /*! \brief The \p Target describing how to compile for the virtual device. */
    Target target;
    /*! \brief The device identifier for the virtual device. This enables us to
   * differentiate between distinct devices with same Target, such as multiple GPUs.
   */
    int vdevice_id;
    MemoryScope memory_scope;
    void VisitAttrs(AttrVisitor* v) {
        v->Visit("target", &target);
        v->Visit("vdevice_id", &vdevice_id);
        v->Visit("memory_scope", &memory_scope);
    }

    LITETVM_API bool SEqualReduce(const VDeviceNode* other, SEqualReducer equal) const {
        return equal(target, other->target) && equal(vdevice_id, other->vdevice_id) &&
               equal(memory_scope, other->memory_scope);
    }

    LITETVM_API void SHashReduce(SHashReducer hash_reduce) const {
        hash_reduce(target);
        hash_reduce(vdevice_id);
        hash_reduce(memory_scope);
    }
    static constexpr const char* _type_key = "VDevice";
    TVM_DECLARE_FINAL_OBJECT_INFO(VDeviceNode, GlobalInfoNode);
};

/*!
 * \brief Managed reference to VDeviceNode.
 * \sa VDeviceNode
 */
class VDevice : public GlobalInfo {
public:
    LITETVM_API explicit VDevice(Target tgt, int dev_id, MemoryScope mem_scope);
    TVM_DEFINE_OBJECT_REF_METHODS(VDevice, GlobalInfo, VDeviceNode);
};

/*!
 * \brief A dummy global info sub-class for testing purpose.
 */
class DummyGlobalInfoNode : public GlobalInfoNode {
public:
    void VisitAttrs(AttrVisitor* v) {}
    static constexpr const char* _type_key = "DummyGlobalInfo";

    LITETVM_API bool SEqualReduce(const DummyGlobalInfoNode* other, SEqualReducer equal) const {
        return true;
    }

    LITETVM_API void SHashReduce(SHashReducer hash_reduce) const {}
    TVM_DECLARE_FINAL_OBJECT_INFO(DummyGlobalInfoNode, GlobalInfoNode);
};

/*!
 * \brief Managed reference to DummyGlobalInfoNode.
 * \sa DummyGlobalInfoNode
 */
class DummyGlobalInfo : public GlobalInfo {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(DummyGlobalInfo, GlobalInfo, DummyGlobalInfoNode);
};

}// namespace litetvm

#endif//LITETVM_IR_GLOBAL_INFO_H
