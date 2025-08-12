//
// Created by richard on 8/2/25.
//

#ifndef LITETVM_NODE_REFLECTION_H
#define LITETVM_NODE_REFLECTION_H

#include "ffi/container/map.h"
#include "ffi/function.h"
#include "runtime/base.h"
#include "runtime/ndarray.h"

namespace litetvm {

/*!
 * \brief Create an object from a type key and a map of fields.
 * \param type_key The type key of the object.
 * \param fields The fields of the object.
 * \return The created object.
 */
TVM_DLL Any CreateObject(const String& type_key, const Map<String, Any>& fields);

}// namespace litetvm

#endif//LITETVM_NODE_REFLECTION_H
