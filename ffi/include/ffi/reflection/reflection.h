//
// Created by 赵丹 on 25-5-20.
//

#ifndef LITETVM_FFI_REFLECTION_H
#define LITETVM_FFI_REFLECTION_H

#include "ffi/any.h"
#include "ffi/c_api.h"
#include "ffi/function.h"
#include "ffi/type_traits.h"

#include <string>

namespace litetvm {
namespace ffi {
namespace details {

/*!
 * \brief Get the byte offset of a class member field.
 *
 * \tparam The original class.
 * \tparam T the field type.
 *
 * \param field_ptr A class member pointer
 * \returns The byteoffset
 */
template<typename Class, typename T>
int64_t GetFieldByteOffsetToObject(T Class::* field_ptr) {
    int64_t field_offset_to_class = reinterpret_cast<int64_t>(&(static_cast<Class*>(nullptr)->*field_ptr));
    return field_offset_to_class - ObjectUnsafe::GetObjectOffsetToSubclass<Class>();
}

struct ReflectionDefFinish {};

class ReflectionDef {
public:
    explicit ReflectionDef(int32_t type_index) : type_index_(type_index) {}

    template<typename Class, typename T>
    ReflectionDef& def_readonly(const char* name, T Class::* field_ptr) {
        RegisterField(name, field_ptr, true);
        return *this;
    }

    template<typename Class, typename T>
    ReflectionDef& def_readwrite(const char* name, T Class::* field_ptr) {
        RegisterField(name, field_ptr, false);
        return *this;
    }

private:
    template<typename Class, typename T>
    void RegisterField(const char* name, T Class::* field_ptr, bool readonly) {
        TVMFFIFieldInfo info;
        info.name = TVMFFIByteArray{name, std::char_traits<char>::length(name)};
        info.field_static_type_index = TypeToFieldStaticTypeIndex<T>::value;
        // store byte offset and setter, getter
        // so the same setter can be reused for all the same type
        info.byte_offset = GetFieldByteOffsetToObject<Class, T>(field_ptr);
        info.readonly = readonly;
        info.getter = FieldGetter<T>;
        info.setter = FieldSetter<T>;
        TVM_FFI_CHECK_SAFE_CALL(TVMFFIRegisterTypeField(type_index_, &info));
    }

    template<typename T>
    static int FieldGetter(void* field, TVMFFIAny* result) {
        TVM_FFI_SAFE_CALL_BEGIN();
        *result = AnyUnsafe::MoveAnyToTVMFFIAny(Any(*reinterpret_cast<T*>(field)));
        TVM_FFI_SAFE_CALL_END();
    }

    template<typename T>
    static int FieldSetter(void* field, const TVMFFIAny* value) {
        TVM_FFI_SAFE_CALL_BEGIN();
        *static_cast<T*>(field) = AnyView::CopyFromTVMFFIAny(*value).cast<T>();
        TVM_FFI_SAFE_CALL_END();
    }

    int32_t type_index_;
};

/*!
 * \brief helper function to get reflection field info by type key and field name
 */
inline const TVMFFIFieldInfo* GetReflectionFieldInfo(std::string_view type_key, const char* field_name) {
    int32_t type_index;
    TVMFFIByteArray type_key_array = {type_key.data(), type_key.size()};
    TVM_FFI_CHECK_SAFE_CALL(TVMFFITypeKeyToIndex(&type_key_array, &type_index));
    const TypeInfo* info = TVMFFIGetTypeInfo(type_index);
    for (int32_t i = 0; i < info->num_fields; ++i) {
        if (std::strncmp(info->fields[i].name.data, field_name, info->fields[i].name.size) == 0) {
            return &(info->fields[i]);
        }
    }
    TVM_FFI_THROW(RuntimeError) << "Cannot find field " << field_name << " in " << type_key;
}

/*!
 * \brief helper wrapper class to obtain a getter.
 */
class ReflectionFieldGetter {
public:
    explicit ReflectionFieldGetter(const TVMFFIFieldInfo* field_info) : field_info_(field_info) {}

    Any operator()(const Object* obj_ptr) const {
        Any result;
        const void* addr = reinterpret_cast<const char*>(obj_ptr) + field_info_->byte_offset;
        TVM_FFI_CHECK_SAFE_CALL(field_info_->getter(const_cast<void*>(addr), reinterpret_cast<TVMFFIAny*>(&result)));
        return result;
    }

    Any operator()(const ObjectPtr<Object>& obj_ptr) const {
        return operator()(obj_ptr.get());
    }

    Any operator()(const ObjectRef& obj) const {
        return operator()(obj.get());
    }

private:
    const TVMFFIFieldInfo* field_info_;
};

#define TVM_FFI_REFLECTION_REG_VAR_DEF \
    static inline TVM_FFI_ATTRIBUTE_UNUSED ::litetvm::ffi::details::ReflectionDef& __TVMFFIReflectionReg

/*!
 * helper macro to define a reflection definition for an object
 */
#define TVM_FFI_REFLECTION_DEF(TypeName)                              \
    TVM_FFI_STR_CONCAT(TVM_FFI_REFLECTION_REG_VAR_DEF, __COUNTER__) = \
            ::litetvm::ffi::details::ReflectionDef(TypeName::_GetOrAllocRuntimeTypeIndex())
}// namespace details
}// namespace ffi
}// namespace litetvm

#endif//LITETVM_FFI_REFLECTION_H
