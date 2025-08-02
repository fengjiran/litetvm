//
// Created by richard on 8/2/25.
//

#include "../../ffi/include/ffi/extra/structural_equal.h"
#include "ffi/function.h"
#include "ffi/reflection/access_path.h"
#include "ffi/reflection/registry.h"
#include "ir/module.h"
#include "node/functor.h"
#include "node/node.h"
#include "node/object_path.h"
#include "node/reflection.h"
#include "node/structural_equal.h"

#include <optional>
#include <unordered_map>

namespace litetvm {

TVM_REGISTER_OBJECT_TYPE(ObjectPathPairNode);

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef()
            .def("node.ObjectPathPairLhsPath",
                 [](const ObjectPathPair& object_path_pair) { return object_path_pair->lhs_path; })
            .def("node.ObjectPathPairRhsPath",
                 [](const ObjectPathPair& object_path_pair) { return object_path_pair->rhs_path; });
});

ObjectPathPairNode::ObjectPathPairNode(ObjectPath lhs_path, ObjectPath rhs_path)
    : lhs_path(std::move(lhs_path)), rhs_path(std::move(rhs_path)) {}

ObjectPathPair::ObjectPathPair(ObjectPath lhs_path, ObjectPath rhs_path) {
    data_ = make_object<ObjectPathPairNode>(std::move(lhs_path), std::move(rhs_path));
}

Optional<ObjectPathPair> ObjectPathPairFromAccessPathPair(
        Optional<ffi::reflection::AccessPathPair> src) {
    if (!src.has_value()) return std::nullopt;
    auto translate_path = [](ffi::reflection::AccessPath path) {
        ObjectPath result = ObjectPath::Root();
        for (const auto& step: path) {
            switch (step->kind) {
                case ffi::reflection::AccessKind::kObjectField: {
                    result = result->Attr(step->key.cast<String>());
                    break;
                }
                case ffi::reflection::AccessKind::kArrayIndex: {
                    result = result->ArrayIndex(step->key.cast<int64_t>());
                    break;
                }
                case ffi::reflection::AccessKind::kMapKey: {
                    result = result->MapValue(step->key);
                    break;
                }
                case ffi::reflection::AccessKind::kArrayIndexMissing: {
                    result = result->MissingArrayElement(step->key.cast<int64_t>());
                    break;
                }
                case ffi::reflection::AccessKind::kMapKeyMissing: {
                    result = result->MissingMapEntry();
                    break;
                }
                default: {
                    LOG(FATAL) << "Invalid access path kind: " << static_cast<int>(step->kind);
                    break;
                }
            }
        }
        return result;
    };

    return ObjectPathPair(translate_path((*src).get<0>()), translate_path((*src).get<1>()));
}

bool NodeStructuralEqualAdapter(const Any& lhs, const Any& rhs, bool assert_mode,
                                bool map_free_vars) {
    if (assert_mode) {
        auto first_mismatch = ObjectPathPairFromAccessPathPair(
                ffi::reflection::StructuralEqual::GetFirstMismatch(lhs, rhs, map_free_vars));
        if (first_mismatch.has_value()) {
            std::ostringstream oss;
            oss << "StructuralEqual check failed, caused by lhs";
            oss << " at " << (*first_mismatch)->lhs_path;
            {
                // print lhs
                PrinterConfig cfg;
                cfg->syntax_sugar = false;
                cfg->path_to_underline.push_back((*first_mismatch)->lhs_path);
                // The TVMScriptPrinter::Script will fallback to Repr printer,
                // if the root node to print is not supported yet,
                // e.g. Relax nodes, ArrayObj, MapObj, etc.
                oss << ":" << std::endl
                    << TVMScriptPrinter::Script(lhs.cast<ObjectRef>(), cfg);
            }
            oss << std::endl
                << "and rhs";
            {
                // print rhs
                oss << " at " << (*first_mismatch)->rhs_path;
                {
                    PrinterConfig cfg;
                    cfg->syntax_sugar = false;
                    cfg->path_to_underline.push_back((*first_mismatch)->rhs_path);
                    // The TVMScriptPrinter::Script will fallback to Repr printer,
                    // if the root node to print is not supported yet,
                    // e.g. Relax nodes, ArrayObj, MapObj, etc.
                    oss << ":" << std::endl
                        << TVMScriptPrinter::Script(rhs.cast<ObjectRef>(), cfg);
                }
            }
            TVM_FFI_THROW(ValueError) << oss.str();
        }
        return true;
    } else {
        return ffi::reflection::StructuralEqual::Equal(lhs, rhs, map_free_vars);
    }
}

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef()
            .def("node.StructuralEqual", NodeStructuralEqualAdapter)
            .def("node.GetFirstStructuralMismatch",
                 [](const Any& lhs, const Any& rhs, bool map_free_vars) {
                     /*
              Optional<ObjectPathPair> first_mismatch;
              bool equal =
                  SEqualHandlerDefault(false, &first_mismatch, true).Equal(lhs, rhs, map_free_vars);
              ICHECK(equal == !first_mismatch.defined());
              return first_mismatch;
             */
                     return ObjectPathPairFromAccessPathPair(
                             ffi::reflection::StructuralEqual::GetFirstMismatch(lhs, rhs, map_free_vars));
                 });
});

bool StructuralEqual::operator()(const ffi::Any& lhs, const ffi::Any& rhs,
                                 bool map_free_params) const {
    return ffi::reflection::StructuralEqual::Equal(lhs, rhs, map_free_params);
}
}// namespace litetvm
