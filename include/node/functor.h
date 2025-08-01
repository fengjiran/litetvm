//
// Created by richard on 8/2/25.
//

#ifndef LITETVM_NODE_FUNCTOR_H
#define LITETVM_NODE_FUNCTOR_H

#include "runtime/logging.h"
#include "runtime/object.h"

#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>

namespace litetvm {

using runtime::ObjectRef;


/*!
 * \brief A dynamically dispatched functor on the type of the first argument.
 *
 * This is a class that is useful to construct polymorphic dispatching
 * base on the AST/IR node's type.
 *
 * \code
 *   NodeFunctor<std::string (const ObjectRef& n, std::string prefix)> tostr;
 *   tostr.set_dispatch<Add>([](const ObjectRef& op, std::string prefix) {
 *     return prefix + "Add";
 *   });
 *   tostr.set_dispatch<IntImm>([](const ObjectRef& op, std::string prefix) {
 *     return prefix + "IntImm"
 *   });
 *
 *   Expr x = make_const(1);
 *   Expr y = x + x;
 *   // dispatch to IntImm, outputs "MyIntImm"
 *   LOG(INFO) << tostr(x, "My");
 *   // dispatch to IntImm, outputs "MyAdd"
 *   LOG(INFO) << tostr(y, "My");
 * \endcode
 *
 * \tparam FType function signiture
 *  This type if only defined for FType with function signature
 */
template<typename FType>
class NodeFunctor;

template<typename R, typename... Args>
class NodeFunctor<R(const ObjectRef& n, Args...)> {
private:
    /*! \brief internal function pointer type */
    typedef R (*FPointer)(const ObjectRef& n, Args...);
    /*! \brief refer to itself. */
    using TSelf = NodeFunctor<R(const ObjectRef& n, Args...)>;
    /*! \brief internal function table */
    std::vector<FPointer> func_;
    /*! \brief start range of func index */
    uint32_t begin_type_index_{0};

public:
    /*! \brief the result type of this functor */
    using result_type = R;
    /*!
   * \brief Whether the functor can dispatch the corresponding Node
   * \param n The node to be dispatched
   * \return Whether dispatching function is registered for n's type.
   */
    bool can_dispatch(const ObjectRef& n) const {
        uint32_t type_index = n->type_index();
        if (type_index < begin_type_index_) return false;
        type_index -= begin_type_index_;
        return type_index < func_.size() && func_[type_index] != nullptr;
    }
    /*!
   * \brief invoke the functor, dispatch on type of n
   * \param n The Node argument
   * \param args The additional arguments
   * \return The result.
   */
    R operator()(const ObjectRef& n, Args... args) const {
        ICHECK(can_dispatch(n)) << "NodeFunctor calls un-registered function on type "
                                << n->GetTypeKey();
        return (*func_[n->type_index() - begin_type_index_])(n, std::forward<Args>(args)...);
    }
    /*!
   * \brief set the dispatcher for type TNode
   * \param f The function to be set.
   * \tparam TNode the type of Node to be dispatched.
   * \return reference to self.
   */
    template<typename TNode>
    TSelf& set_dispatch(FPointer f) {// NOLINT(*)
        uint32_t tindex = TNode::RuntimeTypeIndex();
        if (func_.size() <= tindex) {
            func_.resize(tindex + 1, nullptr);
        }
        ICHECK(func_[tindex] == nullptr) << "Dispatch for " << TNode::_type_key << " is already set";
        ICHECK_EQ(begin_type_index_, 0) << " Cannot call set_dispatch after calling Finalize";
        func_[tindex] = f;
        return *this;
    }
    /*!
   * \brief unset the dispatcher for type TNode
   *
   * \tparam TNode the type of Node to be dispatched.
   * \return reference to self.
   */
    template<typename TNode>
    TSelf& clear_dispatch() {// NOLINT(*)
        uint32_t tindex = TNode::RuntimeTypeIndex();
        ICHECK_LT(tindex, func_.size()) << "clear_dispatch: index out of range";
        ICHECK_EQ(begin_type_index_, 0) << " Cannot call clear_dispatch after calling Finalize";
        func_[tindex] = nullptr;
        return *this;
    }
    /*!
   * \brief Finalize the functor after calling sequence of set_dispatch
   * This function will attempt to find the min type index that is not null
   * and optimize the space of the func table so it is more compact
   */
    void Finalize() {
        ICHECK_EQ(begin_type_index_, 0) << "Can only call Finalize once";
        while (begin_type_index_ < func_.size() && func_[begin_type_index_] == nullptr) {
            ++begin_type_index_;
        }
        // shift up the function value
        size_t new_ftable_size = func_.size() - begin_type_index_;
        if (begin_type_index_ != 0) {
            std::memmove(func_.data(), func_.data() + begin_type_index_,
                         new_ftable_size * sizeof(FPointer));
        }
        func_.resize(new_ftable_size);
        func_.shrink_to_fit();
    }
};

#define TVM_REG_FUNC_VAR_DEF(ClsName) static TVM_ATTRIBUTE_UNUSED auto& __make_functor##_##ClsName

/*!
 * \brief Useful macro to set NodeFunctor dispatch in a global static field.
 *
 * \code
 *  // Use NodeFunctor to implement ReprPrinter similar to Visitor Pattern.
 *  // vtable allows easy patch of new Node types, without changing
 *  // interface of ReprPrinter.
 *
 *  class ReprPrinter {
 *   public:
 *    std::ostream& stream;
 *    // the dispatch function.
 *    void print(Expr e) {
 *      const static FType& f = *vtable();
 *      f(e, this);
 *    }
 *
 *    using FType = NodeFunctor<void (const ObjectRef&, ReprPrinter* )>;
 *    // function to return global function table
 *    static FType& vtable();
 *  };
 *
 *  // in cpp/cc file
 *  ReprPrinter::FType& ReprPrinter::vtable() { // NOLINT(*)
 *    static FType inst; return inst;
 *  }
 *
 *  TVM_STATIC_IR_FUNCTOR(ReprPrinter, vtable)
 *  .set_dispatch<Add>([](const ObjectRef& ref, ReprPrinter* p) {
 *    auto* n = static_cast<const Add*>(ref.get());
 *    p->print(n->a);
 *    p->stream << '+'
 *    p->print(n->b);
 *  });
 *
 *
 * \endcode
 *
 * \param ClsName The name of the class
 * \param FField The static function that returns a singleton of NodeFunctor.
 */
#define TVM_STATIC_IR_FUNCTOR(ClsName, FField) \
    TVM_STR_CONCAT(TVM_REG_FUNC_VAR_DEF(ClsName), __COUNTER__) = ClsName::FField()

}// namespace litetvm

#endif//LITETVM_NODE_FUNCTOR_H
