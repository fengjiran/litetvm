//
// Created by richard on 4/5/25.
//

#ifndef LITETVM_ARITH_ANALYZER_H
#define LITETVM_ARITH_ANALYZER_H

#include "arith/int_set.h"
#include "ir/expr.h"
#include "support/with.h"

#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace litetvm {
namespace arith {

//-------------------------------------------------------
// Base integer analysis API.
//
// We have multiple type of analyzers to do relaxed
// integer set analysis(bound analysis, modulo) and
// equivalence checking and simplification.
//
// Importantly, each analyzer may need result from
// another analyzer.
//-------------------------------------------------------

// Forward declare Analyzer
class Analyzer;

using tir::Var;

enum class DivMode {
    /*! \brief Truncated division. */
    kTruncDiv,
    /*! \brief Floor division. */
    kFloorDiv
};

/*!
 * \brief The strength used in top-level condition proves
 * \note The higher, the more time consuming it can be.
 *
 * Do not use level beyond kDefault in internal recursive rewriting in arith
 * analysis and only use it at top-level simplification to avoid speed issues.
 */
enum class ProofStrength : int {
    /*! \brief default strength, can be used in. */
    kDefault = 0,
    /*!
     * \brief Prove using symbolic bound analysis
     */
    kSymbolicBound = 1,
};

}// namespace arith
}// namespace litetvm

#endif//LITETVM_ARITH_ANALYZER_H
