//
// Created by 赵丹 on 25-2-28.
//

#ifndef REFLECTION_H
#define REFLECTION_H

#include "runtime/c_runtime_api.h"
#include "runtime/data_type.h"
#include "runtime/ndarray.h"
#include "runtime/object.h"
#include "runtime/packed_func.h"

#include <string>
#include <vector>

namespace litetvm {

using runtime::DataType;
using runtime::NDArray;
using runtime::Object;
using runtime::ObjectPtr;
using runtime::ObjectRef;

/*!
 * \brief Visitor class to get the attributes of an AST/IR node.
 *  The content is going to be called for each field.
 *
 *  Each objects that wants reflection will need to implement
 *  a VisitAttrs function and call visitor->Visit on each of its field.
 */
class AttrVisitor {
public:
    //! \cond Doxygen_Suppress
    LITETVM_API virtual ~AttrVisitor() = default;
    LITETVM_API virtual void Visit(const char* key, double* value) = 0;
    LITETVM_API virtual void Visit(const char* key, int64_t* value) = 0;
    LITETVM_API virtual void Visit(const char* key, uint64_t* value) = 0;
    LITETVM_API virtual void Visit(const char* key, int* value) = 0;
    LITETVM_API virtual void Visit(const char* key, bool* value) = 0;
    LITETVM_API virtual void Visit(const char* key, std::string* value) = 0;
    LITETVM_API virtual void Visit(const char* key, void** value) = 0;
    LITETVM_API virtual void Visit(const char* key, DataType* value) = 0;
    LITETVM_API virtual void Visit(const char* key, NDArray* value) = 0;
    LITETVM_API virtual void Visit(const char* key, ObjectRef* value) = 0;

    template<typename ENum,
             typename = std::enable_if_t<std::is_enum_v<ENum>>>
    void Visit(const char* key, ENum* ptr) {
        static_assert(std::is_same_v<int, std::underlying_type_t<ENum>>,
                      "declare enum to be enum int to use visitor");
        this->Visit(key, reinterpret_cast<int*>(ptr));
    }
    //! \endcond
};

}// namespace litetvm

#endif//REFLECTION_H
