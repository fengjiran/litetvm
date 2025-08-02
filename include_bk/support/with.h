//
// Created by 赵丹 on 25-3-18.
//

#ifndef LITETVM_SUPPORT_WITH_H
#define LITETVM_SUPPORT_WITH_H

namespace litetvm {

/*!
 * \brief RAII wrapper function to enter and exit a context object
 *        similar to python's with syntax.
 *
 * \code
 * // context class
 * class MyContext {
 *  private:
 *    friend class With<MyContext>;
      MyContext(arguments);
 *    void EnterWithScope();
 *    void ExitWithScope();
 * };
 *
 * {
 *   With<MyContext> scope(arguments);
 *   // effect take place.
 * }
 * \endcode
 *
 * \tparam ContextType Type of the context object.
 */
template<typename ContextType>
class With {
public:
    /*!
   * \brief constructor.
   *  Enter the scope of the context.
   */
    template<typename... Args>
    explicit With(Args&&... args) : ctx_(std::forward<Args>(args)...) {
        ctx_.EnterWithScope();
    }
    /*! \brief destructor, leaves the scope of the context. */
    ~With() DMLC_THROW_EXCEPTION { ctx_.ExitWithScope(); }

    // Disable copy and move construction.  `With` is intended only for
    // use in nested contexts that are exited in the reverse order of
    // entry.  Allowing context to be copied or moved would break this
    // expectation.
    With(const With& other) = delete;
    With& operator=(const With& other) = delete;
    With(With&& other) = delete;
    With& operator=(With&& other) = delete;

    ContextType* get() { return &ctx_; }
    const ContextType* get() const { return &ctx_; }

    ContextType* operator->() { return get(); }
    const ContextType* operator->() const { return get(); }
    ContextType& operator*() { return *get(); }
    const ContextType* operator*() const { return *get(); }

    ContextType operator()() { return ctx_; }

private:
    /*! \brief internal context type. */
    ContextType ctx_;
};

}// namespace litetvm

#endif//LITETVM_SUPPORT_WITH_H
