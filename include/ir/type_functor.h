//
// Created by 赵丹 on 25-4-18.
//

#ifndef LITETVM_IR_TYPE_FUNCTOR_H
#define LITETVM_IR_TYPE_FUNCTOR_H

#include "ir/type.h"

#include <string>

namespace litetvm {

template<typename FType>
class TypeFunctor;

// functions to be overriden.
#define TYPE_FUNCTOR_DEFAULT \
    { return VisitTypeDefault_(op, std::forward<Args>(args)...); }

#define TVM_TYPE_FUNCTOR_DISPATCH(OP)                                                          \
    vtable.template set_dispatch<OP>([](const ObjectRef& n, TSelf* self, Args... args) {       \
        return self->VisitType_(static_cast<const OP*>(n.get()), std::forward<Args>(args)...); \
    });

template<typename R, typename... Args>
class TypeFunctor<R(const Type& n, Args...)> {
    using TSelf = TypeFunctor;
    using FType = NodeFunctor<R(const ObjectRef& n, TSelf* self, Args...)>;

public:
    /*! \brief the result type of this functor */
    using result_type = R;
    /*! \brief virtual destructor */
    virtual ~TypeFunctor() {}

    /*!
   * \brief Same as call.
   * \param n The expression node.
   * \param args Additional arguments.
   * \return The result of the call
   */
    R operator()(const Type& n, Args... args) {
        return VisitType(n, std::forward<Args>(args)...);
    }

    /*!
   * \brief The functor call.
   * \param n The expression node.
   * \param args Additional arguments.
   * \return The result of the call
   */
    virtual R VisitType(const Type& n, Args... args) {
        CHECK(n.defined());
        static FType vtable = InitVTable();
        return vtable(n, this, std::forward<Args>(args)...);
    }

    // Functions that can be overriden by subclass
    virtual R VisitType_(const FuncTypeNode* op, Args... args) TYPE_FUNCTOR_DEFAULT;
    virtual R VisitType_(const TupleTypeNode* op, Args... args) TYPE_FUNCTOR_DEFAULT;
    virtual R VisitType_(const PrimTypeNode* op, Args... args) TYPE_FUNCTOR_DEFAULT;
    virtual R VisitType_(const PointerTypeNode* op, Args... args) TYPE_FUNCTOR_DEFAULT;
    virtual R VisitTypeDefault_(const Object* op, Args...) {
        LOG(FATAL) << "Do not have a default for " << op->GetTypeKey();
        throw;// unreachable, written to stop compiler warning
    }

private:
    // initialize the vtable.
    static FType InitVTable() {
        FType vtable;
        // Set dispatch
        TVM_TYPE_FUNCTOR_DISPATCH(FuncTypeNode);
        TVM_TYPE_FUNCTOR_DISPATCH(TupleTypeNode);
        TVM_TYPE_FUNCTOR_DISPATCH(PrimTypeNode);
        TVM_TYPE_FUNCTOR_DISPATCH(PointerTypeNode);
        vtable.Finalize();
        return vtable;
    }
};

#undef TVM_TYPE_FUNCTOR_DISPATCH

/*!
 * \brief A type visitor that recursively visit types.
 */
class LITETVM_API TypeVisitor : public TypeFunctor<void(const Type& n)> {
public:
    void VisitType_(const FuncTypeNode* op) override;
    void VisitType_(const TupleTypeNode* op) override;
    void VisitType_(const PrimTypeNode* op) override;
    void VisitType_(const PointerTypeNode* op) override;
};

/*!
 * \brief TypeMutator that mutates expressions.
 */
class LITETVM_API TypeMutator : public TypeFunctor<Type(const Type& n)> {
public:
    Type VisitType(const Type& t) override;
    Type VisitType_(const FuncTypeNode* op) override;
    Type VisitType_(const TupleTypeNode* op) override;
    Type VisitType_(const PrimTypeNode* op) override;
    Type VisitType_(const PointerTypeNode* op) override;

private:
    Array<Type> MutateArray(Array<Type> arr);
};

}// namespace litetvm

#endif//LITETVM_IR_TYPE_FUNCTOR_H
