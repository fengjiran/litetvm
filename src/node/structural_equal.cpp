//
// Created by richard on 8/2/25.
//

#include "ffi/extra/structural_equal.h"
#include "ffi/reflection/access_path.h"
#include "ir/module.h"
#include "node/functor.h"
#include "node/node.h"
#include "node/structural_equal.h"

#include <optional>
#include <unordered_map>

namespace litetvm {

bool NodeStructuralEqualAdapter(const Any& lhs, const Any& rhs, bool assert_mode,
                                bool map_free_vars) {
    if (assert_mode) {
        auto first_mismatch = ffi::StructuralEqual::GetFirstMismatch(lhs, rhs, map_free_vars);
        if (first_mismatch.has_value()) {
            std::ostringstream oss;
            oss << "StructuralEqual check failed, caused by lhs";
            oss << " at " << (*first_mismatch).get<0>();
            {
                // print lhs
                PrinterConfig cfg;
                cfg->syntax_sugar = false;
                cfg->path_to_underline.push_back((*first_mismatch).get<0>());
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
                oss << " at " << (*first_mismatch).get<1>();
                {
                    PrinterConfig cfg;
                    cfg->syntax_sugar = false;
                    cfg->path_to_underline.push_back((*first_mismatch).get<1>());
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
        return ffi::StructuralEqual::Equal(lhs, rhs, map_free_vars);
    }
}

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef()
            .def("node.StructuralEqual", NodeStructuralEqualAdapter)
            .def("node.GetFirstStructuralMismatch", ffi::StructuralEqual::GetFirstMismatch);
});

bool StructuralEqual::operator()(const ffi::Any& lhs, const ffi::Any& rhs,
                                 bool map_free_params) const {
    return ffi::StructuralEqual::Equal(lhs, rhs, map_free_params);
}
}// namespace litetvm
