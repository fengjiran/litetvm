//
// Created by 赵丹 on 25-2-28.
//

#ifndef REFLECTION_H
#define REFLECTION_H

#include "node/structural_equal.h"
#include "node/structural_hash.h"
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

/*!
 * \brief Virtual function table to support IR/AST node reflection.
 *
 * Functions are stored in columnar manner.
 * Each column is a vector indexed by Object's type_index.
 */
class ReflectionVTable {
public:
    /*!
   * \brief Visitor function.
   * \note We use function pointer, instead of std::function
   *       to reduce the dispatch overhead as field visit
   *       does not need as much customization.
   */
    using FVisitAttrs = void (*)(Object* self, AttrVisitor* visitor);

    /*!
   * \brief Equality comparison function.
   */
    using FSEqualReduce = bool (*)(const Object* self, const Object* other, SEqualReducer equal);

    /*!
   * \brief Structural hash reduction function.
   */
    using FSHashReduce = void (*)(const Object* self, SHashReducer hash_reduce);

    /*!
   * \brief creator function.
   * \param repr_bytes Repr bytes to create the object.
   *        If this is not empty then FReprBytes must be defined for the object.
   * \return The created function.
   */
    using FCreate = ObjectPtr<Object> (*)(const std::string& repr_bytes);

    /*!
   * \brief Function to get a byte representation that can be used to recover the object.
   * \param node The node pointer.
   * \return bytes The bytes that can be used to recover the object.
   */
    using FReprBytes = std::string (*)(const Object* self);

    /*!
   * \brief Dispatch the VisitAttrs function.
   * \param self The pointer to the object.
   * \param visitor The attribute visitor.
   */
    void VisitAttrs(Object* self, AttrVisitor* visitor) const {
        uint32_t tindex = self->type_index();
        if (tindex >= fvisit_attrs_.size() || fvisit_attrs_[tindex] == nullptr) {
            return;
        }

        fvisit_attrs_[tindex](self, visitor);
    }

    /*!
   * \brief Get repr bytes if any.
   * \param self The pointer to the object.
   * \param repr_bytes The output repr bytes, can be null, in which case the function
   *                   simply queries if the ReprBytes function exists for the type.
   * \return Whether repr bytes exists
   */
    bool GetReprBytes(const Object* self, std::string* repr_bytes) const {
        uint32_t tindex = self->type_index();
        if (tindex < frepr_bytes_.size() && frepr_bytes_[tindex] != nullptr) {
            if (repr_bytes != nullptr) {
                *repr_bytes = frepr_bytes_[tindex](self);
            }
            return true;
        }
        return false;
    }

    /*!
   * \brief Dispatch the SEqualReduce function.
   * \param self The pointer to the object.
   * \param other The pointer to another object to be compared.
   * \param equal The equality comparator.
   * \return the result.
   */
    bool SEqualReduce(const Object* self, const Object* other, SEqualReducer equal) const {
        uint32_t tindex = self->type_index();
        if (tindex >= fsequal_reduce_.size() || fsequal_reduce_[tindex] == nullptr) {
            LOG(FATAL) << "TypeError: SEqualReduce of " << self->GetTypeKey()
                       << " is not registered via TVM_REGISTER_NODE_TYPE."
                       << " Did you forget to set _type_has_method_sequal_reduce=true?";
        }
        return fsequal_reduce_[tindex](self, other, equal);
    }

    /*!
   * \brief Dispatch the SHashReduce function.
   * \param self The pointer to the object.
   * \param hash_reduce The hash reducer.
   */
    void SHashReduce(const Object* self, SHashReducer hash_reduce) const {
        uint32_t tindex = self->type_index();
        if (tindex >= fshash_reduce_.size() || fshash_reduce_[tindex] == nullptr) {
            LOG(FATAL) << "TypeError: SHashReduce of " << self->GetTypeKey()
                       << " is not registered via TVM_REGISTER_NODE_TYPE";
        }
        fshash_reduce_[tindex](self, hash_reduce);
    }

    /*!
   * \brief Create an initial object using default constructor
   *        by type_key and global key.
   *
   * \param type_key The type key of the object.
   * \param repr_bytes Bytes representation of the object if any.
   */
    NODISCARD LITETVM_API ObjectPtr<Object> CreateInitObject(const std::string& type_key,
                                                             const std::string& repr_bytes = "") const {
        uint32_t tindex = Object::TypeKey2Index(type_key);
        if (tindex >= fcreate_.size() || fcreate_[tindex] == nullptr) {
            LOG(FATAL) << "TypeError: " << type_key << " is not registered via TVM_REGISTER_NODE_TYPE";
        }
        return fcreate_[tindex](repr_bytes);
    }

  /*!
   * \brief Create an object by giving kwargs about its fields.
   *
   * \param type_key The type key.
   * \param kwargs the arguments in format key1, value1, ..., key_n, value_n.
   * \return The created object.
   */
  LITETVM_API ObjectRef CreateObject(const std::string& type_key, const runtime::TVMArgs& kwargs);

private:
    /*! \brief Attribute visitor. */
    std::vector<FVisitAttrs> fvisit_attrs_;
    /*! \brief Structural equal function. */
    std::vector<FSEqualReduce> fsequal_reduce_;
    /*! \brief Structural hash function. */
    std::vector<FSHashReduce> fshash_reduce_;
    /*! \brief Creation function. */
    std::vector<FCreate> fcreate_;
    /*! \brief ReprBytes function. */
    std::vector<FReprBytes> frepr_bytes_;
};

}// namespace litetvm

#endif//REFLECTION_H
