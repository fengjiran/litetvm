//
// Created by richard on 8/2/25.
//

#include "node/repr_printer.h"
#include "ffi/function.h"
#include "ffi/reflection/registry.h"
#include "runtime/device_api.h"

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

void ReprPrinter::Print(const ffi::Any& node) {
    switch (node.type_index()) {
        case ffi::TypeIndex::kTVMFFINone: {
            stream << "(nullptr)";
            break;
        }
        case ffi::TypeIndex::kTVMFFIInt: {
            stream << node.cast<int64_t>();
            break;
        }
        case ffi::TypeIndex::kTVMFFIBool: {
            stream << node.cast<bool>();
            break;
        }
        case ffi::TypeIndex::kTVMFFIFloat: {
            stream << node.cast<double>();
            break;
        }
        case ffi::TypeIndex::kTVMFFIOpaquePtr: {
            stream << node.cast<void*>();
            break;
        }
        case ffi::TypeIndex::kTVMFFIDataType: {
            stream << node.cast<DataType>();
            break;
        }
        case ffi::TypeIndex::kTVMFFIDevice: {
            runtime::operator<<(stream, node.cast<Device>());
            break;
        }
        case ffi::TypeIndex::kTVMFFIObject: {
            Print(node.cast<ObjectRef>());
            break;
        }
        default: {
            if (auto opt_obj = node.as<ObjectRef>()) {
                Print(opt_obj.value());
            } else {
                stream << "Any(type_key=`" << node.GetTypeKey() << "`)";
            }
            break;
        }
    }
}

void ReprPrinter::PrintIndent() {
    for (int i = 0; i < indent; ++i) {
        stream << ' ';
    }
}

ReprPrinter::FType& ReprPrinter::vtable() {
    static FType inst;
    return inst;
}

void Dump(const runtime::ObjectRef& n) { std::cerr << n << "\n"; }

void Dump(const runtime::Object* n) { Dump(runtime::GetRef<runtime::ObjectRef>(n)); }

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef().def("node.AsRepr", [](ffi::Any obj) {
        std::ostringstream os;
        os << obj;
        return os.str();
    });
});
}// namespace litetvm
