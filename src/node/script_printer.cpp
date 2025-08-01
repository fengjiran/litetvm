//
// Created by richard on 8/2/25.
//

#include "node/script_printer.h"
#include "ffi/function.h"
#include "ffi/reflection/registry.h"
#include "node/repr_printer.h"
#include "node/reflection.h"

#include <algorithm>

namespace litetvm {


TVM_FFI_STATIC_INIT_BLOCK({ PrinterConfigNode::RegisterReflection(); });

TVMScriptPrinter::FType& TVMScriptPrinter::vtable() {
    static FType inst;
    return inst;
}

std::string TVMScriptPrinter::Script(const ObjectRef& node, const Optional<PrinterConfig>& cfg) {
    if (!TVMScriptPrinter::vtable().can_dispatch(node)) {
        std::ostringstream os;
        ReprPrinter printer(os);
        printer.Print(node);
        return os.str();
    }
    return TVMScriptPrinter::vtable()(node, cfg.value_or(PrinterConfig()));
}

bool IsIdentifier(const std::string& name) {
    // Python identifiers follow the regex: "^[a-zA-Z_][a-zA-Z0-9_]*$"
    // `std::regex` would cause a symbol conflict with PyTorch, we avoids to use it in the codebase.
    //
    // We convert the regex into following conditions:
    // 1. The name is not empty.
    // 2. The first character is either an alphabet or an underscore.
    // 3. The rest of the characters are either an alphabet, a digit or an underscore.
    return name.size() > 0 &&                          //
           (std::isalpha(name[0]) || name[0] == '_') &&//
           std::all_of(name.begin() + 1, name.end(),
                       [](char c) { return std::isalnum(c) || c == '_'; });
}

PrinterConfig::PrinterConfig(Map<String, Any> config_dict) {
    runtime::ObjectPtr<PrinterConfigNode> n = make_object<PrinterConfigNode>();
    if (auto v = config_dict.Get("name")) {
        n->binding_names.push_back(Downcast<String>(v.value()));
    }
    if (auto v = config_dict.Get("show_meta")) {
        n->show_meta = v.value().cast<bool>();
    }
    if (auto v = config_dict.Get("ir_prefix")) {
        n->ir_prefix = Downcast<String>(v.value());
    }
    if (auto v = config_dict.Get("tir_prefix")) {
        n->tir_prefix = Downcast<String>(v.value());
    }
    if (auto v = config_dict.Get("relax_prefix")) {
        n->relax_prefix = Downcast<String>(v.value());
    }
    if (auto v = config_dict.Get("module_alias")) {
        n->module_alias = Downcast<String>(v.value());
    }
    if (auto v = config_dict.Get("buffer_dtype")) {
        n->buffer_dtype = DataType(StringToDLDataType(Downcast<String>(v.value())));
    }
    if (auto v = config_dict.Get("int_dtype")) {
        n->int_dtype = DataType(StringToDLDataType(Downcast<String>(v.value())));
    }
    if (auto v = config_dict.Get("float_dtype")) {
        n->float_dtype = DataType(StringToDLDataType(Downcast<String>(v.value())));
    }
    if (auto v = config_dict.Get("verbose_expr")) {
        n->verbose_expr = v.value().cast<bool>();
    }
    if (auto v = config_dict.Get("indent_spaces")) {
        n->indent_spaces = v.value().cast<int>();
    }
    if (auto v = config_dict.Get("print_line_numbers")) {
        n->print_line_numbers = v.value().cast<bool>();
    }
    if (auto v = config_dict.Get("num_context_lines")) {
        n->num_context_lines = v.value().cast<int>();
    }
    if (auto v = config_dict.Get("path_to_underline")) {
        n->path_to_underline = Downcast<Optional<Array<ObjectPath>>>(v).value_or(Array<ObjectPath>());
    }
    if (auto v = config_dict.Get("path_to_annotate")) {
        n->path_to_annotate =
                Downcast<Optional<Map<ObjectPath, String>>>(v).value_or(Map<ObjectPath, String>());
    }
    if (auto v = config_dict.Get("obj_to_underline")) {
        n->obj_to_underline = Downcast<Optional<Array<ObjectRef>>>(v).value_or(Array<ObjectRef>());
    }
    if (auto v = config_dict.Get("obj_to_annotate")) {
        n->obj_to_annotate =
                Downcast<Optional<Map<ObjectRef, String>>>(v).value_or(Map<ObjectRef, String>());
    }
    if (auto v = config_dict.Get("syntax_sugar")) {
        n->syntax_sugar = v.value().cast<bool>();
    }
    if (auto v = config_dict.Get("show_object_address")) {
        n->show_object_address = v.value().cast<bool>();
    }
    if (auto v = config_dict.Get("show_all_struct_info")) {
        n->show_all_struct_info = v.value().cast<bool>();
    }

    // Checking prefixes if they are valid Python identifiers.
    CHECK(IsIdentifier(n->ir_prefix)) << "Invalid `ir_prefix`: " << n->ir_prefix;
    CHECK(IsIdentifier(n->tir_prefix)) << "Invalid `tir_prefix`: " << n->tir_prefix;
    CHECK(IsIdentifier(n->relax_prefix)) << "Invalid `relax_prefix`: " << n->relax_prefix;
    CHECK(n->module_alias.empty() || IsIdentifier(n->module_alias))
            << "Invalid `module_alias`: " << n->module_alias;

    this->data_ = std::move(n);
}

Array<String> PrinterConfigNode::GetBuiltinKeywords() {
    Array<String> result{this->ir_prefix, this->tir_prefix, this->relax_prefix};
    if (!this->module_alias.empty()) {
        result.push_back(this->module_alias);
    }
    return result;
}

TVM_REGISTER_NODE_TYPE(PrinterConfigNode);
TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef()
            .def("node.PrinterConfig",
                 [](Map<String, Any> config_dict) { return PrinterConfig(config_dict); })
            .def("node.TVMScriptPrinterScript", TVMScriptPrinter::Script);
});

}// namespace litetvm
