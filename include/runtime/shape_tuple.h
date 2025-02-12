//
// Created by richard on 1/30/25.
//

#ifndef SHAPE_TUPLE_H
#define SHAPE_TUPLE_H

#include "runtime/memory.h"
#include "runtime/object.h"

// nested namespace
namespace litetvm::runtime {

/*! \brief An object representing a shape tuple. */
class ShapeTupleNode : public Object {
public:
    /*! \brief The type of shape index element. */
    using index_type = int64_t;

    explicit ShapeTupleNode(std::vector<index_type> shape) : data_container(std::move(shape)) {}

    /*! \brief The size of the shape tuple object. */
    NODISCARD uint64_t size() const {
        return data_container.size();
    }

    NODISCARD const index_type* data() const {
        return data_container.data();
    }

    /*! \brief Get "numel", meaning the number of elements of an array if the array has this shape */
    NODISCARD index_type Product() const;

private:
    /*! \brief Container that holds the memory. */
    std::vector<index_type> data_container;

public:
    static constexpr uint32_t _type_index = static_cast<uint32_t>(TypeIndex::kRuntimeShapeTuple);
    static constexpr const char* _type_key = "runtime.ShapeTuple";
    TVM_DECLARE_FINAL_OBJECT_INFO(ShapeTupleNode, Object);

    friend class ShapeTuple;
};

class ShapeTuple : public ObjectRef {
public:
    /*! \brief The type of shape index element. */
    using index_type = ShapeTupleNode::index_type;

    /*!
   * \brief Construct an empty shape tuple.
   */
    ShapeTuple() : ShapeTuple(std::vector<index_type>()) {}

    /*!
   * \brief Constructor from iterator
   * \param begin begin of iterator
   * \param end end of iterator
   * \tparam IterType The type of iterator
   */
    template<typename Iter>
    ShapeTuple(Iter begin, Iter end) : ShapeTuple(std::vector<index_type>(begin, end)) {}

    /*!
   * \brief constructor from initializer list
   * \param shape The initializer list
   */
    ShapeTuple(std::initializer_list<index_type> shape) : ShapeTuple(shape.begin(), shape.end()) {}

    /*!
   * \brief Construct a new ShapeTuple object
   *
   * \param shape The moved/copied std::vector object
   *
   * \note If user passes const reference, it will trigger copy. If it's rvalue,
   * it will be moved into other.
   */
    explicit ShapeTuple(std::vector<index_type> shape);

    /*!
   * \brief Return the data pointer
   *
   * \return const index_type* data pointer
   */
    NODISCARD const index_type* data() const {
        return get()->data();
    }

    /*!
   * \brief Return the size of the shape tuple
   *
   * \return size_t shape tuple size
   */
    NODISCARD size_t size() const {
        return get()->size();
    }

    /*!
   * \brief Immutably read i-th element from the shape tuple.
   * \param idx The index
   * \return the i-th element.
   */
    index_type operator[](size_t idx) const {
        CHECK(idx < size()) << "IndexError: indexing " << idx << " on an array of size " << size();
        return data()[idx];
    }

    /*!
   * \brief Immutably read i-th element from the shape tuple.
   * \param idx The index
   * \return the i-th element.
   */
    NODISCARD index_type at(size_t idx) const {
        return operator[](idx);
    }

    /*! \return Whether shape tuple is empty */
    NODISCARD bool empty() const {
        return size() == 0;
    }

    /*! \return The first element of the shape tuple */
    NODISCARD index_type front() const {
        return at(0);
    }

    /*! \return The last element of the shape tuple */
    NODISCARD index_type back() const {
        return at(size() - 1);
    }

    NODISCARD const index_type* begin() const {
        return data();
    }

    NODISCARD const index_type* end() const {
        return data() + size();
    }

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(ShapeTuple, ObjectRef, ShapeTupleNode);
};

inline ShapeTupleNode::index_type ShapeTupleNode::Product() const {
    index_type numel = 1;
    for (index_type x: this->data_container) {
        numel *= x;
    }
    return numel;
}

inline ShapeTuple::ShapeTuple(std::vector<index_type> shape) {
    auto ptr = make_object<ShapeTupleNode>(std::move(shape));
    data_ = std::move(ptr);
}

inline std::ostream& operator<<(std::ostream& os, const ShapeTuple& shape) {
    size_t n = shape.size();
    os << '[';
    for (size_t i = 0; i < n; ++i) {
        os << shape[i] << (i != n - 1 ? ", " : "");
    }
    os << ']';
    return os;
}

using IntTuple = ShapeTuple;
using IntTupleNode = ShapeTupleNode;

}// namespace litetvm::runtime

// namespace std {
//
// template<>
// struct std::formatter<litetvm::runtime::ShapeTuple> {
//     static auto parse(format_parse_context& ctx) {
//         return ctx.begin();
//     }
//
//     static auto format(const litetvm::runtime::ShapeTuple& shape, format_context& ctx) {
//         std::string s;
//         size_t n = shape.size();
//         s += '[';
//         for (size_t i = 0; i < n; ++i) {
//             s += std::to_string(shape[i]) + (i != n - 1 ? ", " : "");
//         }
//         s += ']';
//         return std::format_to(ctx.out(), s.c_str());
//     }
//
// };
//
// }

#endif//SHAPE_TUPLE_H
