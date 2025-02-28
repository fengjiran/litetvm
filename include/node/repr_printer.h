//
// Created by 赵丹 on 25-2-28.
//

#ifndef REPR_PRINTER_H
#define REPR_PRINTER_H

#include "node/functor.h"
#include "node/script_printer.h"

#include <string>

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
    LITETVM_API void Print(const ObjectRef& node);
    /*! \brief Print indent to the stream */
    LITETVM_API void PrintIndent();
    // Allow registration to be printer.
    using FType = NodeFunctor<void(const ObjectRef&, ReprPrinter*)>;
    LITETVM_API static FType& vtable();
};

/*! \brief Legacy behavior of ReprPrinter. */
class ReprLegacyPrinter {
public:
    /*! \brief The indentation level. */
    int indent{0};

    explicit ReprLegacyPrinter(std::ostream& stream)// NOLINT(*)
        : stream(stream) {}

    /*! \brief The node to be printed. */
    LITETVM_API void Print(const ObjectRef& node);
    /*! \brief Print indent to the stream */
    LITETVM_API void PrintIndent();
    /*! \brief Could the LegacyPrinter dispatch the node */
    LITETVM_API static bool CanDispatch(const ObjectRef& node);
    /*! \brief Return the ostream it maintains */
    NODISCARD LITETVM_API std::ostream& Stream() const;
    // Allow registration to be printer.
    using FType = NodeFunctor<void(const ObjectRef&, ReprLegacyPrinter*)>;
    LITETVM_API static FType& vtable();

private:
    /*! \brief The output stream */
    std::ostream& stream;
};

/*!
 * \brief Dump the node to stderr, used for debug purposes.
 * \param node The input node
 */
LITETVM_API void Dump(const ObjectRef& node);

/*!
 * \brief Dump the node to stderr, used for debug purposes.
 * \param node The input node
 */
LITETVM_API void Dump(const Object* node);

}// namespace litetvm

#endif//REPR_PRINTER_H
