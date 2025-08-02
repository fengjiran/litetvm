//
// Created by 赵丹 on 25-4-16.
//

#ifndef LITETVM_NODE_SERIALIZATION_H
#define LITETVM_NODE_SERIALIZATION_H

#include "runtime/c_runtime_api.h"
#include "runtime/object.h"

namespace litetvm {
/*!
 * \brief save the node as well as all the node it depends on as json.
 *  This can be used to serialize any TVM object
 *
 * \return the string representation of the node.
 */
LITETVM_API std::string SaveJSON(const runtime::ObjectRef& node);

/*!
 * \brief Internal implementation of LoadJSON
 * Load tvm Node object from json and return a shared_ptr of Node.
 * \param json_str The json string to load from.
 *
 * \return The shared_ptr of the Node.
 */
LITETVM_API runtime::ObjectRef LoadJSON(std::string json_str);

}// namespace litetvm

#endif//LITETVM_NODE_SERIALIZATION_H
