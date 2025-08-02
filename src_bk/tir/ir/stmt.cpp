//
// Created by 赵丹 on 25-4-10.
//

#include "tir/stmt.h"
#include "arith/analyzer.h"
#include "runtime/registry.h"
#include "tir/op.h"
#include "tir/op_attr_types.h"
#include "tir/utils.h"

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

// Const
// The constructor to create a IRNode with constant data
// depending on the type of ObjectRef, it will either
// create AllocateConstNode with irmod_storage_idx or data
AllocateConst::AllocateConst(Var buffer_var, DataType dtype, Array<PrimExpr> extents,
                             ObjectRef data_or_idx, Stmt body, Map<String, ObjectRef> annotations) {
    CHECK(IsPointerType(buffer_var->type_annotation, dtype))
            << "The allocated data type (" << dtype
            << ") does not match the type annotation of the buffer " << buffer_var << " ("
            << buffer_var->type_annotation
            << "). The data type should be an element of the pointer type.";

    for (size_t i = 0; i < extents.size(); ++i) {
        CHECK(extents[i].defined());
        CHECK(extents[i].dtype().is_scalar());
    }
    CHECK(body.defined());
    CHECK(data_or_idx.defined());

    annotations = Downcast<Map<String, ObjectRef>>(NormalizeAttributeObject(annotations));

    ObjectPtr<AllocateConstNode> node = make_object<AllocateConstNode>();
    node->buffer_var = std::move(buffer_var);
    node->dtype = dtype;
    node->extents = std::move(extents);
    node->body = std::move(body);
    node->annotations = annotations;
    // node->span = std::move(span);
    if (data_or_idx->IsInstance<NDArray::ContainerType>()) {
        node->data = Optional<NDArray>(Downcast<NDArray>(data_or_idx));
        node->irmod_storage_idx = Optional<Integer>();
    } else if (data_or_idx->IsInstance<IntImmNode>()) {
        node->data = Optional<NDArray>();
        node->irmod_storage_idx = Optional<Integer>(Downcast<Integer>(data_or_idx));
    } else {
        LOG(FATAL) << "Data type not supported: " << data_or_idx->GetTypeKey();
    }
    data_ = std::move(node);
}

int64_t AllocateConstNode::ConstantAllocationSize(const Array<PrimExpr>& extents) {
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
TVM_REGISTER_GLOBAL("tir.AllocateConst")
        .set_body_typed([](Var buffer_var, DataType dtype, Array<PrimExpr> extents,
                           ObjectRef data_or_idx, Stmt body, Map<String, ObjectRef> annotations) {
            return AllocateConst(buffer_var, dtype, extents, data_or_idx, body, annotations);
        });

TVM_REGISTER_NODE_TYPE(AllocateConstNode);

// DeclBuffer
DeclBuffer::DeclBuffer(Buffer buffer, Stmt body) {
    ObjectPtr<DeclBufferNode> node = make_object<DeclBufferNode>();
    node->buffer = std::move(buffer);
    node->body = std::move(body);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.DeclBuffer").set_body_typed([](Buffer buffer, Stmt body) {
    return DeclBuffer(buffer, body);
});

TVM_REGISTER_NODE_TYPE(DeclBufferNode);

// IfThenElse
IfThenElse::IfThenElse(PrimExpr condition, Stmt then_case, Optional<Stmt> else_case) {
    CHECK(condition.defined());
    CHECK(then_case.defined());
    // else_case may be null.
    ObjectPtr<IfThenElseNode> node = make_object<IfThenElseNode>();
    node->condition = std::move(condition);
    node->then_case = std::move(then_case);
    node->else_case = std::move(else_case);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_NODE_TYPE(IfThenElseNode);

TVM_REGISTER_GLOBAL("tir.IfThenElse")
        .set_body_typed([](PrimExpr condition, Stmt then_case, Stmt else_case) {
            return IfThenElse(condition, then_case, else_case);
        });

// For
For::For(Var loop_var, PrimExpr min, PrimExpr extent, ForKind kind, Stmt body,
         Optional<IterVar> thread_binding, Map<String, ObjectRef> annotations) {
    CHECK(loop_var.defined());
    CHECK(min.defined());
    CHECK(extent.defined());
    CHECK(body.defined());

    auto require_scalar_int_dtype = [&](PrimExpr expr, const char* field_name) {
        auto dtype = expr.dtype();
        CHECK(dtype.is_scalar() && (dtype.is_int() || dtype.is_uint()))
                << "TIR For nodes require a scalar integer as the " << field_name << ", but received "
                << expr << " with dtype " << dtype;
    };
    require_scalar_int_dtype(loop_var, "loop_var");
    require_scalar_int_dtype(min, "min");
    require_scalar_int_dtype(extent, "extent");

    // When extent or min is an IntImm but has narrower dtype than loop_var, we directly promote them
    // without raising errors.
    auto try_promote_imm_dtype = [&](const PrimExpr& e) {
        CHECK(e.dtype().bits() <= loop_var.dtype().bits())
                << " Loop variable's dtype (" << loop_var.dtype()
                << ") is narrower than that of `min` or `extent` (" << e.dtype() << ")";
        const IntImmNode* a = e.as<IntImmNode>();
        if (a && e.dtype().bits() < loop_var.dtype().bits()) {
            return make_const(loop_var.dtype(), a->value);
        } else {
            return e;
        }
    };

    min = try_promote_imm_dtype(min);
    extent = try_promote_imm_dtype(extent);

    CHECK(loop_var.dtype() == min.dtype()) << loop_var.dtype() << " vs " << min.dtype();
    CHECK(loop_var.dtype() == extent.dtype()) << loop_var.dtype() << " vs " << extent.dtype();

    annotations = Downcast<Map<String, ObjectRef>>(NormalizeAttributeObject(annotations));

    ObjectPtr<ForNode> node = make_object<ForNode>();
    node->loop_var = std::move(loop_var);
    node->min = std::move(min);
    node->extent = std::move(extent);
    node->kind = kind;
    node->body = std::move(body);
    node->thread_binding = std::move(thread_binding);
    node->annotations = std::move(annotations);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.For").set_body_typed(
        [](Var loop_var, PrimExpr min, PrimExpr extent, int kind, Stmt body,
           Optional<IterVar> thread_binding, Optional<Map<String, ObjectRef>> annotations) {
            return For(loop_var, min, extent, static_cast<ForKind>(kind), body, thread_binding,
                       annotations.value_or(Map<String, ObjectRef>()));
        });

TVM_REGISTER_NODE_TYPE(ForNode);

std::ostream& operator<<(std::ostream& out, ForKind type) {// NOLINT(*)
    switch (type) {
        case ForKind::kSerial:
            out << "for";
            break;
        case ForKind::kParallel:
            out << "parallel";
            break;
        case ForKind::kUnrolled:
            out << "unrolled";
            break;
        case ForKind::kVectorized:
            out << "vectorized";
            break;
        case ForKind::kThreadBinding:
            out << "launch_thread";
            break;
    }
    return out;
}

// While
While::While(PrimExpr condition, Stmt body) {
    CHECK(condition.defined());
    CHECK(condition.dtype().is_scalar());
    CHECK(condition.as<tir::IntImmNode>() == nullptr) << "The condition should not be trivial.";
    CHECK(body.defined());

    ObjectPtr<WhileNode> node = make_object<WhileNode>();
    node->condition = std::move(condition);
    node->body = std::move(body);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.While").set_body_typed([](PrimExpr condition, Stmt body) {
    return While(condition, body);
});

TVM_REGISTER_NODE_TYPE(WhileNode);

// Prefetch
Prefetch::Prefetch(Buffer buffer, Array<Range> bounds) {
    data_ = make_object<PrefetchNode>(buffer, bounds);
}

TVM_REGISTER_GLOBAL("tir.Prefetch")
        .set_body_typed([](Buffer buffer, Array<Range> bounds) {
            return Prefetch(buffer, bounds);
        });

TVM_REGISTER_NODE_TYPE(PrefetchNode);

// BufferRegion
BufferRegion::BufferRegion(Buffer buffer, Array<Range> region) {
    CHECK_EQ(buffer->shape.size(), region.size())
            << "The dimension between " << buffer << " and region " << region
            << " mismatched, the buffer is " << buffer;
    ObjectPtr<BufferRegionNode> node = make_object<BufferRegionNode>();
    node->buffer = std::move(buffer);
    node->region = std::move(region);
    data_ = std::move(node);
}

BufferRegion BufferRegion::FullRegion(Buffer buffer) {
    Array<Range> region;
    for (PrimExpr extent: buffer->shape) {
        region.push_back(Range::FromMinExtent(0, extent));
    }
    return BufferRegion(buffer, region);
}

BufferRegion BufferRegion::FromPoint(Buffer buffer, Array<PrimExpr> indices) {
    Array<Range> region;
    for (const PrimExpr& index: indices) {
        if (const RampNode* ramp_index = index.as<RampNode>()) {
            region.push_back(
                    Range::FromMinExtent(ramp_index->base, ramp_index->stride * ramp_index->lanes));
        } else {
            region.push_back(Range::FromMinExtent(index, make_const(index.dtype(), 1)));
        }
    }
    return BufferRegion(buffer, region);
}

TVM_REGISTER_GLOBAL("tir.BufferRegion").set_body_typed([](Buffer buffer, Array<Range> region) {
    return BufferRegion(buffer, region);
});

TVM_REGISTER_NODE_TYPE(BufferRegionNode);

// MatchBufferRegion
MatchBufferRegion::MatchBufferRegion(Buffer buffer, BufferRegion source) {
    const Buffer& source_buffer = source->buffer;
    arith::Analyzer analyzer;
    // Check scope and dtype
    CHECK_EQ(buffer.scope(), source_buffer.scope())
            << "MatchBuffer " << buffer << " scope mismatch:" << buffer.scope() << " vs. "
            << source_buffer.scope();
    CHECK_EQ(buffer->dtype, source_buffer->dtype)
            << "MatchBuffer " << buffer << " data type mismatch:" << buffer->dtype << " vs. "
            << source_buffer->dtype;

    // Check data_alignment
    CHECK(source_buffer->data_alignment % buffer->data_alignment == 0)
            << "Trying to match buffer to another one with lower alignment requirement "
            << " required_alignment=" << buffer->data_alignment
            << ", provided_alignment=" << source_buffer->data_alignment;

    // Check BufferType. AutoBroadcast is not allowed for now.
    CHECK(buffer->buffer_type == BufferType::kDefault &&
          source_buffer->buffer_type == BufferType::kDefault)
            << "AutoBroadcast is not allowed in MatchBuffer";

    // Validate shape
    CHECK(source->region.size() >= buffer->shape.size())
            << "Dimension of source Region expected to be larger or equal than target buffer shape, but "
               "got "
            << source->region.size() << " vs. " << buffer->shape.size();
    size_t offset = source->region.size() - buffer->shape.size();
    for (size_t i = 0; i < offset; ++i) {
        CHECK(analyzer.CanProve(source->region[i]->extent == 1))
                << "The higher dimension should be 1, but got " << source->region[i]->extent << ".";
    }
    for (size_t i = 0; i < buffer->shape.size(); ++i) {
        const Range& source_range = source->region[i + offset];
        const PrimExpr& buffer_shape = buffer->shape[i];
        if (!buffer_shape->IsInstance<VarNode>()) {
            CHECK(analyzer.CanProve(source_range->extent == buffer_shape))
                    << "The dimension mismatched between source region and target buffer shape, got "
                    << source_range->extent << " vs. " << buffer_shape << ".";
        }
    }
    // Note that we do not check elem_offset and strides in this function

    // Construction
    ObjectPtr<MatchBufferRegionNode> node = make_object<MatchBufferRegionNode>();
    node->buffer = std::move(buffer);
    node->source = std::move(source);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.MatchBufferRegion").set_body_typed([](Buffer buffer, BufferRegion source) {
    return MatchBufferRegion(buffer, source);
});

TVM_REGISTER_NODE_TYPE(MatchBufferRegionNode);

// Block
Block::Block(Array<IterVar> iter_vars, Array<BufferRegion> reads, Array<BufferRegion> writes,
             String name_hint, Stmt body, Optional<Stmt> init, Array<Buffer> alloc_buffers,
             Array<MatchBufferRegion> match_buffers, Map<String, ObjectRef> annotations) {
    annotations = Downcast<Map<String, ObjectRef>>(NormalizeAttributeObject(annotations));

    ObjectPtr<BlockNode> node = make_object<BlockNode>();
    node->iter_vars = std::move(iter_vars);
    node->reads = std::move(reads);
    node->writes = std::move(writes);
    node->name_hint = std::move(name_hint);
    node->body = std::move(body);
    node->init = std::move(init);
    node->alloc_buffers = std::move(alloc_buffers);
    node->match_buffers = std::move(match_buffers);
    node->annotations = std::move(annotations);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.Block")
        .set_body_typed([](Array<IterVar> iter_vars, Array<BufferRegion> reads,
                           Array<BufferRegion> writes, String name_hint, Stmt body, Optional<Stmt> init,
                           Array<Buffer> alloc_buffers, Array<MatchBufferRegion> match_buffers,
                           Map<String, ObjectRef> annotations) {
            return Block(iter_vars, reads, writes, name_hint, body, init, alloc_buffers, match_buffers,
                         annotations);
        });

TVM_REGISTER_NODE_TYPE(BlockNode);

// BlockRealize
BlockRealize::BlockRealize(Array<PrimExpr> values, PrimExpr predicate, Block block) {
    CHECK_EQ(block->iter_vars.size(), values.size())
            << "ValueError: BlockRealize needs to have the same number of iter_vars and binding values";
    CHECK(predicate.dtype().is_bool()) << "TypeError: Expect Block.predicate to be a bool expression";
    ObjectPtr<BlockRealizeNode> node = make_object<BlockRealizeNode>();
    node->iter_values = std::move(values);
    node->predicate = std::move(predicate);
    node->block = std::move(block);
    // node->span = std::move(span);
    data_ = std::move(node);
}

TVM_REGISTER_GLOBAL("tir.BlockRealize")
        .set_body_typed([](Array<PrimExpr> iter_values, PrimExpr predicate, Block block) {
            return BlockRealize(iter_values, predicate, block);
        });

TVM_REGISTER_NODE_TYPE(BlockRealizeNode);

PrimExpr TypeAnnotation(DataType dtype) {
    static auto op = Op::Get("tir.type_annotation");
    return tir::Call(dtype, op, {});
}

TVM_TIR_REGISTER_OP("type_annotation")
        .set_attr<TCallEffectKind>("TCallEffectKind", Integer(CallEffectKind::kPure))
        .set_attr<TScriptDtypePrintLocation>("TScriptDtypePrintLocation",
                                             Integer(ScriptDtypePrintLocation::kFirst));


}// namespace tir
}// namespace litetvm
