//
// Created by 赵丹 on 25-2-26.
//

#ifndef ADT_H
#define ADT_H

#include "runtime/base.h"

namespace litetvm::runtime {

/*! \brief An object representing a structure or enumeration. */
class ADTObj : public Object, public InplaceArrayBase<ADTObj, ObjectRef> {
public:
    /*! \brief The tag representing the constructor used. */
    int32_t tag;
    /*! \brief Number of fields in the ADT object. */
    uint32_t size;
    // The fields of the structure follows directly in memory.

    static constexpr uint32_t _type_index = static_cast<uint32_t>(TypeIndex::kRuntimeADT);
    static constexpr const char* _type_key = "runtime.ADT";
    TVM_DECLARE_FINAL_OBJECT_INFO(ADTObj, Object);

private:
    /*!
   * \return The number of elements in the array.
   */
    size_t GetSize() const { return size; }

    /*!
   * \brief Initialize the elements in the array.
   *
   * \tparam Iterator Iterator type of the array.
   * \param begin The begin iterator.
   * \param end The end iterator.
   */
    template<typename Iterator>
    void Init(Iterator begin, Iterator end) {
        size_t num_elems = std::distance(begin, end);
        this->size = 0;
        auto it = begin;
        for (size_t i = 0; i < num_elems; ++i) {
            InplaceArrayBase::EmplaceInit(i, *it++);
            // Only increment size after the initialization succeeds
            this->size++;
        }
    }

    friend class ADT;
    friend InplaceArrayBase;
};

/*! \brief reference to algebraic data type objects. */
class ADT : public ObjectRef {
public:
    /*!
     * \brief construct an ADT object reference.
     * \param tag The tag of the ADT object.
     * \param fields The fields of the ADT object.
     */
    ADT(int32_t tag, std::vector<ObjectRef> fields) : ADT(tag, fields.begin(), fields.end()) {};

    /*!
     * \brief construct an ADT object reference.
     * \param tag The tag of the ADT object.
     * \param begin The begin iterator to the start of the fields array.
     * \param end The end iterator to the end of the fields array.
     */
    template<typename Iterator>
    ADT(int32_t tag, Iterator begin, Iterator end) {
        size_t num_elems = std::distance(begin, end);
        auto ptr = make_inplace_array_object<ADTObj, ObjectRef>(num_elems);
        ptr->tag = tag;
        ptr->Init(begin, end);
        data_ = std::move(ptr);
    }

    /*!
     * \brief construct an ADT object reference.
     * \param tag The tag of the ADT object.
     * \param init The initializer list of fields.
     */
    ADT(int32_t tag, std::initializer_list<ObjectRef> init) : ADT(tag, init.begin(), init.end()) {};

    /*!
     * \brief Access element at index.
     *
     * \param idx The array index
     * \return const ObjectRef
     */
    const ObjectRef& operator[](size_t idx) const {
        return operator->()->operator[](idx);
    }

    /*!
     * \brief Return the ADT tag.
     */
    NODISCARD int32_t tag() const {
        return operator->()->tag;
    }

    /*!
     * \brief Return the number of fields.
     */
    NODISCARD size_t size() const {
        return operator->()->size;
    }

    /*!
     * \brief Construct a tuple object.
     *
     * \tparam Args Type params of tuple feilds.
     * \param args Tuple fields.
     * \return ADT The tuple object reference.
     */
    template<typename... Args>
    static ADT Tuple(Args&&... args) {
        return ADT(0, std::forward<Args>(args)...);
    }

    TVM_DEFINE_OBJECT_REF_METHODS(ADT, ObjectRef, ADTObj);
};

} // namespace litetvm::runtime


#endif//ADT_H
