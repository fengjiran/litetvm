//
// Created by 赵丹 on 25-4-16.
//
#include "tir/schedule/utils.h"

namespace litetvm {
namespace tir {

Array<StmtSRef> GetBlocks(const ScheduleState& self, const String& name, const GlobalVar& gv) {
    struct Finder : public StmtVisitor {
        explicit Finder(const ScheduleState& self, const String& name) : self_(self), name_(name) {}

        void VisitStmt_(const BlockNode* block) override {
            if (block->name_hint == name_) {
                auto it = self_->stmt2ref.find(block);
                CHECK(it != self_->stmt2ref.end());
                results_.push_back(it->second);
            }
            StmtVisitor::VisitStmt_(block);
        }

        const ScheduleState& self_;
        const String& name_;
        Array<StmtSRef> results_;
    };

    BaseFunc func = self->mod->Lookup(gv);
    const auto* prim_func = TVM_TYPE_AS(func, PrimFuncNode);
    Finder finder(self, name);
    finder(prim_func->body);
    return std::move(finder.results_);
}

Array<StmtSRef> GetLoops(const StmtSRef& block_sref) {
    std::vector<StmtSRef> result;
    for (StmtSRefNode* parent = block_sref->parent; parent && parent->stmt->IsInstance<ForNode>();
         parent = parent->parent) {
        result.push_back(GetRef<StmtSRef>(parent));
    }
    return {result.rbegin(), result.rend()};
}

Array<StmtSRef> GetChildBlocks(const ScheduleState& self, const StmtSRef& parent_sref) {
    struct Collector : public StmtVisitor {
    private:
        void VisitStmt_(const BlockNode* block) final { result.push_back(self->stmt2ref.at(block)); }

    public:
        explicit Collector(const ScheduleState& self) : self(self) {}

        const ScheduleState& self;
        Array<StmtSRef> result;
    };
    Collector collector(self);
    if (parent_sref->stmt->IsInstance<ForNode>()) {
        const auto* loop = static_cast<const ForNode*>(parent_sref->stmt);
        collector(loop->body);
    } else if (parent_sref->stmt->IsInstance<BlockNode>()) {
        const auto* block = static_cast<const BlockNode*>(parent_sref->stmt);
        collector(block->body);
    }
    return std::move(collector.result);
}

Array<StmtSRef> GetProducers(const ScheduleState& self, const StmtSRef& block_sref) {
    StmtSRef scope_root = GetScopeRoot(self, block_sref, /*require_stage_pipeline=*/false);
    return tir::GetProducers(block_sref, self->GetBlockScope(scope_root));
}

Array<StmtSRef> GetConsumers(const ScheduleState& self, const StmtSRef& block_sref) {
    StmtSRef scope_root = GetScopeRoot(self, block_sref, /*require_stage_pipeline=*/false);
    return tir::GetConsumers(block_sref, self->GetBlockScope(scope_root));
}

Array<StmtSRef> GetOutputBlocks(const ScheduleState& self, const StmtSRef& scope_sref) {
    const auto* scope_block = TVM_SREF_TO_BLOCK(scope_sref);
    return tir::GetOutputBlocks(self, scope_block);
}

/******** InstructionKind Registration ********/

struct GetBlockTraits : public UnpackedInstTraits<GetBlockTraits> {
    static constexpr const char* kName = "GetBlock";
    static constexpr bool kIsPure = true;

private:
    static constexpr size_t kNumInputs = 0;
    static constexpr size_t kNumAttrs = 2;
    static constexpr size_t kNumDecisions = 0;

    static BlockRV UnpackedApplyToSchedule(Schedule sch, String name, String func_name) {
        return sch->GetBlock(name, func_name);
    }

    static String UnpackedAsPython(Array<String> outputs, String name, String func_name) {
        PythonAPICall py("get_block");
        py.Input("name", name);
        py.Input("func_name", func_name);
        py.SingleOutput(outputs);
        return py.Str();
    }

    template<typename>
    friend struct tir::UnpackedInstTraits;
};

struct GetLoopsTraits : public UnpackedInstTraits<GetLoopsTraits> {
    static constexpr const char* kName = "GetLoops";
    static constexpr bool kIsPure = true;

private:
    static constexpr size_t kNumInputs = 1;
    static constexpr size_t kNumAttrs = 0;
    static constexpr size_t kNumDecisions = 0;

    static Array<LoopRV> UnpackedApplyToSchedule(Schedule sch, BlockRV block_rv) {
        return sch->GetLoops(block_rv);
    }

    static String UnpackedAsPython(Array<String> outputs, String block_rv) {
        PythonAPICall py("get_loops");
        py.Input("block", block_rv);
        py.OutputList(outputs);
        return py.Str();
    }

    template<typename>
    friend struct tir::UnpackedInstTraits;
};

struct GetChildBlocksTraits : public UnpackedInstTraits<GetChildBlocksTraits> {
    static constexpr const char* kName = "GetChildBlocks";
    static constexpr bool kIsPure = true;

private:
    static constexpr size_t kNumInputs = 1;
    static constexpr size_t kNumAttrs = 0;
    static constexpr size_t kNumDecisions = 0;

    static Array<BlockRV> UnpackedApplyToSchedule(Schedule sch, ObjectRef block_or_loop_rv) {
        if (auto block = block_or_loop_rv.as<BlockRV>()) {
            return sch->GetChildBlocks(block.value());
        }
        if (auto loop = block_or_loop_rv.as<LoopRV>()) {
            return sch->GetChildBlocks(loop.value());
        }
        LOG(FATAL) << "TypeError: Expected Block or Loop, but gets: " << block_or_loop_rv->GetTypeKey();
        throw;
    }

    static String UnpackedAsPython(Array<String> outputs, String block_or_loop_rv) {
        PythonAPICall py("get_child_blocks");
        py.Input("", block_or_loop_rv);
        py.OutputList(outputs);
        return py.Str();
    }

    template<typename>
    friend struct tir::UnpackedInstTraits;
};

struct GetProducersTraits : public UnpackedInstTraits<GetProducersTraits> {
    static constexpr const char* kName = "GetProducers";
    static constexpr bool kIsPure = true;

private:
    static constexpr size_t kNumInputs = 1;
    static constexpr size_t kNumAttrs = 0;
    static constexpr size_t kNumDecisions = 0;

    static Array<BlockRV> UnpackedApplyToSchedule(Schedule sch, BlockRV block_rv) {
        return sch->GetProducers(block_rv);
    }

    static String UnpackedAsPython(Array<String> outputs, String block_rv) {
        PythonAPICall py("get_producers");
        py.Input("block", block_rv);
        py.OutputList(outputs);
        return py.Str();
    }

    template<typename>
    friend struct tir::UnpackedInstTraits;
};

struct GetConsumersTraits : public UnpackedInstTraits<GetConsumersTraits> {
    static constexpr const char* kName = "GetConsumers";
    static constexpr bool kIsPure = true;

private:
    static constexpr size_t kNumInputs = 1;
    static constexpr size_t kNumAttrs = 0;
    static constexpr size_t kNumDecisions = 0;

    static Array<BlockRV> UnpackedApplyToSchedule(Schedule sch, BlockRV block_rv) {
        return sch->GetConsumers(block_rv);
    }

    static String UnpackedAsPython(Array<String> outputs, String block_rv) {
        PythonAPICall py("get_consumers");
        py.Input("block", block_rv);
        py.OutputList(outputs);
        return py.Str();
    }

    template<typename>
    friend struct tir::UnpackedInstTraits;
};

struct GetOutputBlocksTraits : public UnpackedInstTraits<GetOutputBlocksTraits> {
    static constexpr const char* kName = "GetOutputBlocks";
    static constexpr bool kIsPure = true;

private:
    static constexpr size_t kNumInputs = 1;
    static constexpr size_t kNumAttrs = 0;
    static constexpr size_t kNumDecisions = 0;

    static Array<BlockRV> UnpackedApplyToSchedule(Schedule sch, BlockRV block_rv) {
        return sch->GetOutputBlocks(block_rv);
    }

    static String UnpackedAsPython(Array<String> outputs, String block_rv) {
        PythonAPICall py("get_output_blocks");
        py.Input("block", block_rv);
        py.OutputList(outputs);
        return py.Str();
    }

    template<typename>
    friend struct tir::UnpackedInstTraits;
};

TVM_REGISTER_INST_KIND_TRAITS(GetBlockTraits);
TVM_REGISTER_INST_KIND_TRAITS(GetLoopsTraits);
TVM_REGISTER_INST_KIND_TRAITS(GetChildBlocksTraits);
TVM_REGISTER_INST_KIND_TRAITS(GetProducersTraits);
TVM_REGISTER_INST_KIND_TRAITS(GetConsumersTraits);
TVM_REGISTER_INST_KIND_TRAITS(GetOutputBlocksTraits);

}// namespace tir
}// namespace litetvm