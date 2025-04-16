//
// Created by 赵丹 on 25-4-16.
//

#ifndef LITETVM_TIR_SCHEDULE_ERROR_H
#define LITETVM_TIR_SCHEDULE_ERROR_H

#include "runtime/back_trace.h"
#include "ir/module.h"
#include "tir/stmt.h"

#include <string>

namespace litetvm {
namespace tir {

/*! \brief Error that happens during TensorIR scheduling */
class ScheduleError : public Error {
 public:
  /*! \brief Base constructor */
  ScheduleError() : Error("") {}
  /*! \brief The error occurred in this IRModule */
  virtual IRModule mod() const = 0;
  /*! \brief The locations of interest that we want to point out */
  virtual Array<ObjectRef> LocationsOfInterest() const = 0;
  /*!
   * \brief Returns an error string template for rendering, corresponds to the "detail" mode.
   * \sa ScheduleErrorRenderLevel
   * \note The template is a string, e.g.
   * "Some error occurred on block {0} and loop {1} blah blah"
   * And renderer will replace {0} and {1} according to the list provided LocationsOfInterest. Right
   * now it only printed out all the locations in plain text, but in the future, we may want to mark
   * the IR with underscores and attach names to each location of interest.
   */
  virtual String DetailRenderTemplate() const = 0;
  /*!
   * \brief Returns an error string without needing to render, corresponds to the "fast" mode
   * \sa ScheduleErrorRenderLevel
   */
  virtual String FastErrorString() const = 0;
  /*! \brief Render the ScheduleError with the template provided by `DetailRenderTemplate` */
  String RenderReport(const String& primitive) const;
};

class LoopPositionError : public ScheduleError {
 public:
  explicit LoopPositionError(IRModule mod, For loop, Block block, const std::string& primitive)
      : mod_(std::move(mod)),
        loop_(std::move(loop)),
        block_(std::move(block)),
        primitive_(primitive) {}

  String FastErrorString() const final {
    return "ScheduleError: " + primitive_ + " expect the loop to be an ancestor of block";
  }

  String DetailRenderTemplate() const final {
    std::ostringstream os;
    os << "ScheduleError: The input loop {0} of " << primitive_
       << " is required to be an ancestor of block {1}.";
    return os.str();
  }

  IRModule mod() const final { return mod_; }
  Array<ObjectRef> LocationsOfInterest() const final { return {loop_, block_}; }

  IRModule mod_;
  For loop_;
  Block block_;
  std::string primitive_;
};

}  // namespace tir

}

#endif //LITETVM_TIR_SCHEDULE_ERROR_H
