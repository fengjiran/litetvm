//
// Created by 赵丹 on 25-4-10.
//

#include "tir/stmt.h"
#include "arith/analyzer.h"
#include "runtime/registry.h"
#include "tir/op.h"
#include "tir/op_attr_types.h"

namespace litetvm {
namespace tir {

// LetStmt
LetStmt::LetStmt(Var var, PrimExpr value, Stmt body) {
    CHECK(value.defined());
    CHECK(body.defined());
    auto vdtype = value.dtype();
    // It is still valid to bind a pointer type
    // var to a value that is of type handle.
    if (var->type_annotation.as<PointerTypeNode>()) {
        CHECK(vdtype.is_handle());
    } else {
        CHECK_EQ(value.dtype(), var.dtype());
    }

    ObjectPtr<LetStmtNode> node = make_object<LetStmtNode>();
    node->var = std::move(var);
    node->value = std::move(value);
    node->body = std::move(body);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.LetStmt")
        .set_body_typed([](Var var, PrimExpr value, Stmt body) {
            return LetStmt(var, value, body);
        });

TVM_REGISTER_NODE_TYPE(LetStmtNode);

// AttrStmt
AttrStmt::AttrStmt(ObjectRef node, String attr_key, PrimExpr value, Stmt body) {
    // The nodes are not required to be a TIR type, and may legally
    // contain any ObjectRef.  However, normalizing to an IR type if
    // possible prevents spurious discrepancies in StructuralEqual().
    if (auto opt = node.as<runtime::Bool>()) {
        node = Bool(opt.value());
    } else if (auto opt = node.as<runtime::Int>()) {
        node = Integer(opt.value());
    }

    auto n = make_object<AttrStmtNode>();
    n->node = node;
    n->attr_key = std::move(attr_key);
    n->value = std::move(value);
    n->body = std::move(body);
    // n->span = std::move(span);
    data_ = std::move(n);
}

TVM_REGISTER_GLOBAL("tir.AttrStmt")
        .set_body_typed([](ObjectRef node, String attr_key, PrimExpr value, Stmt body) {
            return AttrStmt(node, attr_key, value, body);
        });

TVM_REGISTER_NODE_TYPE(AttrStmtNode);

// AssertStmt
AssertStmt::AssertStmt(PrimExpr condition, PrimExpr message, Stmt body) {
    CHECK(condition.defined());
    CHECK(condition.dtype().is_bool())
            << "AssertStmt should have boolean condition, "
            << "but received " << condition << " with dtype " << condition.dtype();
    CHECK(message.dtype() == DataType::Int(32) || message.as<StringImmNode>())
            << "TypeError: AssertStmt message must be an int or string:" << message << "\n";

    ObjectPtr<AssertStmtNode> node = make_object<AssertStmtNode>();
    node->condition = std::move(condition);
    node->message = std::move(message);
    node->body = std::move(body);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_NODE_TYPE(AssertStmtNode);
TVM_REGISTER_GLOBAL("tir.AssertStmt")
        .set_body_typed([](PrimExpr condition, ObjectRef message, Stmt body) {
            if (const auto* str = message.as<StringObj>()) {
                auto msg = StringImm(str->data);
                return AssertStmt(condition, msg, body);
            }
            return AssertStmt(condition, Downcast<PrimExpr>(message), body);
        });

// Evaluate
Evaluate::Evaluate(PrimExpr value) {
    CHECK(value.defined());

    ObjectPtr<EvaluateNode> node = make_object<EvaluateNode>();
    node->value = std::move(value);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.Evaluate").set_body_typed([](PrimExpr value) {
    return Evaluate(value);
});

TVM_REGISTER_NODE_TYPE(EvaluateNode);

// SeqStmt
SeqStmt::SeqStmt(Array<Stmt> seq) {
    bool requires_flattening = std::any_of(
            seq.begin(), seq.end(), [](const Stmt& stmt) { return stmt->IsInstance<SeqStmtNode>(); });

    if (requires_flattening) {
        auto flattened = SeqStmt::Flatten(seq);
        if (auto* ptr = flattened.as<SeqStmtNode>()) {
            seq = ptr->seq;
        } else {
            seq = {flattened};
        }
    }

    CHECK_NE(seq.size(), 0) << "An empty SeqStmt is prohibited.  "
                            << "To write a no-op, use Evaluate(0), "
                            << "or the result of SeqStmt::Flatten()";
    CHECK_NE(seq.size(), 1) << "A SeqStmt of length 1 is prohibited.  "
                            << "Use the node " << seq[0] << "directly, "
                            << "or for dynamic usage, normalize using SeqStmt::Flatten()";

    auto node = make_object<SeqStmtNode>();
    node->seq = std::move(seq);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.SeqStmt").set_body_typed([](Array<Stmt> seq) {
    return SeqStmt(std::move(seq));
});

TVM_REGISTER_NODE_TYPE(SeqStmtNode);

// BufferStore
BufferStore::BufferStore(Buffer buffer, PrimExpr value, Array<PrimExpr> indices,
                         Optional<PrimExpr> predicate) {
    CHECK_EQ(buffer->shape.size(), indices.size())
            << "Buffer " << buffer->name << " is " << buffer->shape.size()
            << "-dimensional, cannot be indexed with the " << indices.size()
            << "-dimensional indices provided.";

    for (int i = 0; i < static_cast<int>(indices.size()) - 1; i++) {
        CHECK(indices[i].dtype().is_scalar())
                << "Only the last index of a buffer access may be a vector type.";
    }

    bool is_index_scalable = indices.empty() ? false : indices.back().dtype().is_scalable_vector();
    bool is_buffer_dtype_scalable = buffer->dtype.is_scalable_vector();
    bool is_value_dtype_scalable = value.dtype().is_scalable_vector();

    CHECK(!(is_index_scalable && is_buffer_dtype_scalable))
            << "Index dtype and buffer dtype can't both be scalable.";

    if (predicate.defined()) {
        bool is_predicate_dtype_scalable = predicate.value().dtype().is_scalable_vector();
        CHECK_EQ(is_value_dtype_scalable, is_predicate_dtype_scalable)
                << "Predicate mask dtype and value dtype must both be scalable.";
    }

    if (is_index_scalable || is_buffer_dtype_scalable) {
        CHECK(is_value_dtype_scalable) << "Can't store non-scalable data into scalable buffer";
    }

    int index_lanes = indices.empty() ? 1 : indices.back().dtype().get_lanes_or_vscale_factor();
    int buffer_lanes = buffer->dtype.get_lanes_or_vscale_factor();
    int value_dtype_lanes = value.dtype().get_lanes_or_vscale_factor();

    CHECK_EQ(index_lanes * buffer_lanes, value_dtype_lanes)
            << "Cannot store value with " << value_dtype_lanes << ", expected value with "
            << index_lanes * buffer_lanes << " (" << index_lanes << " index lanes * " << buffer_lanes
            << " buffer element lanes)";

    if (predicate.defined()) {
        DataType predicate_dtype = predicate.value().dtype();
        int predicate_dtype_lanes = predicate_dtype.get_lanes_or_vscale_factor();
        CHECK_EQ(value_dtype_lanes, predicate_dtype_lanes)
                << "Got a predicate mask with " << predicate_dtype_lanes
                << " lanes, but trying to store a value with " << value_dtype_lanes
                << " lanes. The number of lanes must match.";

        DataType predicate_element_dtype = predicate_dtype.element_of();
        CHECK(predicate_element_dtype.is_bool())
                << "Predicate mask elements must be boolean values, but got " << predicate_element_dtype
                << ".";
    }

    runtime::DataType buffer_dtype;
    if (is_index_scalable || is_buffer_dtype_scalable) {
        buffer_dtype = buffer->dtype.with_scalable_vscale_factor(buffer_lanes * index_lanes);
    } else {
        buffer_dtype = buffer->dtype.with_lanes(buffer_lanes * index_lanes);
    }
    if (buffer_dtype != value.dtype()) {
        LOG(FATAL) << "TypeError: dtype mismatch on BufferStore: "    //
                   << "buffer's dtype is `" << buffer->dtype          //
                   << "`, the lanes of indexing are: `" << index_lanes//
                   << "`, the scalability is: `" << buffer_dtype.is_scalable_vector()
                   << "`, but RHS's dtype is `" << value.dtype() << "`";
    }

    ObjectPtr<BufferStoreNode> node = make_object<BufferStoreNode>();
    node->buffer = std::move(buffer);
    node->value = std::move(value);
    node->indices = std::move(indices);
    node->predicate = std::move(predicate);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.BufferStore")
        .set_body_typed([](Buffer buffer, PrimExpr value, Array<PrimExpr> indices,
                           Optional<PrimExpr> predicate) { return BufferStore(buffer, value, indices, predicate); });

TVM_REGISTER_NODE_TYPE(BufferStoreNode);

// BufferRealize
BufferRealize::BufferRealize(Buffer buffer, Array<Range> bounds, PrimExpr condition, Stmt body) {
    data_ = make_object<BufferRealizeNode>(buffer, bounds, condition, body);
}

TVM_REGISTER_GLOBAL("tir.BufferRealize")
        .set_body_typed([](Buffer buffer, Array<Range> bounds, PrimExpr condition, Stmt body) {
            return BufferRealize(buffer, bounds, condition, body);
        });

TVM_REGISTER_NODE_TYPE(BufferRealizeNode);

// ProducerStore
ProducerStore::ProducerStore(DataProducer producer, PrimExpr value, Array<PrimExpr> indices) {
    ObjectPtr<ProducerStoreNode> node = make_object<ProducerStoreNode>();
    node->producer = std::move(producer);
    node->value = std::move(value);
    node->indices = std::move(indices);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.ProducerStore")
        .set_body_typed([](DataProducer producer, PrimExpr value, Array<PrimExpr> indices) {
            return ProducerStore(producer, value, indices);
        });

TVM_REGISTER_NODE_TYPE(ProducerStoreNode);

// ProducerRealize
ProducerRealize::ProducerRealize(DataProducer producer, Region bounds, PrimExpr condition,
                                 Stmt body, String storage_scope) {
    for (size_t i = 0; i < bounds.size(); ++i) {
        CHECK(bounds[i]->min.defined());
        CHECK(bounds[i]->extent.defined());
        CHECK(bounds[i]->min.dtype().is_scalar());
        CHECK(bounds[i]->extent.dtype().is_scalar());
    }
    CHECK(body.defined());
    CHECK(condition.defined());
    CHECK(condition.dtype().is_bool());

    ObjectPtr<ProducerRealizeNode> node = make_object<ProducerRealizeNode>();
    node->producer = std::move(producer);
    node->bounds = std::move(bounds);
    node->condition = std::move(condition);
    node->body = std::move(body);
    // node->span = std::move(span);
    node->storage_scope = std::move(storage_scope);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.ProducerRealize")
        .set_body_typed([](DataProducer producer, Region bounds, PrimExpr condition, Stmt body,
                           String storage_scope) {
            return ProducerRealize(producer, bounds, condition, body, storage_scope);
        });

TVM_REGISTER_NODE_TYPE(ProducerRealizeNode);

// Allocate
Allocate::Allocate(Var buffer_var, DataType dtype, Array<PrimExpr> extents, PrimExpr condition,
                   Stmt body, Map<String, ObjectRef> annotations) {
    CHECK(IsPointerType(buffer_var->type_annotation, dtype) ||
          (dtype.is_bool() && IsPointerType(buffer_var->type_annotation, DataType::Int(8))))
            << "The allocated data type (" << dtype
            << ") does not match the type annotation of the buffer " << buffer_var << " ("
            << buffer_var->type_annotation
            << "). The data type should be an element of the pointer type.";

    for (size_t i = 0; i < extents.size(); ++i) {
        CHECK(extents[i].defined());
        CHECK(extents[i].dtype().is_scalar());
    }
    CHECK(body.defined());
    CHECK(condition.defined());
    CHECK(condition.dtype().is_bool());

    annotations = Downcast<Map<String, ObjectRef>>(NormalizeAttributeObject(annotations));

    ObjectPtr<AllocateNode> node = make_object<AllocateNode>();
    node->buffer_var = std::move(buffer_var);
    node->dtype = dtype;
    node->extents = std::move(extents);
    node->condition = std::move(condition);
    node->body = std::move(body);
    node->annotations = std::move(annotations);
    // node->span = std::move(span);
    data_ = std::move(node);
}

int64_t AllocateNode::ConstantAllocationSize(const Array<PrimExpr>& extents) {
    int64_t result = 1;
    for (size_t i = 0; i < extents.size(); ++i) {
        if (const IntImmNode* int_size = extents[i].as<IntImmNode>()) {
            result *= int_size->value;
            if (result > std::numeric_limits<int64_t>::max()) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    return result;
}

TVM_REGISTER_GLOBAL("tir.Allocate")
        .set_body_typed([](Var buffer_var, DataType type, Array<PrimExpr> extents, PrimExpr condition,
                           Stmt body, Map<String, ObjectRef> annotations) {
            return Allocate(buffer_var, type, extents, condition, body, annotations);
        });

TVM_REGISTER_NODE_TYPE(AllocateNode);

}// namespace tir
}// namespace litetvm
