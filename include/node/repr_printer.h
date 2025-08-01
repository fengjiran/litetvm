//
// Created by richard on 8/2/25.
//

#ifndef LITETVM_NODE_REPR_PRINTER_H
#define LITETVM_NODE_REPR_PRINTER_H

#include "node/functor.h"
#include "node/script_printer.h"

namespace litetvm {

/*! \brief A printer class to print the AST/IR nodes. */
class ReprPrinter {
public:
    /*! \brief The output stream */
    std::ostream& stream;
    /*! \brief The indentation level. */
    int indent{0};

    explicit ReprPrinter(std::ostream& stream)// NOLINT(*)
        : stream(stream) {}

    /*! \brief The node to be printed. */
    TVM_DLL void Print(const ObjectRef& node);
    /*! \brief The node to be printed. */
    TVM_DLL void Print(const ffi::Any& node);
    /*! \brief Print indent to the stream */
    TVM_DLL void PrintIndent();
    // Allow registration to be printer.
    using FType = NodeFunctor<void(const ObjectRef&, ReprPrinter*)>;
    TVM_DLL static FType& vtable();
};

/*!
 * \brief Dump the node to stderr, used for debug purposes.
 * \param node The input node
 */
TVM_DLL void Dump(const runtime::ObjectRef& node);

/*!
 * \brief Dump the node to stderr, used for debug purposes.
 * \param node The input node
 */
TVM_DLL void Dump(const runtime::Object* node);
}// namespace litetvm

namespace litetvm {
namespace ffi {
// default print function for all objects
// provide in the runtime namespace as this is where objectref originally comes from.
inline std::ostream& operator<<(std::ostream& os, const ObjectRef& n) {// NOLINT(*)
    ReprPrinter(os).Print(n);
    return os;
}

// default print function for any
inline std::ostream& operator<<(std::ostream& os, const Any& n) {// NOLINT(*)
    ReprPrinter(os).Print(n);
    return os;
}

template<typename... V>
inline std::ostream& operator<<(std::ostream& os, const Variant<V...>& n) {// NOLINT(*)
    ReprPrinter(os).Print(Any(n));
    return os;
}

}// namespace ffi
}// namespace litetvm

#endif//LITETVM_NODE_REPR_PRINTER_H
