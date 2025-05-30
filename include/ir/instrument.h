//
// Created by 赵丹 on 25-4-15.
//

#ifndef LITETVM_IR_INSTRUMENT_H
#define LITETVM_IR_INSTRUMENT_H

#include "node/reflection.h"
#include "runtime/string.h"

#include <utility>
#include <vector>

namespace litetvm {

class IRModule;

// Forward class for PassInstrumentNode methods
namespace transform {
class PassInfo;
}// namespace transform

namespace instrument {

/*!
 * \brief PassInstrumentNode forms an instrument implementation.
 * It provides API for users to register callbacks at different instrumentation points.
 *
 * Within a PassContext, call sequence of a PassInstrument implementation is like:
 *
 *   with PassContext(instruments=[pi]):  # pi = a PassInstrument implementation
 *       pi.EnterPassContext()
 *
 *       if pi.ShouldRun(Pass1):
 *           pi.RunBeforePass()
 *           Pass1()
 *           pi.RunAfterPass()
 *
 *       if pi.ShouldRun(Pass2):
 *           pi.RunBeforePass()
 *           Pass2()
 *           pi.RunAfterPass()
 *
 *       pi.ExitPassContext()
 *
 * `EnterPassContext` and `ExitPassContext` are only called once when entering/exiting a
 * PassContext. `ShouldRun`, `RunBeforePass` and `RunAfterPass` are called multiple times depending
 * on how many passes.
 *
 * If there are multiple pass instrumentations provided, the instrument points are the same.
 * PassInstrument implementations' callbacks are called in order:
 *
 *   with PassContext(instruments=[pi1, pi2]):  # pi1, pi2 = two distinct PassInstrument impls
 *       pi.EnterPassContext() for pi in instruments
 *
 *       should_run = all([pi.ShoudRun(Pass1) for pi in instruments)])
 *       if (should_run)
 *           pi.RunBeforePass() for pi in instruments
 *           Pass1()
 *           pi.RunAfterPass()  for pi in instruments
 *
 *       should_run = all([pi.ShouldRun(Pass2) for pi in instruments)])
 *       if (should_run)
 *           pi.RunBeforePass() for pi in instruments
 *           Pass2()
 *           pi.RunAfterPass() for pi in instruments
 *
 *       pi.ExitPassContext() for pi in instruments
 *
 * Note:
 *   1. Assume there is no dependency between PassInstrument implementations in `instruments` .
 *   2. `EnterPassContext` and `ExitPassContext` have `with` behavior (see PassContext and its FFI):
 *        If there is any exception raised in `ShouldRun()`, `RunBeforePass()`, `RunAfterPass()` and
 *        `Pass()`, `ExitPassContext()` is still called.
 *   3. In mutiple PassInstrument instances scenario, callbacks are called in order:
 *        If one throws exceptions, remainings will not be called.
 *
 * \sa PassInstrument
 * \sa src/ir/transform.cc
 */
class PassInstrumentNode : public Object {
public:
    /*! \brief Name of this pass instrument object. */
    String name;

    virtual ~PassInstrumentNode() {}

    /*! \brief Instrument when entering PassContext. Called once within a PassContext. */
    virtual void EnterPassContext() const = 0;

    /*! \brief Instrument when exiting PassContext. Called once within a PassContext. */
    virtual void ExitPassContext() const = 0;

    /*!
   * \brief Determine whether to run the pass or not. Called multiple times depend on number of
   *        passes.
   * \param mod The module that an optimization pass runs on.
   * \param info The pass information.
   *
   * \return true to run the pass; false to skip the pass.
   */
    virtual bool ShouldRun(const IRModule& mod, const transform::PassInfo& info) const = 0;

    /*!
   * \brief Instrument before pass run. Called multiple times depend on number of passes.
   * \param mod The module that an optimization pass runs on.
   * \param info The pass information.
   */
    virtual void RunBeforePass(const IRModule& mod, const transform::PassInfo& info) const = 0;

    /*!
   * \brief Instrument after pass run. Called multiple time depend on number of passes.
   * \param mod The module that an optimization pass runs on.
   * \param info The pass information.
   */
    virtual void RunAfterPass(const IRModule& mod, const transform::PassInfo& info) const = 0;

    void VisitAttrs(AttrVisitor* v) { v->Visit("name", &name); }

    static constexpr const char* _type_key = "instrument.PassInstrument";
    TVM_DECLARE_BASE_OBJECT_INFO(PassInstrumentNode, Object);
};

/*!
 * \brief Managed reference class for PassInstrumentNode
 * \sa PassInstrumentNode
 */
class PassInstrument : public ObjectRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(PassInstrument, ObjectRef, PassInstrumentNode);
};

}// namespace instrument

}// namespace litetvm

#endif//LITETVM_IR_INSTRUMENT_H
