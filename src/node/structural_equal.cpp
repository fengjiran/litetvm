//
// Created by 赵丹 on 25-2-28.
//

#include "node/structural_equal.h"
#include "node/object_path.h"
#include "node/reflection.h"
#include "node/repr_printer.h"
#include "runtime/registry.h"

#include <optional>
#include <unordered_map>
#include <utility>

namespace litetvm {

using runtime::ObjectPtrEqual;
using runtime::ObjectPtrHash;

TVM_REGISTER_OBJECT_TYPE(ObjectPathPairNode);

TVM_REGISTER_GLOBAL("node.ObjectPathPairLhsPath")
        .set_body_typed([](const ObjectPathPair& object_path_pair) {
            return object_path_pair->lhs_path;
        });

TVM_REGISTER_GLOBAL("node.ObjectPathPairRhsPath")
        .set_body_typed([](const ObjectPathPair& object_path_pair) {
            return object_path_pair->rhs_path;
        });

namespace {
ObjectPath GetAttrPath(const ObjectRef& obj, const void* attr_address, const ObjectPath& path) {
    if (obj->IsInstance<runtime::Int::ContainerType>() || obj->IsInstance<runtime::Bool::ContainerType>() || obj->IsInstance<runtime::Float::ContainerType>()) {
        // Special case for containers that contain boxed primitives.  The
        // "value" attribute containing the boxed value should not be part
        // of the reported mismatched path.
        return path;
    }

    Optional<String> attr_key = GetAttrKeyByAddress(obj.get(), attr_address);
    return path->Attr(attr_key);
}
}// namespace

struct SEqualReducer::PathTracingData {
    ObjectPathPair current_paths;
    ObjectRef lhs_object;
    ObjectRef rhs_object;
    Optional<ObjectPathPair>* first_mismatch;

    NODISCARD ObjectPathPair GetPathsForAttrs(const ObjectRef& lhs, const ObjectRef& rhs) const {
        ObjectPath lhs_attr_path = GetAttrPath(lhs_object, &lhs, current_paths->lhs_path);
        ObjectPath rhs_attr_path = GetAttrPath(rhs_object, &rhs, current_paths->rhs_path);
        return {lhs_attr_path, rhs_attr_path};
    }
};

bool SEqualReducer::operator()(const ObjectRef& lhs, const ObjectRef& rhs) const {
    if (tracing_data_ == nullptr) {
        // Fast path: no tracing
        return handler_->SEqualReduce(lhs, rhs, map_free_vars_, NullOpt);
    }
    return ObjectAttrsEqual(lhs, rhs, map_free_vars_, nullptr);
}

bool SEqualReducer::DefEqual(const ObjectRef& lhs, const ObjectRef& rhs) const {
    if (tracing_data_ == nullptr) {
        // Fast path: no tracing
        return handler_->SEqualReduce(lhs, rhs, true, NullOpt);
    }
    return ObjectAttrsEqual(lhs, rhs, true, nullptr);
}

void SEqualReducer::GetPathsFromAttrAddressesAndStoreMismatch(const void* lhs_address, const void* rhs_address,
                                                              const PathTracingData* tracing_data) {
    if (tracing_data && !tracing_data->first_mismatch->defined()) {
        ObjectPath lhs_attr_path = GetAttrPath(tracing_data->lhs_object, lhs_address, tracing_data->current_paths->lhs_path);
        ObjectPath rhs_attr_path = GetAttrPath(tracing_data->rhs_object, rhs_address, tracing_data->current_paths->rhs_path);

        *tracing_data->first_mismatch = ObjectPathPair(lhs_attr_path, rhs_attr_path);
    }
}

template<typename T>
bool SEqualReducer::CompareAttributeValues(const T& lhs, const T& rhs, const PathTracingData* tracing_data, const Optional<ObjectPathPair>& paths) {
    if (BaseValueEqual()(lhs, rhs)) {
        return true;
    }

    if (tracing_data && !tracing_data->first_mismatch->defined()) {
        if (paths) {
            *tracing_data->first_mismatch = paths.value();
        } else {
            GetPathsFromAttrAddressesAndStoreMismatch(&lhs, &rhs, tracing_data);
        }
    }
    return false;
}

bool SEqualReducer::operator()(const double& lhs, const double& rhs, const Optional<ObjectPathPair>&) const {
    return CompareAttributeValues(lhs, rhs, tracing_data_);
}

bool SEqualReducer::operator()(const int64_t& lhs, const int64_t& rhs, const Optional<ObjectPathPair>&) const {
    return CompareAttributeValues(lhs, rhs, tracing_data_);
}

bool SEqualReducer::operator()(const uint64_t& lhs, const uint64_t& rhs, const Optional<ObjectPathPair>&) const {
    return CompareAttributeValues(lhs, rhs, tracing_data_);
}

bool SEqualReducer::operator()(const int& lhs, const int& rhs, const Optional<ObjectPathPair>&) const {
    return CompareAttributeValues(lhs, rhs, tracing_data_);
}

bool SEqualReducer::operator()(const bool& lhs, const bool& rhs, const Optional<ObjectPathPair>&) const {
    return CompareAttributeValues(lhs, rhs, tracing_data_);
}

bool SEqualReducer::operator()(const std::string& lhs, const std::string& rhs, const Optional<ObjectPathPair>&) const {
    return CompareAttributeValues(lhs, rhs, tracing_data_);
}

bool SEqualReducer::operator()(const DataType& lhs, const DataType& rhs, const Optional<ObjectPathPair>&) const {
    return CompareAttributeValues(lhs, rhs, tracing_data_);
}

const ObjectPathPair& SEqualReducer::GetCurrentObjectPaths() const {
    CHECK(tracing_data_ != nullptr) << "GetCurrentObjectPaths() can only be called when path tracing is enabled";
    return tracing_data_->current_paths;
}

void SEqualReducer::RecordMismatchPaths(const ObjectPathPair& paths) const {
    CHECK(tracing_data_ != nullptr) << "RecordMismatchPaths() can only be called when path tracing is enabled";
    if (!tracing_data_->first_mismatch->defined()) {
        *tracing_data_->first_mismatch = paths;
    }
}

bool SEqualReducer::EnumAttrsEqual(int lhs, int rhs, const void* lhs_address,
                                   const void* rhs_address, const Optional<ObjectPathPair>& paths) const {
    if (lhs == rhs) {
        return true;
    }

    if (tracing_data_ && !tracing_data_->first_mismatch->defined()) {
        if (paths) {
            *tracing_data_->first_mismatch = paths.value();
        } else {
            GetPathsFromAttrAddressesAndStoreMismatch(&lhs, &rhs, tracing_data_);
        }
    }

    return false;
}

bool SEqualReducer::ObjectAttrsEqual(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars,
                                     const ObjectPathPair* paths) const {
    if (tracing_data_ == nullptr) {
        // Fast path: no tracing
        return handler_->SEqualReduce(lhs, rhs, map_free_vars, NullOpt);
    }

    // Slow path: tracing object paths for better error reporting
    ObjectPathPair new_paths = paths == nullptr ? tracing_data_->GetPathsForAttrs(lhs, rhs) : *paths;

    if (handler_->SEqualReduce(lhs, rhs, map_free_vars, new_paths)) {
        return true;
    }

    if (!tracing_data_->first_mismatch->defined()) {
        *tracing_data_->first_mismatch = new_paths;
    }
    return false;
}

/*!
 * \brief A non-recursive stack-based SEqual handler that can remaps vars.
 *
 *  This handler pushes the Object equality cases into a stack, and
 *  traverses the stack to expand the necessary children that need to be checked.
 *
 *  The order of SEqual being called is the same as the order as if we
 *  eagerly do recursive calls in SEqualReduce.
 */
class SEqualHandlerDefault::Impl {
public:
    Impl(SEqualHandlerDefault* parent, bool assert_mode, Optional<ObjectPathPair>* first_mismatch, bool defer_fails)
        : parent_(parent), assert_mode_(assert_mode), first_mismatch_(first_mismatch), defer_fails_(defer_fails) {}

    // Function that implements actual equality check.
    bool Equal(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars) {
        if (!lhs.defined() && !rhs.defined()) {
            return true;
        }

        task_stack_.clear();
        pending_tasks_.clear();
        equal_map_lhs_.clear();
        equal_map_rhs_.clear();
        root_lhs_ = lhs;
        root_rhs_ = rhs;

        Optional<ObjectPathPair> current_paths;
        if (IsPathTracingEnabled()) {
            auto root_path = ObjectPath::Root();
            current_paths = ObjectPathPair(root_path, root_path);
        }

        if (!SEqualReduce(lhs, rhs, map_free_vars, current_paths)) {
            return false;
        }

        CHECK_EQ(pending_tasks_.size(), 1U);
        CHECK(allow_push_to_stack_);
        task_stack_.emplace_back(std::move(pending_tasks_.back()));
        pending_tasks_.clear();
        return RunTasks();
    }

    bool SEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars, const Optional<ObjectPathPair>& current_paths) {
        // We cannot use check lhs.same_as(rhs) to check equality.
        // if we choose to enable var remapping.
        //
        // Counter-example below (%x, %y) are shared vars
        // between the two functions (possibly before/after rewriting).
        //
        // - function0: fn (%x, %y) { %x + %y }
        // - function1. fn (%y, %x) { %x + %y }
        //
        // Because we choose to enable var remapping,
        // %x is mapped to %y, and %y is mapped to %x,
        // the body of the function no longer means the same thing.
        //
        // Take away: We can either choose only compare Var by address,
        // in which case we can use same_as for quick checking,
        // or we have to run deep comparison and avoid using same_as checks.
        auto run = [=] {
            auto early_result = [&] -> std::optional<bool> {
                if (!lhs.defined() && !rhs.defined()) {
                    return true;
                }

                if ((!lhs.defined() && rhs.defined()) || (!rhs.defined() && lhs.defined())) {
                    return false;
                }

                if (lhs->type_index() != rhs->type_index()) {
                    return false;
                }

                auto it = equal_map_lhs_.find(lhs);
                if (it != equal_map_lhs_.end()) {
                    return it->second.same_as(rhs);
                }

                if (equal_map_rhs_.contains(rhs)) {
                    return false;
                }

                return std::nullopt;
            }();

            if (early_result.has_value()) {
                if (early_result.value()) {
                    return true;
                }

                if (IsPathTracingEnabled() && IsFailDeferralEnabled() && current_paths.defined()) {
                    DeferFail(current_paths.value());
                    return true;
                }
                return false;
            }

            // need to push to pending tasks in this case
            pending_tasks_.emplace_back(lhs, rhs, map_free_vars, current_paths);
            return true;
        };

        return CheckResult(run(), lhs, rhs, current_paths);
    }

    // The default equal as registered in the structural equal vtable.
    bool DispatchSEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars,
                              const Optional<ObjectPathPair>& current_paths) {
        auto compute = [=] {
            CHECK(lhs.defined() && rhs.defined() && lhs->type_index() == rhs->type_index());
            // skip entries that already have equality maps.
            auto it = equal_map_lhs_.find(lhs);
            if (it != equal_map_lhs_.end()) {
                return it->second.same_as(rhs);
            }

            if (equal_map_rhs_.contains(rhs)) {
                return false;
            }

            if (!IsPathTracingEnabled()) {
                return vtable_->SEqualReduce(lhs.get(), rhs.get(),
                                             SEqualReducer(parent_, nullptr, map_free_vars));
            }

            PathTracingData tracing_data = {current_paths.value(), lhs, rhs, first_mismatch_};
            return vtable_->SEqualReduce(lhs.get(), rhs.get(), SEqualReducer(parent_, &tracing_data, map_free_vars));
        };
        return CheckResult(compute(), lhs, rhs, current_paths);
    }

    void DeferFail(const ObjectPathPair& mismatch_paths) {
        pending_tasks_.emplace_back(Task::ForceFailTag{}, mismatch_paths);
    }

    NODISCARD bool IsFailDeferralEnabled() const {
        return defer_fails_;
    }

    void MarkGraphNode() {
        // need to push to pending tasks in this case
        CHECK(!allow_push_to_stack_ && !task_stack_.empty());
        task_stack_.back().graph_equal = true;
    }

    ObjectRef MapLhsToRhs(const ObjectRef& lhs) {
        auto it = equal_map_lhs_.find(lhs);
        if (it != equal_map_lhs_.end())
            return it->second;
        return ObjectRef(nullptr);
    }

protected:
    // Check the result.
    bool CheckResult(bool result, const ObjectRef& lhs, const ObjectRef& rhs, const Optional<ObjectPathPair>& current_paths) {
        if (IsPathTracingEnabled() && !result && !first_mismatch_->defined()) {
            *first_mismatch_ = current_paths;
        }

        if (assert_mode_ && !result) {
            std::ostringstream oss;
            oss << "ValueError: StructuralEqual check failed, caused by lhs";
            if (first_mismatch_->defined()) {
                oss << " at " << first_mismatch_->value()->lhs_path;
                if (root_lhs_.defined()) {
                    PrinterConfig cfg;
                    cfg->syntax_sugar = false;
                    cfg->path_to_underline.push_back(first_mismatch_->value()->lhs_path);
                    // The TVMScriptPrinter::Script will fallback to Repr printer,
                    // if the root node to print is not supported yet,
                    // e.g. Relay nodes, ArrayNode, MapNode, etc.
                    oss << ":" << std::endl
                        << TVMScriptPrinter::Script(root_lhs_.value(), cfg);
                }
            } else {
                oss << ":" << std::endl
                    << lhs;
            }

            oss << std::endl
                << "and rhs";

            if (first_mismatch_->defined()) {
                oss << " at " << first_mismatch_->value()->rhs_path;
                if (root_rhs_.defined()) {
                    PrinterConfig cfg;
                    cfg->syntax_sugar = false;
                    cfg->path_to_underline.push_back(first_mismatch_->value()->rhs_path);
                    // The TVMScriptPrinter::Script will fallback to Repr printer,
                    // if the root node to print is not supported yet,
                    // e.g. Relay nodes, ArrayNode, MapNode, etc.
                    oss << ":" << std::endl
                        << TVMScriptPrinter::Script(root_rhs_.value(), cfg);
                }
            } else {
                oss << ":" << std::endl
                    << rhs;
            }
            LOG(FATAL) << oss.str();
        }
        return result;
    }

    /*!
   * \brief Run tasks until the stack reaches the stack begin
   * \param stack_begin The expected beginning of the stack.
   * \return The checks we encountered throughout the process.
   */
    bool RunTasks() {
        while (!task_stack_.empty()) {
            // Caution: entry becomes invalid when the stack changes
            auto& entry = task_stack_.back();

            if (entry.force_fail) {
                return CheckResult(false, entry.lhs, entry.rhs, entry.current_paths);
            }

            if (entry.children_expanded) {
                // When all the children have expanded and visited.
                // This means all the condition checks for
                // the current entry have been passed
                // We can safely mark lhs and rhs as equal to each other.
                auto it = equal_map_lhs_.find(entry.lhs);
                if (it != equal_map_lhs_.end()) {
                    CHECK(it->second.same_as(entry.rhs));
                }
                // create the map if the quality is graph equal.
                if (entry.graph_equal) {
                    equal_map_lhs_[entry.lhs] = entry.rhs;
                    equal_map_rhs_[entry.rhs] = entry.lhs;
                }
                task_stack_.pop_back();
            } else {
                // mark before expand
                // Important: because entry becomes invalid when stack changes.
                entry.children_expanded = true;
                // Expand the objects
                // The SEqual of the object can call into this->SEqualReduce
                // which populates the pending tasks.
                // CHECK_EQ(pending_tasks_.size(), 0U);
                CHECK(pending_tasks_.empty());
                allow_push_to_stack_ = false;
                if (!parent_->DispatchSEqualReduce(entry.lhs, entry.rhs, entry.map_free_vars, entry.current_paths)) {
                    return false;
                }
                allow_push_to_stack_ = true;
                // Push pending tasks in reverse order, so earlier tasks get to
                // expand first in the stack
                while (!pending_tasks_.empty()) {
                    task_stack_.emplace_back(std::move(pending_tasks_.back()));
                    pending_tasks_.pop_back();
                }
            }
        }
        return true;
    }

private:
    /*! \brief Pending reduce tasks. */
    struct Task {
        /*! \brief The lhs operand to be compared. */
        ObjectRef lhs;
        /*! \brief The rhs operand to be compared. */
        ObjectRef rhs;

        /*! \brief If path tracing is enabled, paths taken so far from the root to lhs and rhs objects. */
        Optional<ObjectPathPair> current_paths;

        /*! \brief The map free var argument. */
        bool map_free_vars;
        /*! \brief Whether the children have been expanded via SEqualReduce */
        bool children_expanded{false};
        /*! \brief whether the task is about graph equality(need remap). */
        bool graph_equal{false};
        /*! \brief whether the task should return "false" without actually comparing anything */
        bool force_fail{false};

        struct ForceFailTag {};// dispatch tag for the constructor below

        Task() = default;
        Task(ObjectRef lhs, ObjectRef rhs, bool map_free_vars, Optional<ObjectPathPair> current_paths)
            : lhs(std::move(lhs)), rhs(std::move(rhs)), current_paths(std::move(current_paths)), map_free_vars(map_free_vars) {}

        Task(ForceFailTag, const ObjectPathPair& current_paths) : current_paths(current_paths), force_fail(true) {}
    };

    NODISCARD bool IsPathTracingEnabled() const {
        return first_mismatch_ != nullptr;
    }

    // The owner of this impl
    SEqualHandlerDefault* parent_;
    // list of pending tasks to be pushed to the stack.
    std::vector<Task> pending_tasks_;
    // Internal task stack to executed the task.
    std::vector<Task> task_stack_;
    // Whether we allow push to stack.
    bool allow_push_to_stack_{true};
    //  If in assert mode, must return true, and will throw error otherwise.
    bool assert_mode_{false};
    // Location to store the paths to the first detected mismatch, or nullptr to disable path
    // tracing.
    Optional<ObjectPathPair>* first_mismatch_;
    // reflection vtable
    ReflectionVTable* vtable_ = ReflectionVTable::Global();
    // map from lhs to rhs
    std::unordered_map<ObjectRef, ObjectRef, ObjectPtrHash, ObjectPtrEqual> equal_map_lhs_;
    // map from rhs to lhs
    std::unordered_map<ObjectRef, ObjectRef, ObjectPtrHash, ObjectPtrEqual> equal_map_rhs_;
    // root lhs for result printing
    Optional<ObjectRef> root_lhs_;
    // root rhs for result printing
    Optional<ObjectRef> root_rhs_;
    // whether to defer fails
    bool defer_fails_;
};

SEqualHandlerDefault::SEqualHandlerDefault(bool assert_mode, Optional<ObjectPathPair>* first_mismatch, bool defer_fails) {
    impl = new Impl(this, assert_mode, first_mismatch, defer_fails);
}

SEqualHandlerDefault::~SEqualHandlerDefault() {
    delete impl;
}

bool SEqualHandlerDefault::SEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars,
                                        const Optional<ObjectPathPair>& current_paths) {
    return impl->SEqualReduce(lhs, rhs, map_free_vars, current_paths);
}

void SEqualHandlerDefault::DeferFail(const ObjectPathPair& mismatch_paths) {
    impl->DeferFail(mismatch_paths);
}

bool SEqualHandlerDefault::IsFailDeferralEnabled() {
    return impl->IsFailDeferralEnabled();
}

ObjectRef SEqualHandlerDefault::MapLhsToRhs(const ObjectRef& lhs) {
    return impl->MapLhsToRhs(lhs);
}

void SEqualHandlerDefault::MarkGraphNode() {
    impl->MarkGraphNode();
}

bool SEqualHandlerDefault::Equal(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars) {
    return impl->Equal(lhs, rhs, map_free_vars);
}

bool SEqualHandlerDefault::DispatchSEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars,
                                                const Optional<ObjectPathPair>& current_paths) {
    return impl->DispatchSEqualReduce(lhs, rhs, map_free_vars, current_paths);
}

bool StructuralEqual::operator()(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_params) const {
    return SEqualHandlerDefault(false, nullptr, false).Equal(lhs, rhs, map_free_params);
}

TVM_REGISTER_GLOBAL("node.StructuralEqual")
        .set_body_typed([](const ObjectRef& lhs, const ObjectRef& rhs, bool assert_mode, bool map_free_vars) {
            // If we are asserting on failure, then the `defer_fails` option
            // should be enabled, to provide better error messages.  For
            // example, if the number of bindings in a `relax::BindingBlock`
            // differs, highlighting the first difference rather than the
            // entire block.
            bool defer_fails = assert_mode;
            Optional<ObjectPathPair> first_mismatch;
            return SEqualHandlerDefault(assert_mode, &first_mismatch, defer_fails).Equal(lhs, rhs, map_free_vars);
        });

TVM_REGISTER_GLOBAL("node.GetFirstStructuralMismatch")
        .set_body_typed([](const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars) {
            Optional<ObjectPathPair> first_mismatch;
            bool equal = SEqualHandlerDefault(false, &first_mismatch, true).Equal(lhs, rhs, map_free_vars);
            CHECK(equal == !first_mismatch.defined());
            return first_mismatch;
        });

bool NDArrayEqual(const NDArray::Container* lhs, const NDArray::Container* rhs, SEqualReducer equal, bool compare_data) {
    if (lhs == rhs) return true;

    auto ldt = lhs->dl_tensor.dtype;
    auto rdt = rhs->dl_tensor.dtype;
    CHECK_EQ(static_cast<int>(lhs->dl_tensor.device.device_type), static_cast<int>(DLDeviceType::kDLCPU)) << "can only compare CPU tensor";
    CHECK_EQ(static_cast<int>(rhs->dl_tensor.device.device_type), static_cast<int>(DLDeviceType::kDLCPU)) << "can only compare CPU tensor";
    CHECK(runtime::IsContiguous(lhs->dl_tensor)) << "Can only compare contiguous tensor";
    CHECK(runtime::IsContiguous(rhs->dl_tensor)) << "Can only compare contiguous tensor";

    if (lhs->dl_tensor.ndim != rhs->dl_tensor.ndim) return false;
    for (int i = 0; i < lhs->dl_tensor.ndim; ++i) {
        if (!equal(lhs->dl_tensor.shape[i], rhs->dl_tensor.shape[i])) return false;
    }
    if (ldt.code == rdt.code && ldt.lanes == rdt.lanes && ldt.bits == rdt.bits) {
        size_t data_size = runtime::GetDataSize(lhs->dl_tensor);
        if (compare_data) {
            return std::memcmp(lhs->dl_tensor.data, rhs->dl_tensor.data, data_size) == 0;
        }
        return true;
    }
    return false;
}

bool NDArrayContainerTrait::SEqualReduce(const NDArray::Container* lhs, const NDArray::Container* rhs, SEqualReducer equal) {
    return NDArrayEqual(lhs, rhs, equal, true);
}

}// namespace litetvm
