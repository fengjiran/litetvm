//
// Created by 赵丹 on 25-2-13.
//

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "runtime/string.h"
#include "runtime/map.h"
#include "runtime/meta_data.h"

#include <string>

namespace litetvm {
namespace runtime {

/*!
 * \brief Get file format from given file name or format argument.
 * \param file_name The name of the file.
 * \param format The format of the file.
 */
std::string GetFileFormat(const std::string& file_name, const std::string& format);


}
}

#endif //FILE_UTILS_H
