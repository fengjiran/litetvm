//
// Created by 赵丹 on 25-4-16.
//

#include "relax/dataflow_pattern_functor.h"

namespace litetvm {
namespace relax {

// DFPatternVisitor

void DFPatternVisitor::VisitDFPattern(const DFPattern& pattern) {
    if (this->visited_.count(pattern.get()) == 0) {
        visited_.insert(pattern.get());
        DFPatternFunctor::VisitDFPattern(pattern);
    }
}

void DFPatternVisitor::VisitDFPattern_(const OrPatternNode* op) {
    VisitDFPattern(op->left);
    VisitDFPattern(op->right);
}

void DFPatternVisitor::VisitDFPattern_(const AndPatternNode* op) {
    VisitDFPattern(op->left);
    VisitDFPattern(op->right);
}

void DFPatternVisitor::VisitDFPattern_(const NotPatternNode* op) { VisitDFPattern(op->reject); }

void DFPatternVisitor::VisitDFPattern_(const AttrPatternNode* op) { VisitDFPattern(op->pattern); }

void DFPatternVisitor::VisitDFPattern_(const CallPatternNode* op) {
    VisitDFPattern(op->op);
    if (op->args.defined()) {
        for (auto arg: op->args) {
            VisitDFPattern(arg);
        }
    }
}

void DFPatternVisitor::VisitDFPattern_(const DataTypePatternNode* op) {
    VisitDFPattern(op->pattern);
}

void DFPatternVisitor::VisitDFPattern_(const ExprPatternNode* op) {}

void DFPatternVisitor::VisitDFPattern_(const FunctionPatternNode* op) {
    if (op->params.defined()) {
        for (auto param: op->params) {
            VisitDFPattern(param);
        }
    }
    VisitDFPattern(op->body);
}

void DFPatternVisitor::VisitDFPattern_(const ShapePatternNode* op) { VisitDFPattern(op->pattern); }

void DFPatternVisitor::VisitDFPattern_(const TupleGetItemPatternNode* op) {
    VisitDFPattern(op->tuple);
}

void DFPatternVisitor::VisitDFPattern_(const TuplePatternNode* op) {
    if (op->fields.defined()) {
        for (auto field: op->fields) {
            VisitDFPattern(field);
        }
    }
}

void DFPatternVisitor::VisitDFPattern_(const UnorderedTuplePatternNode* op) {
    if (op->fields.defined()) {
        for (auto field: op->fields) {
            VisitDFPattern(field);
        }
    }
}

void DFPatternVisitor::VisitDFPattern_(const TypePatternNode* op) { VisitDFPattern(op->pattern); }

void DFPatternVisitor::VisitDFPattern_(const StructInfoPatternNode* op) {
    VisitDFPattern(op->pattern);
}

// leaf nodes.
void DFPatternVisitor::VisitDFPattern_(const PrimArrPatternNode* op) {}
void DFPatternVisitor::VisitDFPattern_(const VarPatternNode* op) {}
void DFPatternVisitor::VisitDFPattern_(const ConstantPatternNode* op) {}
void DFPatternVisitor::VisitDFPattern_(const DataflowVarPatternNode* op) {}
void DFPatternVisitor::VisitDFPattern_(const GlobalVarPatternNode* op) {}
void DFPatternVisitor::VisitDFPattern_(const ExternFuncPatternNode* op) {}
void DFPatternVisitor::VisitDFPattern_(const WildcardPatternNode* op) {}

}// namespace relax
}// namespace litetvm