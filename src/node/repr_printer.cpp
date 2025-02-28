//
// Created by 赵丹 on 25-2-28.
//

#include "node/repr_printer.h"
#include "runtime/registry.h"

namespace litetvm {

void ReprPrinter::Print(const ObjectRef& node) {
    static const FType& f = vtable();
    if (!node.defined()) {
        stream << "(nullptr)";
    } else {
        if (f.can_dispatch(node)) {
            f(node, this);
        } else {
            // default value, output type key and addr.
            stream << node->GetTypeKey() << "(" << node.get() << ")";
        }
    }
}

void ReprPrinter::PrintIndent() const {
    for (int i = 0; i < indent; ++i) {
        stream << ' ';
    }
}

ReprPrinter::FType& ReprPrinter::vtable() {
    static FType inst;
    return inst;
}

void ReprLegacyPrinter::Print(const ObjectRef& node) {
    static const FType& f = vtable();
    if (!node.defined()) {
        stream << "(nullptr)";
    } else if (f.can_dispatch(node)) {
        f(node, this);
    } else {
        try {
            stream << node;// Use ReprPrinter
        } catch (const Error& e) {
            LOG(WARNING) << "ReprPrinter fails";
            stream << node->GetTypeKey() << '(' << node.get() << ')';
        }
    }
}

bool ReprLegacyPrinter::CanDispatch(const ObjectRef& node) {
    static const FType& f = vtable();
    return !node.defined() || f.can_dispatch(node);
}

void ReprLegacyPrinter::PrintIndent() const {
    for (int i = 0; i < indent; ++i) {
        stream << ' ';
    }
}

ReprLegacyPrinter::FType& ReprLegacyPrinter::vtable() {
    static FType inst;
    return inst;
}

void Dump(const ObjectRef& n) { std::cerr << n << "\n"; }

void Dump(const Object* n) {
    Dump(runtime::GetRef<ObjectRef>(n));
}

TVM_REGISTER_GLOBAL("node.AsRepr").set_body_typed([](const ObjectRef& obj) {
    std::ostringstream os;
    os << obj;
    return os.str();
});

TVM_REGISTER_GLOBAL("node.AsLegacyRepr").set_body_typed(runtime::AsLegacyRepr);

}// namespace litetvm
