//
// Created by richard on 8/2/25.
//

#ifndef LITETVM_NODE_SERIALIZATION_H
#define LITETVM_NODE_SERIALIZATION_H

#include "runtime/base.h"
#include "runtime/object.h"

#include <string>

namespace litetvm {
/*!
 * \brief save the node as well as all the node it depends on as json.
 *  This can be used to serialize any TVM object
 *
 * \return the string representation of the node.
 */
TVM_DLL std::string SaveJSON(Any node);

/*!
 * \brief Internal implementation of LoadJSON
 * Load tvm Node object from json and return a shared_ptr of Node.
 * \param json_str The json string to load from.
 *
 * \return The shared_ptr of the Node.
 */
TVM_DLL Any LoadJSON(std::string json_str);

}// namespace litetvm

#endif//LITETVM_NODE_SERIALIZATION_H
