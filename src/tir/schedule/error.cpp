//
// Created by 赵丹 on 25-4-16.
//

#include "tir/schedule/error.h"
#include "tir/schedule/utils.h"

namespace litetvm {
namespace tir {

String ScheduleError::RenderReport(const String& primitive) const {
    IRModule mod = this->mod();
    std::ostringstream os;

    // get locations of interest
    Array<ObjectRef> locs = LocationsOfInterest();
    std::unordered_map<ObjectRef, String, ObjectPtrHash, ObjectPtrEqual> loc_obj_to_name;
    int n_locs = locs.size();
    std::string msg = DetailRenderTemplate();
    PrinterConfig cfg;
    cfg->syntax_sugar = false;
    if (n_locs > 0) {
        for (int i = 0; i < n_locs; ++i) {
            std::string name = locs[i]->GetTypeKey() + '#' + std::to_string(i);
            std::string src = "{" + std::to_string(i) + "}";
            for (size_t pos; (pos = msg.find(src)) != std::string::npos;) {
                msg.replace(pos, src.length(), name);
            }
            cfg->obj_to_annotate.Set(locs[i], name);
            cfg->obj_to_underline.push_back(locs[i]);
        }
    }
    os << "ScheduleError: An error occurred in the schedule primitive '" << primitive
       << "'.\n\nThe IR with diagnostic is:\n"
       << TVMScriptPrinter::Script(mod, cfg) << std::endl;

    // print error message
    os << "Error message: " << msg;
    return os.str();
}

}// namespace tir
}// namespace litetvm