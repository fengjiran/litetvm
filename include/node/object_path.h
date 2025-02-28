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

    const ObjectPathNode* ParentNode() const;

    /*! Compares just the last node of the path, without comparing the whole path. */
    virtual bool LastNodeEqual(const ObjectPathNode* other) const = 0;

    virtual std::string LastNodeString() const = 0;

private:
    Optional<ObjectRef> parent_;
    int32_t length_;

    friend class ObjectPath;
    friend std::string GetObjectPathRepr(const ObjectPathNode* node);
};

class ObjectPath : public ObjectRef {
public:
    /*! \brief Create a path that represents the root object itself. */
    static ObjectPath Root(Optional<String> name = NullOpt);

    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(ObjectPath, ObjectRef, ObjectPathNode);
};


//-------------------------------------------------------------------------
//-----   Concrete object path nodes   ------------------------------------
//-------------------------------------------------------------------------

// ----- Root -----

class RootPathNode final : public ObjectPathNode {
public:
    Optional<String> name;

    explicit RootPathNode(const Optional<String>& name = NullOpt);

    static constexpr const char* _type_key = "RootPath";
    TVM_DECLARE_FINAL_OBJECT_INFO(RootPathNode, ObjectPathNode);

protected:
    bool LastNodeEqual(const ObjectPathNode* other) const override;
    std::string LastNodeString() const override;
};

class RootPath : public ObjectPath {
public:
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(RootPath, ObjectPath, RootPathNode);
};

// ----- Attribute access -----

class AttributeAccessPathNode final : public ObjectPathNode {
public:
    /*! \brief Name of the attribute being accessed. Must be a static string. */
    String attr_key;

    explicit AttributeAccessPathNode(const ObjectPathNode* parent, String attr_key);

    static constexpr const char* _type_key = "AttributeAccessPath";
    TVM_DECLARE_FINAL_OBJECT_INFO(AttributeAccessPathNode, ObjectPathNode);

protected:
    bool LastNodeEqual(const ObjectPathNode* other) const override;
    std::string LastNodeString() const override;
};

class AttributeAccessPath : public ObjectPath {
public:
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(AttributeAccessPath, ObjectPath,
                                              AttributeAccessPathNode);
};

// ----- Unknown attribute access -----

class UnknownAttributeAccessPathNode final : public ObjectPathNode {
public:
    explicit UnknownAttributeAccessPathNode(const ObjectPathNode* parent);

    static constexpr const char* _type_key = "UnknownAttributeAccessPath";
    TVM_DECLARE_FINAL_OBJECT_INFO(UnknownAttributeAccessPathNode, ObjectPathNode);

protected:
    bool LastNodeEqual(const ObjectPathNode* other) const final;
    std::string LastNodeString() const final;
};

class UnknownAttributeAccessPath : public ObjectPath {
public:
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(UnknownAttributeAccessPath, ObjectPath,
                                              UnknownAttributeAccessPathNode);
};

// ----- Array element access by index -----

class ArrayIndexPathNode : public ObjectPathNode {
public:
    /*! \brief Index of the array element that is being accessed. */
    int32_t index;

    explicit ArrayIndexPathNode(const ObjectPathNode* parent, int32_t index);

    static constexpr const char* _type_key = "ArrayIndexPath";
    TVM_DECLARE_FINAL_OBJECT_INFO(ArrayIndexPathNode, ObjectPathNode);

protected:
    bool LastNodeEqual(const ObjectPathNode* other) const final;
    std::string LastNodeString() const final;
};

class ArrayIndexPath : public ObjectPath {
public:
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(ArrayIndexPath, ObjectPath, ArrayIndexPathNode);
};

// ----- Missing array element -----

class MissingArrayElementPathNode : public ObjectPathNode {
public:
    /*! \brief Index of the array element that is missing. */
    int32_t index;

    explicit MissingArrayElementPathNode(const ObjectPathNode* parent, int32_t index);

    static constexpr const char* _type_key = "MissingArrayElementPath";
    TVM_DECLARE_FINAL_OBJECT_INFO(MissingArrayElementPathNode, ObjectPathNode);

protected:
    bool LastNodeEqual(const ObjectPathNode* other) const final;
    std::string LastNodeString() const final;
};

class MissingArrayElementPath : public ObjectPath {
public:
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(MissingArrayElementPath, ObjectPath,
                                              MissingArrayElementPathNode);
};

// ----- Map value -----

class MapValuePathNode : public ObjectPathNode {
public:
    /*! \brief Key of the map entry that is being accessed */
    ObjectRef key;

    explicit MapValuePathNode(const ObjectPathNode* parent, ObjectRef key);

    static constexpr const char* _type_key = "MapValuePath";
    TVM_DECLARE_FINAL_OBJECT_INFO(MapValuePathNode, ObjectPathNode);

protected:
    bool LastNodeEqual(const ObjectPathNode* other) const final;
    std::string LastNodeString() const final;
};

class MapValuePath : public ObjectPath {
public:
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(MapValuePath, ObjectPath, MapValuePathNode);
};

// ----- Missing map entry -----

class MissingMapEntryPathNode : public ObjectPathNode {
public:
    explicit MissingMapEntryPathNode(const ObjectPathNode* parent);

    static constexpr const char* _type_key = "MissingMapEntryPath";
    TVM_DECLARE_FINAL_OBJECT_INFO(MissingMapEntryPathNode, ObjectPathNode);

protected:
    bool LastNodeEqual(const ObjectPathNode* other) const final;
    std::string LastNodeString() const final;
};

class MissingMapEntryPath : public ObjectPath {
public:
    TVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(MissingMapEntryPath, ObjectPath,
                                              MissingMapEntryPathNode);
};


}// namespace litetvm

#endif//OBJECT_PATH_H
