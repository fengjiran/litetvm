//
// Created by richard on 8/2/25.
//

#include "ffi/function.h"
#include "node/functor.h"
#include "node/repr_printer.h"

namespace litetvm {

// Container printer
TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<ffi::ArrayObj>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const ffi::ArrayObj*>(node.get());
            p->stream << '[';
            for (size_t i = 0; i < op->size(); ++i) {
                if (i != 0) {
                    p->stream << ", ";
                }
                p->Print(op->at(i));
            }
            p->stream << ']';
        });

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<ffi::MapObj>([](const ObjectRef& node, ReprPrinter* p) {
            auto* op = static_cast<const ffi::MapObj*>(node.get());
            p->stream << '{';
            for (auto it = op->begin(); it != op->end(); ++it) {
                if (it != op->begin()) {
                    p->stream << ", ";
                }
                if (auto opt_str = it->first.as<ffi::String>()) {
                    p->stream << '\"' << opt_str.value() << "\": ";
                } else {
                    p->Print(it->first);
                    p->stream << ": ";
                }
                p->Print(it->second);
            }
            p->stream << '}';
        });

TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
        .set_dispatch<ffi::ShapeObj>([](const ObjectRef& node, ReprPrinter* p) {
            p->stream << ffi::Downcast<ffi::Shape>(node);
        });
}// namespace litetvm