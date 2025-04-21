//
// Created by 赵丹 on 25-2-28.
//

#ifndef LITETVM_NODE_REFLECTION_H
#define LITETVM_NODE_REFLECTION_H

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
 *  Each object that wants reflection needs to implement
 *  a VisitAttrs function and call visitor->Visit on each of its fields.
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
        static_assert(std::is_same_v<int, std::underlying_type_t<ENum>>, "declare enum to be enum int to use visitor");
        this->Visit(key, reinterpret_cast<int*>(ptr));
    }
    //! \endcond
};

/*!
 * \brief Virtual function table to support IR/AST node reflection.
 *
 * Functions are stored in a columnar manner.
 * Each column is a vector indexed by Object's type_index.
 */
class ReflectionVTable {
public:
    /*!
   * \brief Visitor function.
   * \note We use a function pointer, instead of std::function
   *       to reduce the dispatch overhead as a field visit
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
    bool SEqualReduce(const Object* self, const Object* other, const SEqualReducer& equal) const {
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
                                                             const std::string& repr_bytes = "") const;

    /*!
   * \brief Create an object by giving kwargs about its fields.
   *
   * \param type_key The type key.
   * \param kwargs the arguments in format key1, value1, ..., key_n, value_n.
   * \return The created object.
   */
    NODISCARD LITETVM_API ObjectRef CreateObject(const std::string& type_key, const runtime::TVMArgs& kwargs) const;

    /*!
   * \brief Create an object by giving kwargs about its fields.
   *
   * \param type_key The type key.
   * \param kwargs The field arguments.
   * \return The created object.
   */
    NODISCARD LITETVM_API ObjectRef CreateObject(const std::string& type_key, const Map<String, ObjectRef>& kwargs) const;

    /*!
   * \brief Get an field object by the attr name.
   * \param self The pointer to the object.
   * \param attr_name The name of the field.
   * \return The corresponding attribute value.
   * \note This function will throw an exception if the object does not contain the field.
   */
    LITETVM_API runtime::TVMRetValue GetAttr(Object* self, const String& attr_name) const;

    /*!
   * \brief List all the fields in the object.
   * \return All the fields.
   */
    LITETVM_API std::vector<std::string> ListAttrNames(Object* self) const;

    /*! \return The global singleton. */
    LITETVM_API static ReflectionVTable* Global();

    class Registry;

    template<typename T, typename TraitName>
    Registry Register();

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

/*! \brief Registry of a reflection table. */
class ReflectionVTable::Registry {
public:
    Registry(ReflectionVTable* parent, uint32_t type_index) : parent_(parent), type_index_(type_index) {}

    /*!
   * \brief Set fcreate function.
   * \param f The creator function.
   * \return Reference to self.
   */
    Registry& set_creator(FCreate f) {// NOLINT(*)
        CHECK_LT(type_index_, parent_->fcreate_.size());
        parent_->fcreate_[type_index_] = f;
        return *this;
    }

    /*!
   * \brief Set bytes repr function.
   * \param f The ReprBytes function.
   * \return Reference to self.
   */
    Registry& set_repr_bytes(FReprBytes f) {// NOLINT(*)
        CHECK_LT(type_index_, parent_->frepr_bytes_.size());
        parent_->frepr_bytes_[type_index_] = f;
        return *this;
    }

private:
    ReflectionVTable* parent_;
    uint32_t type_index_;
};

#define TVM_REFLECTION_REG_VAR_DEF \
    static TVM_ATTRIBUTE_UNUSED ::litetvm::ReflectionVTable::Registry __make_reflection

/*!
 * \brief Directly register reflection VTable.
 * \param TypeName The name of the type.
 * \param TraitName A trait class that implements functions like VisitAttrs and SEqualReduce.
 *
 * \code
 *
 *  // Example SEQualReduce traits for runtime StringObj.
 *
 *  struct StringObjTrait {
 *    static constexpr const std::nullptr_t VisitAttrs = nullptr;
 *
 *    static void SHashReduce(const runtime::StringObj* key, SHashReducer hash_reduce) {
 *      hash_reduce->SHashReduceHashedValue(runtime::String::StableHashBytes(key->data, key->size));
 *    }
 *
 *    static bool SEqualReduce(const runtime::StringObj* lhs,
 *                             const runtime::StringObj* rhs,
 *                             SEqualReducer equal) {
 *      if (lhs == rhs) return true;
 *      if (lhs->size != rhs->size) return false;
 *      if (lhs->data != rhs->data) return true;
 *      return std::memcmp(lhs->data, rhs->data, lhs->size) != 0;
 *    }
 *  };
 *
 *  TVM_REGISTER_REFLECTION_VTABLE(runtime::StringObj, StringObjTrait);
 *
 * \endcode
 *
 * \note This macro can be called in different place as TVM_REGISTER_OBJECT_TYPE.
 *       And can be used to register the related reflection functions for runtime objects.
 */
#define TVM_REGISTER_REFLECTION_VTABLE(TypeName, TraitName)   \
    TVM_STR_CONCAT(TVM_REFLECTION_REG_VAR_DEF, __COUNTER__) = \
            ::litetvm::ReflectionVTable::Global()->Register<TypeName, TraitName>()

/*!
 * \brief Register a node type to object registry and reflection registry.
 * \param TypeName The name of the type.
 * \note This macro will call TVM_REGISTER_OBJECT_TYPE for the type as well.
 */
#define TVM_REGISTER_NODE_TYPE(TypeName)                                                   \
    TVM_REGISTER_OBJECT_TYPE(TypeName);                                                    \
    TVM_REGISTER_REFLECTION_VTABLE(TypeName, ::litetvm::detail::ReflectionTrait<TypeName>) \
            .set_creator([](const std::string&) -> ObjectPtr<Object> {                     \
                return ::litetvm::runtime::make_object<TypeName>();                        \
            })

// Implementation details
namespace detail {

template<typename T, bool = T::_type_has_method_visit_attrs>
struct ImplVisitAttrs {
    static constexpr auto VisitAttrs = nullptr;
};

template<typename T>
struct ImplVisitAttrs<T, true> {
    static void VisitAttrs(T* self, AttrVisitor* v) { self->VisitAttrs(v); }
};

template<typename T, bool = T::_type_has_method_sequal_reduce>
struct ImplSEqualReduce {
    static constexpr auto SEqualReduce = nullptr;
};

template<typename T>
struct ImplSEqualReduce<T, true> {
    static bool SEqualReduce(const T* self, const T* other, SEqualReducer equal) {
        return self->SEqualReduce(other, equal);
    }
};

template<typename T, bool = T::_type_has_method_shash_reduce>
struct ImplSHashReduce {
    static constexpr auto SHashReduce = nullptr;
};

template<typename T>
struct ImplSHashReduce<T, true> {
    static void SHashReduce(const T* self, SHashReducer hash_reduce) {
        self->SHashReduce(hash_reduce);
    }
};

template<typename T>
struct ReflectionTrait : ImplVisitAttrs<T>, ImplSEqualReduce<T>, ImplSHashReduce<T> {};

template<typename T, typename TraitName,
         bool = std::is_null_pointer_v<decltype(TraitName::VisitAttrs)>>
struct SelectVisitAttrs {
    static constexpr auto VisitAttrs = nullptr;
};

template<typename T, typename TraitName>
struct SelectVisitAttrs<T, TraitName, false> {
    static void VisitAttrs(Object* self, AttrVisitor* v) {
        TraitName::VisitAttrs(static_cast<T*>(self), v);
    }
};

template<typename T, typename TraitName,
         bool = std::is_null_pointer_v<decltype(TraitName::SEqualReduce)>>
struct SelectSEqualReduce {
    static constexpr std::nullptr_t SEqualReduce = nullptr;
};

template<typename T, typename TraitName>
struct SelectSEqualReduce<T, TraitName, false> {
    static bool SEqualReduce(const Object* self, const Object* other, SEqualReducer equal) {
        return TraitName::SEqualReduce(static_cast<const T*>(self), static_cast<const T*>(other),
                                       equal);
    }
};

template<typename T, typename TraitName,
         bool = std::is_null_pointer_v<decltype(TraitName::SHashReduce)>>
struct SelectSHashReduce {
    static constexpr std::nullptr_t SHashReduce = nullptr;
};

template<typename T, typename TraitName>
struct SelectSHashReduce<T, TraitName, false> {
    static void SHashReduce(const Object* self, SHashReducer hash_reduce) {
        return TraitName::SHashReduce(static_cast<const T*>(self), hash_reduce);
    }
};

}// namespace detail

template<typename T, typename TraitName>
ReflectionVTable::Registry ReflectionVTable::Register() {
    uint32_t tindex = T::RuntimeTypeIndex();
    if (tindex >= fvisit_attrs_.size()) {
        fvisit_attrs_.resize(tindex + 1, nullptr);
        fcreate_.resize(tindex + 1, nullptr);
        frepr_bytes_.resize(tindex + 1, nullptr);
        fsequal_reduce_.resize(tindex + 1, nullptr);
        fshash_reduce_.resize(tindex + 1, nullptr);
    }
    // functor that implements the redirection.
    fvisit_attrs_[tindex] = detail::SelectVisitAttrs<T, TraitName>::VisitAttrs;

    fsequal_reduce_[tindex] = detail::SelectSEqualReduce<T, TraitName>::SEqualReduce;

    fshash_reduce_[tindex] = detail::SelectSHashReduce<T, TraitName>::SHashReduce;

    return {this, tindex};
}

/*!
 * \brief Given an object and an address of its attribute, return the key of the attribute.
 * \return nullptr if no attribute with the given address exists.
 */
Optional<String> GetAttrKeyByAddress(const Object* object, const void* attr_address);

}// namespace litetvm

#endif//LITETVM_NODE_REFLECTION_H
