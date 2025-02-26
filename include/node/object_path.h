//
// Created by richard on 2/26/25.
//

#ifndef OBJECT_PATH_H
#define OBJECT_PATH_H

#include "runtime/object.h"
#include "runtime/optional.h"
#include "runtime/string.h"

namespace litetvm {

using runtime::Object;
using runtime::ObjectPtr;
using runtime::ObjectRef;
using runtime::String;
using runtime::StringObj;

class ObjectPath;

/*!
 * \brief Path to an object from some root object.
 *
 * Motivation:
 *
 * Same IR node object can be referenced in several different contexts inside a larger IR object.
 * For example, a variable could be referenced in several statements within a block.
 *
 * This makes it impossible to use an object pointer to uniquely identify a "location" within
 * the larger IR object for error reporting purposes. The ObjectPath class addresses this problem
 * by serving as a unique "locator".
 */
class ObjectPathNode : public Object {
public:
    /*! \brief Get the parent path */
    NODISCARD Optional<ObjectPath> GetParent() const;
    /*!
   * \brief Get the length of the path.
   *
   * For example, the path returned by `ObjectPath::Root()` has length 1.
   */
    NODISCARD int32_t Length() const {
        return length_;
    }

    /*!
   * \brief Get a path prefix of the given length.
   *
   * Provided `length` must not exceed the `Length()` of this path.
   */
    ObjectPath GetPrefix(int32_t length) const;

    /*!
   * \brief Check if this path is a prefix of another path.
   *
   * The prefix is not strict, i.e. a path is considered a prefix of itself.
   */
    bool IsPrefixOf(const ObjectPath& other) const;

    /*! \brief Check if two paths are equal. */
    bool PathsEqual(const ObjectPath& other) const;

    /*! \brief Extend this path with access to an object attribute. */
    ObjectPath Attr(const char* attr_key) const;

    /*! \brief Extend this path with access to an object attribute. */
    ObjectPath Attr(Optional<String> attr_key) const;

    /*! \brief Extend this path with access to an array element. */
    ObjectPath ArrayIndex(int32_t index) const;

    /*! \brief Extend this path with access to a missing array element. */
    ObjectPath MissingArrayElement(int32_t index) const;

    /*! \brief Extend this path with access to a map value. */
    ObjectPath MapValue(ObjectRef key) const;

    /*! \brief Extend this path with access to a missing map entry. */
    ObjectPath MissingMapEntry() const;

    static constexpr const char* _type_key = "ObjectPath";
    TVM_DECLARE_BASE_OBJECT_INFO(ObjectPathNode, Object);

protected:
    explicit ObjectPathNode(const ObjectPathNode* parent)
        : parent_(GetRef<ObjectRef>(parent)), length_(parent ? parent->length_ + 1 : 1) {}

    friend class ObjectPath;
    friend std::string GetObjectPathRepr(const ObjectPathNode* node);

    const ObjectPathNode* ParentNode() const;

    /*! Compares just the last node of the path, without comparing the whole path. */
    virtual bool LastNodeEqual(const ObjectPathNode* other) const = 0;

    virtual std::string LastNodeString() const = 0;

private:
    Optional<ObjectRef> parent_;
    int32_t length_;
};

class ObjectPath : public ObjectRef {
public:
    /*! \brief Create a path that represents the root object itself. */
    static ObjectPath Root(Optional<String> name = NullOpt);

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(ObjectPath, ObjectRef, ObjectPathNode);
};


}// namespace litetvm

#endif//OBJECT_PATH_H
