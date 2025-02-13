//
// Created by 赵丹 on 25-2-13.
//

#include "runtime/file_utils.h"

#include <dmlc/json.h>
#include <dmlc/memory_io.h>

#include <fstream>
#include <unordered_map>
#include <vector>

namespace litetvm {
namespace runtime {

std::string GetFileFormat(const std::string& file_name, const std::string& format) {
    std::string fmt = format;
    if (fmt.length() == 0) {
        size_t pos = file_name.find_last_of(".");
        if (pos != std::string::npos) {
            return file_name.substr(pos + 1, file_name.length() - pos - 1);
        } else {
            return "";
        }
    } else {
        return format;
    }
}

}
}
