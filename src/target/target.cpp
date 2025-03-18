//
// Created by 赵丹 on 25-3-18.
//

#include "runtime/device_api.h"
#include "runtime/registry.h"
#include "target/target.h"
#include "target/target_kind.h"

#include <dmlc/thread_local.h>
#include <algorithm>
#include <cctype>
#include <ios>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace litetvm {

using runtime::TVMArgs;

TVM_REGISTER_NODE_TYPE(TargetNode);

class TargetInternal {
public:
    static void EnterScope(Target target) { target.EnterWithScope(); }
    static void ExitScope(Target target) { target.ExitWithScope(); }
    static Map<String, ObjectRef> Export(Target target) { return target->Export(); }
    static const TargetKindNode::ValueTypeInfo& FindTypeInfo(const TargetKind& kind,
                                                             const std::string& key);
    static Optional<String> StringifyAttrsToRaw(const Map<String, ObjectRef>& attrs);
    static ObjectRef ParseType(const std::string& str, const TargetKindNode::ValueTypeInfo& info);
    static ObjectRef ParseType(const ObjectRef& obj, const TargetKindNode::ValueTypeInfo& info);
    static ObjectPtr<Object> FromString(const String& tag_or_config_or_target_str);
    static ObjectPtr<Object> FromConfigString(const String& config_str);
    static ObjectPtr<Object> FromRawString(const String& target_str);
    static ObjectPtr<Object> FromConfig(Map<String, ObjectRef> config);
    static void ConstructorDispatcher(TVMArgs args, TVMRetValue* rv);
    static Target WithHost(const Target& target, const Target& target_host) {
        ObjectPtr<TargetNode> n = make_object<TargetNode>(*target.get());
        n->host = target_host;
        return (Target)n;
    }

private:
    static std::unordered_map<String, ObjectRef> QueryDevice(int device_id, const TargetNode* target);
    static bool IsQuoted(const std::string& str);
    static std::string Quote(const std::string& str);
    static std::string JoinString(const std::vector<std::string>& array, char separator);
    static std::vector<std::string> SplitString(const std::string& str, char separator);
    static std::string Interpret(const std::string& str);
    static std::string Uninterpret(const std::string& str);
    static std::string StringifyAtomicType(const ObjectRef& obj);
    static std::string StringifyArray(const ArrayNode& array);

    static constexpr char quote = '\'';
    static constexpr char escape = '\\';
};

}
