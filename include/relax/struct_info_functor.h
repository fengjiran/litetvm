//
// Created by 赵丹 on 25-4-17.
//

#ifndef LITETVM_RELAX_STRUCT_INFO_FUNCTOR_H
#define LITETVM_RELAX_STRUCT_INFO_FUNCTOR_H

#include "node/functor.h"
#include "relax/distributed/struct_info.h"
#include "relax/struct_info.h"

namespace litetvm {
namespace relax {

template<typename FStructInfo>
class StructInfoFunctor;

// functions to be overriden.
#define STRUCT_INFO_FUNCTOR_DEFAULT \
    { return VisitStructInfoDefault_(op, std::forward<Args>(args)...); }

#define TVM_STRUCT_INFO_FUNCTOR_DISPATCH(OP)                                                         \
    vtable.template set_dispatch<OP>([](const ObjectRef& n, TSelf* self, Args... args) {             \
        return self->VisitStructInfo_(static_cast<const OP*>(n.get()), std::forward<Args>(args)...); \
    });

template<typename R, typename... Args>
class StructInfoFunctor<R(const StructInfo& n, Args...)> {
    using TSelf = StructInfoFunctor;
    using FStructInfo = NodeFunctor<R(const ObjectRef& n, TSelf* self, Args...)>;

public:
    /*! \brief the result type of this functor */
    using result_type = R;

    /*! \brief virtual destructor */
    virtual ~StructInfoFunctor() = default;

    /*!
   * \brief Same as call.
   * \param n The expression node.
   * \param args Additional arguments.
   * \return The result of the call
   */
    R operator()(const StructInfo& n, Args... args) {
        return VisitStructInfo(n, std::forward<Args>(args)...);
    }

    /*!
   * \brief The functor call.
   * \param n The expression node.
   * \param args Additional arguments.
   * \return The result of the call
   */
    virtual R VisitStructInfo(const StructInfo& n, Args... args) {
        CHECK(n.defined());
        static FStructInfo vtable = InitVTable();
        return vtable(n, this, std::forward<Args>(args)...);
    }
    // Functions that can be overriden by subclass
    virtual R VisitStructInfo_(const ObjectStructInfoNode* op, Args... args) STRUCT_INFO_FUNCTOR_DEFAULT;
    virtual R VisitStructInfo_(const PrimStructInfoNode* op, Args... args) STRUCT_INFO_FUNCTOR_DEFAULT;
    virtual R VisitStructInfo_(const ShapeStructInfoNode* op, Args... args) STRUCT_INFO_FUNCTOR_DEFAULT;
    virtual R VisitStructInfo_(const TensorStructInfoNode* op, Args... args) STRUCT_INFO_FUNCTOR_DEFAULT;
    virtual R VisitStructInfo_(const distributed::DTensorStructInfoNode* op, Args... args) STRUCT_INFO_FUNCTOR_DEFAULT;
    virtual R VisitStructInfo_(const TupleStructInfoNode* op, Args... args) STRUCT_INFO_FUNCTOR_DEFAULT;
    virtual R VisitStructInfo_(const FuncStructInfoNode* op, Args... args) STRUCT_INFO_FUNCTOR_DEFAULT;
    virtual R VisitStructInfoDefault_(const Object* op, Args...) {
        LOG(FATAL) << "Do not have a default for " << op->GetTypeKey();
        throw;// unreachable, written to stop compiler warning
    }

private:
    // initialize the vtable.
    static FStructInfo InitVTable() {
        FStructInfo vtable;
        // Set dispatch
        TVM_STRUCT_INFO_FUNCTOR_DISPATCH(ObjectStructInfoNode);
        TVM_STRUCT_INFO_FUNCTOR_DISPATCH(PrimStructInfoNode);
        TVM_STRUCT_INFO_FUNCTOR_DISPATCH(ShapeStructInfoNode);
        TVM_STRUCT_INFO_FUNCTOR_DISPATCH(TensorStructInfoNode);
        TVM_STRUCT_INFO_FUNCTOR_DISPATCH(distributed::DTensorStructInfoNode);
        TVM_STRUCT_INFO_FUNCTOR_DISPATCH(TupleStructInfoNode);
        TVM_STRUCT_INFO_FUNCTOR_DISPATCH(FuncStructInfoNode);
        vtable.Finalize();
        return vtable;
    }
};

#undef TVM_STRUCT_INFO_FUNCTOR_DISPATCH

/*!
 * \brief A struct info visitor.
 */
class LITETVM_API StructInfoVisitor : public StructInfoFunctor<void(const StructInfo& n)> {
public:
    void VisitStructInfo_(const ObjectStructInfoNode* op) override;
    void VisitStructInfo_(const PrimStructInfoNode* op) override;
    void VisitStructInfo_(const ShapeStructInfoNode* op) override;
    void VisitStructInfo_(const TensorStructInfoNode* op) override;
    void VisitStructInfo_(const distributed::DTensorStructInfoNode* op) override;
    void VisitStructInfo_(const TupleStructInfoNode* op) override;
    void VisitStructInfo_(const FuncStructInfoNode* op) override;

protected:
    // two functions to override when visit expr fields in struct info.
    virtual void VisitStructInfoExprField(const Expr& expr) {}
    virtual void VisitStructInfoExprField(const PrimExpr& expr) {}
};

/*!
 * \brief StructInfoMutator that mutates struct info.
 */
class LITETVM_API StructInfoMutator : public StructInfoFunctor<StructInfo(const StructInfo& n)> {
public:
    StructInfo VisitStructInfo_(const ObjectStructInfoNode* op) override;
    StructInfo VisitStructInfo_(const PrimStructInfoNode* op) override;
    StructInfo VisitStructInfo_(const ShapeStructInfoNode* op) override;
    StructInfo VisitStructInfo_(const TensorStructInfoNode* op) override;
    StructInfo VisitStructInfo_(const distributed::DTensorStructInfoNode* op) override;
    StructInfo VisitStructInfo_(const TupleStructInfoNode* op) override;
    StructInfo VisitStructInfo_(const FuncStructInfoNode* op) override;

protected:
    // two functions to override when visit expr fields in struct info.
    virtual Expr VisitStructInfoExprField(const Expr& expr) { return expr; }
    virtual PrimExpr VisitStructInfoExprField(const PrimExpr& expr) { return expr; }
};

}// namespace relax
}// namespace litetvm

#endif//LITETVM_RELAX_STRUCT_INFO_FUNCTOR_H
