//
// Created by richard on 8/2/25.
//

#include "ffi/extra/structural_hash.h"
#include "ffi/extra/base64.h"
#include "ffi/function.h"
#include "ffi/reflection/registry.h"
#include "node/functor.h"
#include "node/node.h"
#include "node/structural_hash.h"
#include "runtime/profiling.h"
#include "support/base64.h"
#include "support/str_escape.h"
#include "support/utils.h"
#include "target/codegen.h"

#include <algorithm>
#include <dmlc/memory_io.h>
#include <unordered_map>

namespace litetvm {

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef().def("node.StructuralHash",
                          [](const Any& object, bool map_free_vars) -> int64_t {
                              return ffi::StructuralHash::Hash(object, map_free_vars);
                          });
    refl::TypeAttrDef<runtime::ModuleNode>()
            .def("__data_to_json__",
                 [](const runtime::ModuleNode* node) {
                     std::string bytes = codegen::SerializeModuleToBytes(GetRef<runtime::Module>(node),
                                                                         /*export_dso*/ false);
                     return ffi::Base64Encode(ffi::Bytes(bytes));
                 })
            .def("__data_from_json__", [](const String& base64_bytes) {
                Bytes bytes = ffi::Base64Decode(base64_bytes);
                runtime::Module rtmod = codegen::DeserializeModuleFromBytes(bytes.operator std::string());
                return rtmod;
            });

    refl::TypeAttrDef<runtime::NDArray::Container>()
            .def("__data_to_json__",
                 [](const runtime::NDArray::Container* node) {
                     std::string blob;
                     dmlc::MemoryStringStream mstrm(&blob);
                     support::Base64OutStream b64strm(&mstrm);
                     runtime::SaveDLTensor(&b64strm, node);
                     b64strm.Finish();
                     return String(blob);
                 })
            .def("__data_from_json__", [](const std::string& blob) {
                dmlc::MemoryStringStream mstrm(const_cast<std::string*>(&blob));
                support::Base64InStream b64strm(&mstrm);
                b64strm.InitPosition();
                runtime::NDArray temp;
                ICHECK(temp.Load(&b64strm));
                return temp;
            });
});

uint64_t StructuralHash::operator()(const ffi::Any& object) const {
    return ffi::StructuralHash::Hash(object, false);
}

struct RefToObjectPtr : public ObjectRef {
    static ObjectPtr<Object> Get(const ObjectRef& ref) {
        return ffi::details::ObjectUnsafe::ObjectPtrFromObjectRef<Object>(ref);
    }
};

struct ReportNodeTrait {
    static void RegisterReflection() {
        namespace refl = ffi::reflection;
        refl::ObjectDef<runtime::profiling::ReportNode>()
                .def_ro("calls", &runtime::profiling::ReportNode::calls)
                .def_ro("device_metrics", &runtime::profiling::ReportNode::device_metrics)
                .def_ro("configuration", &runtime::profiling::ReportNode::configuration);
    }
};

TVM_FFI_STATIC_INIT_BLOCK({ ReportNodeTrait::RegisterReflection(); });

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<runtime::profiling::ReportNode>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const runtime::profiling::ReportNode*>(node.get());
            p->stream << op->AsTable();
        });

struct CountNodeTrait {
    static void RegisterReflection() {
        namespace refl = litetvm::ffi::reflection;
        refl::ObjectDef<runtime::profiling::CountNode>().def_ro("value",
                                                                &runtime::profiling::CountNode::value);
    }
};

TVM_FFI_STATIC_INIT_BLOCK({ CountNodeTrait::RegisterReflection(); });

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<runtime::profiling::CountNode>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const runtime::profiling::CountNode*>(node.get());
            p->stream << op->GetTypeKey() << "(" << op->value << ")";
        });

struct DurationNodeTrait {
    static void RegisterReflection() {
        namespace refl = litetvm::ffi::reflection;
        refl::ObjectDef<runtime::profiling::DurationNode>().def_ro(
                "microseconds", &runtime::profiling::DurationNode::microseconds);
    }
};

TVM_FFI_STATIC_INIT_BLOCK({ DurationNodeTrait::RegisterReflection(); });

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<runtime::profiling::DurationNode>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const runtime::profiling::DurationNode*>(node.get());
            p->stream << op->GetTypeKey() << "(" << op->microseconds << ")";
        });

struct PercentNodeTrait {
    static void RegisterReflection() {
        namespace refl = litetvm::ffi::reflection;
        refl::ObjectDef<runtime::profiling::PercentNode>().def_ro(
                "percent", &runtime::profiling::PercentNode::percent);
    }
};

TVM_FFI_STATIC_INIT_BLOCK({ PercentNodeTrait::RegisterReflection(); });

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<runtime::profiling::PercentNode>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const runtime::profiling::PercentNode*>(node.get());
            p->stream << op->GetTypeKey() << "(" << op->percent << ")";
        });

struct RatioNodeTrait {
    static void RegisterReflection() {
        namespace refl = litetvm::ffi::reflection;
        refl::ObjectDef<runtime::profiling::RatioNode>().def_ro("ratio",
                                                                &runtime::profiling::RatioNode::ratio);
    }
};

TVM_FFI_STATIC_INIT_BLOCK({ RatioNodeTrait::RegisterReflection(); });

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<runtime::profiling::RatioNode>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const runtime::profiling::RatioNode*>(node.get());
            p->stream << op->GetTypeKey() << "(" << op->ratio << ")";
        });

}// namespace litetvm
