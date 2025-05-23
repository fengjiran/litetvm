//
// Created by richard on 1/30/25.
//

#ifndef PACKED_FUNC_H
#define PACKED_FUNC_H

#include "runtime/array.h"
#include "runtime/boxed_primitive.h"
#include "runtime/c_runtime_api.h"
#include "runtime/data_type.h"
#include "runtime/map.h"
#include "runtime/module.h"
#include "runtime/ndarray.h"
#include "runtime/object.h"
#include "runtime/object_type_checker.h"
#include "runtime/optional.h"
#include "runtime/string.h"
#include "runtime/variant.h"

#include <functional>
#include <utility>

// nested namespace
namespace litetvm::runtime {

// forward declarations
class TVMArgs;
class TVMArgValue;
class TVMRetValue;
class TVMArgsSetter;
class TVMMovableArgValueWithContext_;
template<typename FType>
class TypedPackedFunc;

/*!
 * \brief Object container class that backs PackedFunc.
 * \note Do not use this function directly, use PackedFunc.
 */
class PackedFuncObj : public Object {
public:
    /*!
     * \brief Call the function in packed format.
     * \param args The arguments
     * \param rv The return value.
     */
    TVM_ALWAYS_INLINE void CallPacked(TVMArgs args, TVMRetValue* rv) const;

    static constexpr uint32_t _type_index = static_cast<uint32_t>(TypeIndex::kRuntimePackedFunc);
    static constexpr const char* _type_key = "runtime.PackedFunc";
    TVM_DECLARE_FINAL_OBJECT_INFO(PackedFuncObj, Object);

    /*! \brief Delete the default constructor explicitly. */
    PackedFuncObj() = delete;

protected:
    /*!
     * \brief Internal struct for extracting the callable method from callable type.
     */
    template<typename TPackedFuncSubObj>
    struct Extractor {
        /*!
         * \brief Extracting the callable method from callable type.
         * \param obj The base packed function object class.
         * \param args The arguments
         * \param rv The return value.
         */
        static void Call(const PackedFuncObj* obj, TVMArgs args, TVMRetValue* rv);
    };

    /*! \brief The internal callable function type. */
    // using FCallPacked = void(const PackedFuncObj*, TVMArgs, TVMRetValue*);
    using FCallPacked = std::function<void(const PackedFuncObj*, TVMArgs, TVMRetValue*)>;

    /*!
     * \brief Constructing a packed function object from a function pointer.
     * \param f_call_pack The function pointer used to call the packed function.
     */
    explicit PackedFuncObj(FCallPacked f_call_pack) : f_call_packed_(std::move(f_call_pack)) {}

    /*! \brief Internal callable function pointer used to call the packed function. */
    FCallPacked f_call_packed_;
};

/*! \brief Derived object class for constructing PackedFuncObj. */
template<typename TCallable>
class PackedFuncSubObj : public PackedFuncObj {
    using TStorage = std::remove_cv_t<std::remove_reference_t<TCallable>>;

public:
    /*! \brief The type of derived object class */
    using TSelf = PackedFuncSubObj;
    /*!
     * \brief Derived object class for constructing PackedFuncObj.
     * \param callable The type-erased callable object.
     */
    explicit PackedFuncSubObj(TCallable callable)
        : PackedFuncObj(Extractor<TSelf>::Call), callable_(callable) {}
    /*! \brief Type-erased filed for storing callable object*/
    mutable TStorage callable_;
};

/*!
 * \brief Packed function is a type-erased function.
 *  The arguments are passed by packed format.
 *
 *  This is a useful-unified interface to call generated functions,
 *  It is the unified function type of TVM.
 *  It corresponds to TVMFunctionHandle in C runtime API.
 */
class PackedFunc : public ObjectRef {
public:
    /*! \brief Constructor from null */
    PackedFunc(std::nullptr_t) : ObjectRef(nullptr) {}// NOLINT(*)

    /*!
   * \brief Constructing a packed function from a callable type
   *        whose signature is consistent with `PackedFunc`
   * \param data the internal container of packed function.
   */
    template<typename TCallable,
             typename = std::enable_if_t<
                     std::is_convertible_v<TCallable, std::function<void(TVMArgs, TVMRetValue*)>> &&
                     !std::is_base_of_v<TCallable, PackedFunc>>>
    explicit PackedFunc(TCallable data) {
        using ObjType = PackedFuncSubObj<TCallable>;
        data_ = make_object<ObjType>(std::forward<TCallable>(data));
    }

    /*!
   * \brief Call packed function by directly passing in unpacked format.
   * \param args Arguments to be passed.
   * \tparam Args arguments to be passed.
   *
   * \code
   *   // Example code on how to call packed function
   *   void CallPacked(PackedFunc f) {
   *     // call like normal functions by pass in arguments
   *     // return value is automatically converted back
   *     int rvalue = f(1, 2.0);
   *   }
   * \endcode
   */
    template<typename... Args>
    TVMRetValue operator()(Args&&... args) const;
    /*!
   * \brief Call the function in packed format.
   * \param args The arguments
   * \param rv The return value.
   */
    TVM_ALWAYS_INLINE void CallPacked(TVMArgs args, TVMRetValue* rv) const;
    /*! \return Whether the packed function is nullptr */
    bool operator==(std::nullptr_t) const { return data_ == nullptr; }
    /*! \return Whether the packed function is not nullptr */
    bool operator!=(std::nullptr_t) const { return data_ != nullptr; }

    TVM_DEFINE_OBJECT_REF_METHODS(PackedFunc, ObjectRef, PackedFuncObj);
};

/*! \brief Using static function to output TypedPackedFunc signature */
using FSig = std::string();


/*!
 * \anchor TypedPackedFuncAnchor
 * \brief A PackedFunc wrapper to provide typed function signature.
 * It is backed by a PackedFunc internally.
 *
 * TypedPackedFunc enables compile time type checking.
 * TypedPackedFunc works with the runtime system:
 * - It can be passed as an argument of PackedFunc.
 * - It can be assigned to TVMRetValue.
 * - It can be directly converted to a type-erased PackedFunc.
 *
 * Developers should prefer TypedPackedFunc over PackedFunc in C++ code
 * as it enables compile time checking.
 * We can construct a TypedPackedFunc from a lambda function
 * with the same signature.
 *
 * \code
 *  // user defined lambda function.
 *  auto addone = [](int x)->int {
 *    return x + 1;
 *  };
 *  // We can directly convert
 *  // lambda function to TypedPackedFunc
 *  TypedPackedFunc<int(int)> ftyped(addone);
 *  // invoke the function.
 *  int y = ftyped(1);
 *  // Can be directly converted to PackedFunc
 *  PackedFunc packed = ftype;
 * \endcode
 * \tparam R The return value of the function.
 * \tparam Args The argument signature of the function.
 */
template<typename R, typename... Args>
class TypedPackedFunc<R(Args...)> {
public:
    /*! \brief shorthand for this function type */
    using TSelf = TypedPackedFunc;
    /*! \brief default constructor */
    TypedPackedFunc() = default;
    /*! \brief constructor from null */
    TypedPackedFunc(std::nullptr_t) {}// NOLINT(*)
    /*!
   * \brief construct by wrap a PackedFunc
   *
   * Example usage:
   * \code
   * PackedFunc packed([](TVMArgs args, TVMRetValue *rv) {
   *   int x = args[0];
   *   *rv = x + 1;
   *  });
   * // construct from packed function
   * TypedPackedFunc<int(int)> ftyped(packed);
   * // call the typed version.
   * ICHECK_EQ(ftyped(1), 2);
   * \endcode
   *
   * \param packed The packed function
   */
    explicit TypedPackedFunc(PackedFunc packed);

    /*!
   * \brief constructor from TVMRetValue
   * \param value The TVMRetValue
   */
    explicit TypedPackedFunc(const TVMRetValue& value);

    /*!
   * \brief constructor from TVMArgValue
   * \param value The TVMArgValue
   */
    TypedPackedFunc(const TVMArgValue& value);// NOLINT(*)

    /*!
   * \brief constructor from TVMMovableArgValue_
   * \param value The TVMMovableArgValue_
   */
    TypedPackedFunc(TVMMovableArgValueWithContext_&& value);// NOLINT(*)

    /*!
   * \brief construct from a lambda function with the same signature.
   *
   * Example usage:
   * \code
   * auto typed_lambda = [](int x)->int { return x + 1; }
   * // construct from packed function
   * TypedPackedFunc<int(int)> ftyped(typed_lambda, "add_one");
   * // call the typed version.
   * ICHECK_EQ(ftyped(1), 2);
   * \endcode
   *
   * \param typed_lambda typed lambda function.
   * \param name the name of the lambda function.
   * \tparam FLambda the type of the lambda function.
   */
    template<typename FLambda,
             typename = std::enable_if_t<std::is_convertible_v<FLambda, std::function<R(Args...)>>>>
    TypedPackedFunc(const FLambda& typed_lambda, std::string name) {// NOLINT(*)
        this->AssignTypedLambda(typed_lambda, name);
    }
    /*!
   * \brief construct from a lambda function with the same signature.
   *
   * This version does not take a name. It is highly recommend you use the
   * version that takes a name for the lambda.
   *
   * Example usage:
   * \code
   * auto typed_lambda = [](int x)->int { return x + 1; }
   * // construct from packed function
   * TypedPackedFunc<int(int)> ftyped(typed_lambda);
   * // call the typed version.
   * ICHECK_EQ(ftyped(1), 2);
   * \endcode
   *
   * \param typed_lambda typed lambda function.
   * \tparam FLambda the type of the lambda function.
   */
    template<typename FLambda,
             typename = std::enable_if_t<std::is_convertible_v<FLambda, std::function<R(Args...)>>>>
    TypedPackedFunc(const FLambda& typed_lambda) {// NOLINT(*)
        this->AssignTypedLambda(typed_lambda);
    }
    /*!
   * \brief copy assignment operator from typed lambda
   *
   * Example usage:
   * \code
   * // construct from packed function
   * TypedPackedFunc<int(int)> ftyped;
   * ftyped = [](int x) { return x + 1; }
   * // call the typed version.
   * ICHECK_EQ(ftyped(1), 2);
   * \endcode
   *
   * \param typed_lambda typed lambda function.
   * \tparam FLambda the type of the lambda function.
   * \returns reference to self.
   */
    template<typename FLambda,
             typename = std::enable_if_t<std::is_convertible_v<FLambda, std::function<R(Args...)>>>>
    TSelf& operator=(FLambda typed_lambda) {// NOLINT(*)
        this->AssignTypedLambda(typed_lambda);
        return *this;
    }

    /*!
   * \brief copy assignment operator from PackedFunc.
   * \param packed The packed function.
   * \returns reference to self.
   */
    TSelf& operator=(const PackedFunc& packed) {
        packed_ = packed;
        return *this;
    }

    /*!
   * \brief Invoke the operator.
   * \param args The arguments
   * \returns The return value.
   */
    TVM_ALWAYS_INLINE R operator()(Args... args) const;
    /*!
   * \brief convert to PackedFunc
   * \return the internal PackedFunc
   */
    operator PackedFunc() const { return packed(); }
    /*!
   * \return reference the internal PackedFunc
   */
    const PackedFunc& packed() const { return packed_; }
    /*! \return Whether the packed function is nullptr */
    bool operator==(std::nullptr_t) const { return packed_ == nullptr; }
    /*! \return Whether the packed function is not nullptr */
    bool operator!=(std::nullptr_t) const { return packed_ != nullptr; }

private:
    friend class TVMRetValue;
    /*! \brief The internal packed function */
    PackedFunc packed_;

    /*!
   * \brief Assign the packed field using a typed lambda function.
   *
   * \param flambda The lambda function.
   * \param name The name associated with this lambda.
   * \tparam FLambda The lambda function type.
   * \note We capture the lambda when possible for maximum efficiency.
   */
    template<typename FLambda>
    void AssignTypedLambda(FLambda flambda, std::string name);
    /*!
   * \brief Assign the packed field using a typed lambda function. This variant is for functions
   * without names.
   *
   * \param flambda The lambda function.
   * \tparam FLambda The lambda function type.
   * \note We capture the lambda when possible for maximum efficiency.
   */
    template<typename FLambda>
    void AssignTypedLambda(FLambda flambda);
};

/*! \brief Arguments into TVM functions. */
class TVMArgs {
public:
    const TVMValue* values;
    const int* type_codes;
    int num_args;
    /*!
     * \brief constructor
     * \param values The argument values
     * \param type_codes The argument type codes
     * \param num_args number of arguments.
     */
    TVMArgs(const TVMValue* values, const int* type_codes, int num_args)
        : values(values), type_codes(type_codes), num_args(num_args) {}

    /*! \return size of the arguments */
    NODISCARD int size() const {
        return num_args;
    }

    /*!
     * \brief Get i-th argument
     * \param i the index.
     * \return the ith argument.
     */
    inline TVMArgValue operator[](int i) const;
    /*!
     * \brief Get the i-th argument and do proper type checking with detailed error messages.
     * \tparam T The expected type.
     * \param i The index
     * \return The corresponding argument value.
     */
    template<typename T>
    T At(int i) const;
};

/*!
 * \brief Convert argument type code to string.
 * \param type_code The input type code.
 * \return The corresponding string repr.
 */
inline const char* ArgTypeCode2Str(int type_code);

inline std::ostream& operator<<(std::ostream& os, DLDevice dev);// NOLINT(*)

#define TVM_LOG_INCORRECT_TYPE_CODE(CODE, T) \
    "expected " << ArgTypeCode2Str(T) << " but got " << ArgTypeCode2Str(CODE)

// macro to check type code.
#define TVM_CHECK_TYPE_CODE(CODE, T) CHECK_EQ(CODE, T) << TVM_LOG_INCORRECT_TYPE_CODE(CODE, T)


/*!
 * \brief Internal base class to
 *  handle conversion to POD values.
 */
class TVMPODValue_ {
public:
    explicit operator void*() const {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {
            return nullptr;
        }

        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMDLTensorHandle)) {
            return value_.v_handle;
        }

        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMOpaqueHandle));
        return value_.v_handle;
    }

    explicit operator DLTensor*() const {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMDLTensorHandle) ||
            type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle)) {
            return static_cast<DLTensor*>(value_.v_handle);
        }

        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {
            return nullptr;
        }

        LOG(FATAL) << "Expected DLTensor* or NDArray but got " << ArgTypeCode2Str(type_code_);
        return nullptr;
    }

    explicit operator NDArray() const {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {
            return NDArray(ObjectPtr<Object>(nullptr));
        }

        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle));
        return NDArray(NDArray::FFIDataFromHandle(static_cast<TVMArrayHandle>(value_.v_handle)));
    }

    explicit operator Module() const {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {
            return Module(ObjectPtr<Object>(nullptr));
        }

        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMModuleHandle));
        return Module(ObjectPtr<Object>(static_cast<Object*>(value_.v_handle)));
    }

    operator PackedFunc() const {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {
            return PackedFunc(ObjectPtr<Object>(nullptr));
        }

        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle));
        return PackedFunc(ObjectPtr<Object>(static_cast<Object*>(value_.v_handle)));
    }

    explicit operator Device() const {
        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kDLDevice));
        return value_.v_device;
    }

    NODISCARD int type_code() const {
        return type_code_;
    }
    /*!
   * \brief return handle as specific pointer type.
   * \tparam T the data type.
   * \return The pointer type.
   */
    template<typename T>
    NODISCARD T* ptr() const {
        return static_cast<T*>(value_.v_handle);
    }

    NODISCARD std::optional<bool> TryAsBool() const {
        // Helper function to reduce duplication in the variable integer
        // conversions.  This is publicly exposed, as it can be useful in
        // specializations of PackedFuncValueConverter.
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMArgBool)) {
            return static_cast<bool>(value_.v_int64);
        }

        return std::nullopt;
    }

    NODISCARD std::optional<int64_t> TryAsInt() const {
        // Helper function to reduce duplication in the variable integer
        // conversions.  This is publicly exposed, as it can be useful in
        // specializations of PackedFuncValueConverter.
        if (type_code_ == static_cast<int>(DLDataTypeCode::kDLInt)) {
            return value_.v_int64;
        }

        return std::nullopt;
    }

    NODISCARD std::optional<double> TryAsFloat() const {
        // Helper function to reduce duplication in the variable integer
        // conversions.  This is publicly exposed, as it can be useful in
        // specializations of PackedFuncValueConverter.
        if (type_code_ == static_cast<int>(DLDataTypeCode::kDLFloat)) {
            return value_.v_float64;
        }
        return std::nullopt;
    }

protected:
    friend class TVMArgsSetter;
    friend class TVMRetValue;
    friend class TVMMovableArgValue_;

    TVMPODValue_() : type_code_(static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {}

    TVMPODValue_(TVMValue value, int type_code) : value_(value), type_code_(type_code) {}

    /*! \brief The value */
    TVMValue value_{};
    /*! \brief the type code */
    int type_code_;
};

/*! \brief A utility class that adds methods useful for each POD type
 *
 * These cannot be provided in the base PODValue_ class, because
 * TVMArgValue and TVMRetValue have different semantics for kTVMStr
 * and kTVMBytes.
 *
 * kTVMStr:
 *
 *     For `TVMArgValue`, the active variant is `v_str`, a `const
 *     char*`.  For `TVMRetValue`, the active variant is `v_handle`,
 *     and should be cast from `void*` to `std::string*`.
 *
 * kTVMBytes:
 *
 *     The active variant is `v_handle`, a `void*`.  For
 *     `TVMArgValue`, should be cast to `TVMByteArray*`.  For
 *     `TVMRetValue`, should be cast to `std::string*`.
 *
 * When converting into an `ObjectRef`, a string may be used to build
 * a `tvm::runtime::String`.  Because TVMArgValue and TVMRetValue use
 * different representations for strings, any utility funciton which
 * might attempt a conversion to an `ObjectRef` must be performed
 * within a context that is aware of the derived class.
 */
template<typename Derived>
class TVMPODValue_CRTP_ : public TVMPODValue_ {
public:
    using TVMPODValue_::TVMPODValue_;

    // ObjectRef handling
    template<typename TObjectRef,
             typename = std::enable_if_t<std::is_base_of_v<ObjectRef, TObjectRef>>>
    NODISCARD bool IsObjectRef() const;

    template<typename TObjectRef>
    TObjectRef AsObjectRef() const;

    explicit operator double() const {
        // Allow automatic conversion from int to float
        // This avoids errors when user pass in int from
        // the frontend while the API expects a float.
        if (auto opt = TryAsFloat()) {
            return opt.value();
        }

        if (auto opt = TryAsInt()) {
            return opt.value();
        }

        if (auto opt = TryAsBool()) {
            return opt.value();
        }

        LOG(FATAL) << TVM_LOG_INCORRECT_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kDLDevice));
        return 0;
    }

    explicit operator int64_t() const {
        if (auto opt = TryAsInt()) {
            return opt.value();
        }

        if (auto opt = TryAsBool()) {
            return opt.value();
        }

        LOG(FATAL) << TVM_LOG_INCORRECT_TYPE_CODE(type_code_, static_cast<int>(DLDataTypeCode::kDLInt));
    }

    explicit operator uint64_t() const {
        return operator int64_t();
    }

    explicit operator int() const {
        int64_t value = operator int64_t();
        CHECK_LE(value, std::numeric_limits<int>::max());
        CHECK_GE(value, std::numeric_limits<int>::min());
        return value;
    }

    operator bool() const {
        if (auto opt = TryAsBool()) {
            return opt.value();
        }

        if (auto opt = TryAsInt()) {
            return opt.value();
        }

        LOG(FATAL) << TVM_LOG_INCORRECT_TYPE_CODE(type_code_, static_cast<int>(DLDataTypeCode::kDLInt));
    }
};


/*!
 * \brief A single argument value to PackedFunc.
 *  Containing both type_code and TVMValue
 *
 *  Provides utilities to do type cast into other types.
 */
class TVMArgValue : public TVMPODValue_CRTP_<TVMArgValue> {
public:
    /*! \brief default constructor */
    TVMArgValue() = default;
    /*!
     * \brief constructor
     * \param value of the function
     * \param type_code The type code.
     */
    TVMArgValue(TVMValue value, int type_code) : TVMPODValue_CRTP_(value, type_code) {}

    // reuse converter from parent
    using TVMPODValue_::operator void*;
    using TVMPODValue_::operator DLTensor*;
    using TVMPODValue_::operator NDArray;
    using TVMPODValue_::operator Device;
    using TVMPODValue_::operator Module;
    using TVMPODValue_::operator PackedFunc;
    using TVMPODValue_CRTP_::operator double;
    using TVMPODValue_CRTP_::operator int64_t;
    using TVMPODValue_CRTP_::operator uint64_t;
    using TVMPODValue_CRTP_::operator int;
    using TVMPODValue_CRTP_::operator bool;
    using TVMPODValue_CRTP_::AsObjectRef;
    using TVMPODValue_CRTP_::IsObjectRef;

    // conversion operator.
    operator std::string() const {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMDataType)) {
            return DLDataType2String(operator DLDataType());
        }

        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMBytes)) {
            auto* arr = static_cast<TVMByteArray*>(value_.v_handle);
            return {arr->data, arr->size};
        }

        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMStr)) {
            return {value_.v_str};
        }

        return AsObjectRef<String>().operator std::string();
    }

    template<typename FType>
    explicit operator TypedPackedFunc<FType>() const {
        return TypedPackedFunc<FType>(operator PackedFunc());
    }

    NODISCARD const TVMValue& value() const {
        return value_;
    }

    template<typename T,
             typename = std::enable_if_t<std::is_class_v<T>>>
    operator T() const;

    inline explicit operator DLDataType() const;
    inline explicit operator DataType() const;
};


/*!
 * \brief Internal auxiliary struct for TypedPackedFunc to indicate a movable argument.
 *
 *  We can only construct a movable argument once from a single argument position.
 *  If the argument is passed as RValue reference, the result will be moved.
 *  We should only construct a MovableArg from an argument once,
 *  as the result will can moved.
 *
 * \note For internal development purpose only.
 */
class TVMMovableArgValue_ : public TVMPODValue_CRTP_<TVMMovableArgValue_> {
public:
    TVMMovableArgValue_(TVMValue value, int type_code) : TVMPODValue_CRTP_(value, type_code) {}

    // reuse converter from parent
    using TVMPODValue_::operator void*;
    using TVMPODValue_::operator DLTensor*;
    using TVMPODValue_::operator NDArray;
    using TVMPODValue_::operator Device;
    using TVMPODValue_::operator Module;
    using TVMPODValue_::operator PackedFunc;
    using TVMPODValue_CRTP_::operator double;
    using TVMPODValue_CRTP_::operator int64_t;
    using TVMPODValue_CRTP_::operator uint64_t;
    using TVMPODValue_CRTP_::operator int;
    using TVMPODValue_CRTP_::operator bool;

    // reuse conversion rule from ArgValue.
    operator std::string() const {
        return AsArgValue().operator std::string();
    }

    template<typename FType>
    operator TypedPackedFunc<FType>() const {
        return TypedPackedFunc<FType>(operator PackedFunc());
    }

    operator DLDataType() const {
        return AsArgValue().operator DLDataType();
    }

    operator DataType() const {
        return AsArgValue().operator DataType();
    }

    operator TVMArgValue() const {
        return AsArgValue();
    }
    /*!
     * \brief Helper converter function.
     *  Try to move out an argument if possible,
     *  fall back to normal argument conversion rule otherwise.
     */
    template<typename T,
             typename = std::enable_if_t<std::is_base_of_v<ObjectRef, T>>>
    operator T() const;

private:
    /*! \return The arg value repr of the value. */
    NODISCARD TVMArgValue AsArgValue() const {
        return {value_, type_code_};
    }
};

/*!
 * \brief Internal auxiliary struct for TypedPackedFunc to indicate a movable argument with
 * additional context information (function name and argument index) for better error reporting.
 *
 * \sa MovableArgValue_
 * \note For internal development purpose only.
 */
class TVMMovableArgValueWithContext_ {
public:
    /*!
     * \brief move constructor from another return value.
     * \param value The other return value.
     * \param type_code The code associated with the type of the value.
     * \param arg_index In a function call, this argument is at index arg_index (0-indexed).
     * \param optional_name Name of the function being called. Can be nullptr if the function is not.
     * \param f_sig Pointer to static function outputting signature of the function being called.
     * named.
     */
    TVMMovableArgValueWithContext_(TVMValue value, int type_code, int arg_index,
                                   const std::string* optional_name, FSig* f_sig)
        : value_(value, type_code),
          arg_index_(arg_index),
          optional_name_(optional_name),
          f_sig_(f_sig) {}

    template<typename T>
    operator T() const {
        return static_cast<T>(value_);
    }

private:
    TVMMovableArgValue_ value_;
    int arg_index_;
    const std::string* optional_name_;
    FSig* f_sig_;
};


/*!
 * \brief Return Value container,
 *  Unlike TVMArgValue, which only holds reference and do not delete
 *  the underlying container during destruction.
 *
 *  TVMRetValue holds value and will manage the underlying containers
 *  when it stores a complicated data type.
 */
class TVMRetValue : public TVMPODValue_CRTP_<TVMRetValue> {
public:
    /*! \brief default constructor */
    TVMRetValue() = default;

    TVMRetValue(const TVMRetValue& other) : TVMPODValue_CRTP_() {
        this->Assign(other);
    }

    /*!
   * \brief move constructor from another return value.
   * \param other The other return value.
   */
    TVMRetValue(TVMRetValue&& other) noexcept : TVMPODValue_CRTP_(other.value_, other.type_code_) {
        other.value_.v_handle = nullptr;
        other.type_code_ = static_cast<int>(TVMArgTypeCode::kTVMNullptr);
    }

    /*! \brief destructor */
    ~TVMRetValue() {
        this->Clear();
    }

    // reuse converter from parent
    using TVMPODValue_CRTP_::operator double;
    using TVMPODValue_CRTP_::operator int64_t;
    using TVMPODValue_CRTP_::operator uint64_t;
    using TVMPODValue_CRTP_::operator int;
    using TVMPODValue_CRTP_::operator bool;
    using TVMPODValue_::operator void*;
    using TVMPODValue_::operator DLTensor*;
    using TVMPODValue_::operator Device;
    using TVMPODValue_::operator NDArray;
    using TVMPODValue_::operator Module;
    using TVMPODValue_::operator PackedFunc;
    using TVMPODValue_CRTP_::AsObjectRef;
    using TVMPODValue_CRTP_::IsObjectRef;

    // conversion operators
    operator DLDataType() const {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMStr)) {
            return String2DLDataType(operator std::string());
        }

        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMDataType));
        return value_.v_type;
    }

    operator DataType() const {
        return DataType(operator DLDataType());
    }

    operator std::string() const {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMDataType)) {
            return DLDataType2String(operator DLDataType());
        }

        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMBytes)) {
            return *ptr<std::string>();
        }

        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMStr));
        return *ptr<std::string>();
    }

    template<typename FType>
    operator TypedPackedFunc<FType>() const {
        return TypedPackedFunc<FType>(operator PackedFunc());
    }

    // Assign operators
    TVMRetValue& operator=(TVMRetValue&& other) noexcept {
        this->Clear();
        value_ = other.value_;
        type_code_ = other.type_code_;
        other.type_code_ = static_cast<int>(TVMArgTypeCode::kTVMNullptr);
        return *this;
    }

    TVMRetValue& operator=(double value) {
        this->SwitchToPOD(static_cast<int>(DLDataTypeCode::kDLFloat));
        value_.v_float64 = value;
        return *this;
    }

    TVMRetValue& operator=(std::nullptr_t value) {
        this->SwitchToPOD(static_cast<int>(TVMArgTypeCode::kTVMNullptr));
        value_.v_handle = value;
        return *this;
    }

    TVMRetValue& operator=(void* value) {
        this->SwitchToPOD(static_cast<int>(TVMArgTypeCode::kTVMOpaqueHandle));
        value_.v_handle = value;
        return *this;
    }

    TVMRetValue& operator=(int64_t value) {
        this->SwitchToPOD(static_cast<int>(DLDataTypeCode::kDLInt));
        value_.v_int64 = value;
        return *this;
    }

    TVMRetValue& operator=(int value) {
        this->SwitchToPOD(static_cast<int>(DLDataTypeCode::kDLInt));
        value_.v_int64 = value;
        return *this;
    }

    TVMRetValue& operator=(DLDevice value) {
        this->SwitchToPOD(static_cast<int>(TVMArgTypeCode::kDLDevice));
        value_.v_device = value;
        return *this;
    }

    TVMRetValue& operator=(DLDataType t) {
        this->SwitchToPOD(static_cast<int>(TVMArgTypeCode::kTVMDataType));
        value_.v_type = t;
        return *this;
    }

    TVMRetValue& operator=(const DataType& other) {
        return operator=(other.operator DLDataType());
    }

    TVMRetValue& operator=(bool value) {
        this->SwitchToPOD(static_cast<int>(TVMArgTypeCode::kTVMArgBool));
        value_.v_int64 = value;
        return *this;
    }

    TVMRetValue& operator=(const std::string& value) {
        this->SwitchToClass(static_cast<int>(TVMArgTypeCode::kTVMStr), value);
        return *this;
    }

    TVMRetValue& operator=(TVMByteArray value) {
        this->SwitchToClass(static_cast<int>(TVMArgTypeCode::kTVMBytes), std::string(value.data, value.size));
        return *this;
    }

    TVMRetValue& operator=(NDArray other) {
        if (other.data_ != nullptr) {
            this->Clear();
            type_code_ = static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle);
            value_.v_handle = NDArray::FFIGetHandle(other);
            ObjectRef::FFIClearAfterMove(&other);
        } else {
            SwitchToPOD(static_cast<int>(TVMArgTypeCode::kTVMNullptr));
            value_.v_handle = nullptr;
        }
        return *this;
    }

    TVMRetValue& operator=(Module m) {
        SwitchToObject(static_cast<int>(TVMArgTypeCode::kTVMModuleHandle), std::move(m.data_));
        return *this;
    }

    TVMRetValue& operator=(PackedFunc f) {
        this->SwitchToObject(static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle), std::move(f.data_));
        return *this;
    }

    template<typename FType>
    TVMRetValue& operator=(const TypedPackedFunc<FType>& f) {
        return operator=(f.packed());
    }

    TVMRetValue& operator=(const TVMRetValue& other) {// NOLINT(*0
        this->Assign(other);
        return *this;
    }

    TVMRetValue& operator=(const TVMArgValue& other) {
        this->Assign(other);
        return *this;
    }

    TVMRetValue& operator=(TVMMovableArgValue_&& other) {
        this->Assign(other);
        return *this;
    }

    /*!
   * \brief Move the value back to front-end via C API.
   *  This marks the current container as null.
   *  The managed resources are moved to the front-end.
   *  The front end should take charge in managing them.
   *
   * \param ret_value The return value.
   * \param ret_type_code The return type code.
   */
    void MoveToCHost(TVMValue* ret_value, int* ret_type_code) {
        // cannot move str; need specially handle.
        CHECK(type_code_ != static_cast<int>(TVMArgTypeCode::kTVMStr) &&
              type_code_ != static_cast<int>(TVMArgTypeCode::kTVMBytes));
        *ret_value = value_;
        *ret_type_code = type_code_;
        type_code_ = static_cast<int>(TVMArgTypeCode::kTVMNullptr);
    }

    /*!
   * \brief Construct a new TVMRetValue by
   *        moving from return value stored via C API.
   * \param value the value.
   * \param type_code The type code.
   * \return The created TVMRetValue.
   */
    static TVMRetValue MoveFromCHost(TVMValue value, int type_code) {
        // Can move POD and everything under the object system.
        CHECK(type_code <= static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle) ||
              type_code == static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle) ||
              type_code == static_cast<int>(TVMArgTypeCode::kTVMArgBool));
        TVMRetValue ret;
        ret.value_ = value;
        ret.type_code_ = type_code;
        return ret;
    }

    /*! \return The value field, if the data is POD */
    NODISCARD const TVMValue& value() const {
        CHECK(type_code_ != static_cast<int>(TVMArgTypeCode::kTVMObjectHandle) &&
              type_code_ != static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle) &&
              type_code_ != static_cast<int>(TVMArgTypeCode::kTVMModuleHandle) &&
              type_code_ != static_cast<int>(TVMArgTypeCode::kTVMStr))
                << "TVMRetValue.value can only be used for POD data";
        return value_;
    }

    // ObjectRef handling
    template<typename TObjectRef,
             typename = std::enable_if_t<std::is_base_of_v<ObjectRef, TObjectRef>>>
    TVMRetValue& operator=(TObjectRef other);

    template<typename T,
             typename = std::enable_if_t<std::is_class_v<T>>>
    operator T() const;

private:
    // get the internal container.
    void SwitchToPOD(int type_code) {
        if (type_code_ != type_code) {
            this->Clear();
            type_code_ = type_code;
        }
    }

    template<typename T>
    void SwitchToClass(int type_code, const T& v) {
        if (type_code_ != type_code) {
            this->Clear();
            type_code_ = type_code;
            value_.v_handle = new T(v);
        } else {
            *static_cast<T*>(value_.v_handle) = v;
        }
    }

    void SwitchToObject(int type_code, ObjectPtr<Object> other) {
        if (other.data_ != nullptr) {
            this->Clear();
            type_code_ = type_code;
            // move the handle out
            value_.v_handle = other.data_;
            other.data_ = nullptr;
        } else {
            SwitchToPOD(static_cast<int>(TVMArgTypeCode::kTVMNullptr));
            value_.v_handle = nullptr;
        }
    }

    template<typename T>
    void Assign(const T& other) {
        switch (other.type_code()) {
            case static_cast<int>(TVMArgTypeCode::kTVMStr): {
                SwitchToClass<std::string>(static_cast<int>(TVMArgTypeCode::kTVMStr), other);
                break;
            }

            case static_cast<int>(TVMArgTypeCode::kTVMBytes): {
                SwitchToClass<std::string>(static_cast<int>(TVMArgTypeCode::kTVMBytes), other);
                break;
            }

            case static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle): {
                *this = other.operator PackedFunc();
                break;
            }

            case static_cast<int>(TVMArgTypeCode::kTVMModuleHandle): {
                *this = other.operator Module();
                break;
            }

            case static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle): {
                *this = other.operator NDArray();
                break;
            }

            case static_cast<int>(TVMArgTypeCode::kTVMObjectRValueRefArg):
            case static_cast<int>(TVMArgTypeCode::kTVMObjectHandle): {
                // We have already known it is not NDArray/Module, but
                // operator=(ObjectRef) also handles conversions from wrappers
                // around primitive types.  For NDArray/Module, the duplicate
                // checks are removed with if constexpr.
                operator=(other.operator ObjectRef());
                break;
            }

                // case static_cast<int>(TVMArgTypeCode::kTVMObjectRValueRefArg): {
                //     operator=(other.operator ObjectRef());
                //     break;
                // }

            default: {
                SwitchToPOD(other.type_code());
                value_ = other.value_;
                break;
            }
        }
    }

    void Clear() {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNullptr))
            return;

        switch (type_code_) {
            case static_cast<int>(TVMArgTypeCode::kTVMStr):
            case static_cast<int>(TVMArgTypeCode::kTVMBytes): {
                delete ptr<std::string>();
                break;
            }

            case static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle): {
                NDArray::FFIDecRef(static_cast<TVMArrayHandle>(value_.v_handle));
                break;
            }

            case static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle):
            case static_cast<int>(TVMArgTypeCode::kTVMModuleHandle):
            case static_cast<int>(TVMArgTypeCode::kTVMObjectHandle): {
                static_cast<Object*>(value_.v_handle)->DecRef();
                break;
            }

            default:
                type_code_ = static_cast<int>(TVMArgTypeCode::kTVMNullptr);
                break;
        }

        // type_code_ = static_cast<int>(TVMArgTypeCode::kTVMNullptr);
    }
};

/*!
 * \brief Type trait to specify special value conversion rules from
 *        TVMArgValue and TVMRetValue.
 *
 *  The trait can be specialized to add type specific conversion logic
 *  from the TVMArgvalue and TVMRetValue.
 *
 * \tparam TObjectRef the specific ObjectRefType.
 */
template<typename TObjectRef>
struct PackedFuncValueConverter {
    /*!
     * \brief Convert a TObjectRef from an argument value.
     * \param val The argument value.
     * \return the converted result.
     */
    static TObjectRef From(const TVMArgValue& val) {
        return val.AsObjectRef<TObjectRef>();
    }

    /*!
     * \brief Convert a TObjectRef from a return value.
     * \param val The argument value.
     * \return the converted result.
     */
    static TObjectRef From(const TVMRetValue& val) {
        return val.AsObjectRef<TObjectRef>();
    }
};

inline TVMArgValue TVMArgs::operator[](int i) const {
    CHECK_LT(i, num_args) << "not enough argument passed, " << num_args << " passed"
                          << " but request arg[" << i << "].";
    return {values[i], type_codes[i]};
}

template<typename T>
T TVMArgs::At(int i) const {
    TVMArgValue arg = operator[](i);
    return arg.operator T();
}

template<class TPackedFuncSubObj>
void PackedFuncObj::Extractor<TPackedFuncSubObj>::Call(const PackedFuncObj* obj, TVMArgs args,
                                                       TVMRetValue* rv) {
    static_cast<const TPackedFuncSubObj*>(obj)->callable_(args, rv);
}

TVM_ALWAYS_INLINE void PackedFuncObj::CallPacked(TVMArgs args, TVMRetValue* rv) const {
    f_call_packed_(this, args, rv);
}

TVM_ALWAYS_INLINE void PackedFunc::CallPacked(TVMArgs args, TVMRetValue* rv) const {
    static_cast<PackedFuncObj*>(data_.get())->CallPacked(args, rv);
}

// internal namespace
inline const char* ArgTypeCode2Str(int type_code) {
    switch (type_code) {
        case static_cast<int>(DLDataTypeCode::kDLInt):
            return "int";
        case static_cast<int>(TVMArgTypeCode::kTVMArgBool):
            return "bool";
        case static_cast<int>(DLDataTypeCode::kDLUInt):
            return "uint";
        case static_cast<int>(DLDataTypeCode::kDLFloat):
            return "float";
        case static_cast<int>(TVMArgTypeCode::kTVMStr):
            return "str";
        case static_cast<int>(TVMArgTypeCode::kTVMBytes):
            return "bytes";
        case static_cast<int>(TVMArgTypeCode::kTVMOpaqueHandle):
            return "handle";
        case static_cast<int>(TVMArgTypeCode::kTVMNullptr):
            return "NULL";
        case static_cast<int>(TVMArgTypeCode::kTVMDLTensorHandle):
            return "ArrayHandle";
        case static_cast<int>(TVMArgTypeCode::kTVMDataType):
            return "DLDataType";
        case static_cast<int>(TVMArgTypeCode::kDLDevice):
            return "DLDevice";
        case static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle):
            return "FunctionHandle";
        case static_cast<int>(TVMArgTypeCode::kTVMModuleHandle):
            return "ModuleHandle";
        case static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle):
            return "NDArrayContainer";
        case static_cast<int>(TVMArgTypeCode::kTVMObjectHandle):
            return "Object";
        case static_cast<int>(TVMArgTypeCode::kTVMObjectRValueRefArg):
            return "ObjectRValueRefArg";
        default:
            LOG(FATAL) << "unknown type_code=" << type_code;
    }
    throw;
}

/*!
 * \brief The name of DLDeviceType.
 * \param type The device type.
 * \return the device name.
 */
inline const char* DLDeviceType2Str(int type) {
    switch (type) {
        case static_cast<int>(DLDeviceType::kDLCPU):
            return "cpu";
        case static_cast<int>(DLDeviceType::kDLCUDA):
            return "cuda";
        case static_cast<int>(DLDeviceType::kDLCUDAHost):
            return "cuda_host";
        case static_cast<int>(DLDeviceType::kDLCUDAManaged):
            return "cuda_managed";
        case static_cast<int>(DLDeviceType::kDLOpenCL):
            return "opencl";
        case static_cast<int>(TVMDeviceExtType::kDLSDAccel):
            return "sdaccel";
        case static_cast<int>(TVMDeviceExtType::kDLAOCL):
            return "aocl";
        case static_cast<int>(DLDeviceType::kDLVulkan):
            return "vulkan";
        case static_cast<int>(DLDeviceType::kDLMetal):
            return "metal";
        case static_cast<int>(DLDeviceType::kDLVPI):
            return "vpi";
        case static_cast<int>(DLDeviceType::kDLROCM):
            return "rocm";
        case static_cast<int>(DLDeviceType::kDLROCMHost):
            return "rocm_host";
        case static_cast<int>(DLDeviceType::kDLExtDev):
            return "ext_dev";
        case static_cast<int>(DLDeviceType::kDLOneAPI):
            return "oneapi";
        case static_cast<int>(DLDeviceType::kDLWebGPU):
            return "webgpu";
        case static_cast<int>(DLDeviceType::kDLHexagon):
            return "hexagon";
        case static_cast<int>(TVMDeviceExtType::kOpenGL):
            return "opengl";
        case static_cast<int>(TVMDeviceExtType::kDLMicroDev):
            return "microdev";
        default:
            LOG(FATAL) << "unknown type = " << type;
    }
    throw;
}

namespace detail {

template<typename F, typename... Args>
void for_each(const F& f, Args&&... args) {
    using IntArray = int[];
    int i = 0;
    (void) IntArray{(f(i++, std::forward<Args>(args)), 0)...};
}

// template<bool stop, std::size_t I, typename F>
// struct for_each_dispatcher {
//     template<typename T, typename... Args>
//     static void run(const F& f, T&& value, Args&&... args) {// NOLINT(*)
//         f(I, std::forward<T>(value));
//         for_each_dispatcher<sizeof...(Args) == 0, (I + 1), F>::run(f, std::forward<Args>(args)...);
//     }
// };
//
// template<std::size_t I, typename F>
// struct for_each_dispatcher<true, I, F> {
//     static void run(const F& f) {}// NOLINT(*)
// };

// template<typename F, typename... Args>
// void for_each(const F& f, Args&&... args) {// NOLINT(*)
//     for_each_dispatcher<sizeof...(Args) == 0, 0, F>::run(f, std::forward<Args>(args)...);
// }

template<typename T>
struct ModuleVTableEntryHelper {};

template<typename T, typename R, typename... Args>
struct ModuleVTableEntryHelper<R (T::*)(Args...) const> {
    using MemFnType = R (T::*)(Args...) const;
    using IndexSeq = std::index_sequence_for<Args...>;
    static constexpr const std::size_t LenArgs = sizeof...(Args);

    template<std::size_t... Is>
    static TVM_ALWAYS_INLINE void Call(TVMRetValue* rv, T* self, MemFnType f, TVMArgs args,
                                       std::index_sequence<Is...>) {
        *rv = (self->*f)(args[Is]...);
    }
};

template<typename T, typename R, typename... Args>
struct ModuleVTableEntryHelper<R (T::*)(Args...)> {
    using MemFnType = R (T::*)(Args...);
    using IndexSeq = std::index_sequence_for<Args...>;
    static constexpr const std::size_t LenArgs = sizeof...(Args);

    template<std::size_t... Is>
    static TVM_ALWAYS_INLINE void Call(TVMRetValue* rv, T* self, MemFnType f, TVMArgs args,
                                       std::index_sequence<Is...>) {
        *rv = (self->*f)(args[Is]...);
    }
};

template<typename T, typename... Args>
struct ModuleVTableEntryHelper<void (T::*)(Args...) const> {
    using MemFnType = void (T::*)(Args...) const;
    using IndexSeq = std::index_sequence_for<Args...>;
    static constexpr const std::size_t LenArgs = sizeof...(Args);

    template<std::size_t... Is>
    static TVM_ALWAYS_INLINE void Call(TVMRetValue* rv, T* self, MemFnType f, TVMArgs args,
                                       std::index_sequence<Is...>) {
        (self->*f)(args[Is]...);
    }
};

template<typename T, typename... Args>
struct ModuleVTableEntryHelper<void (T::*)(Args...)> {
    using MemFnType = void (T::*)(Args...);
    using IndexSeq = std::index_sequence_for<Args...>;
    static constexpr const std::size_t LenArgs = sizeof...(Args);

    template<std::size_t... Is>
    static TVM_ALWAYS_INLINE void Call(TVMRetValue* rv, T* self, MemFnType f, TVMArgs args,
                                       std::index_sequence<Is...>) {
        (self->*f)(args[Is]...);
    }
};

namespace parameter_pack {

template<typename... EnumArgs>
struct EnumeratedParamPack {
    struct InvokeWithoutArg {
        template<template<size_t i, typename TArgument> class Functor,
                 typename ExtraParams>
        static void F(ExtraParams&& extra_params) {
            using TExpander = int[];
            (void) TExpander{
                    0,
                    (Functor<EnumArgs::i, typename EnumArgs::T>::F(std::forward<ExtraParams>(extra_params)), 0)...,
            };
        }
    };

    struct InvokeWithArg {
        template<template<size_t i, typename TArgument> class Functor,
                 typename ExtraParams,
                 typename... Params>
        static void F(ExtraParams&& extra_params, Params&&... params) {
            using TExpander = int[];
            (void) TExpander{
                    0,
                    (Functor<EnumArgs::i, typename EnumArgs::T>::F(std::forward<ExtraParams>(extra_params),
                                                                   std::forward<Params>(params)),
                     0)...,
            };
        }
    };
};

template<typename... Args>
struct EnumerateImpl {
private:
    template<size_t _i, typename _T>
    struct Item {
        static constexpr size_t i = _i;
        using T = _T;
    };

    template<typename...>
    struct Zipper;

    template<std::size_t... id>
    struct Zipper<std::integer_sequence<std::size_t, id...>> {
        using WithoutArg = typename EnumeratedParamPack<Item<id, Args>...>::InvokeWithoutArg;
        using WithArg = typename EnumeratedParamPack<Item<id, Args>...>::InvokeWithArg;
    };

public:
    using WithoutArg = typename Zipper<std::index_sequence_for<Args...>>::WithoutArg;
    using WithArg = typename Zipper<std::index_sequence_for<Args...>>::WithArg;
};

template<typename... Args>
using EnumerateWithoutArg = typename EnumerateImpl<Args...>::WithoutArg;

template<typename... Args>
using EnumerateWithArg = typename EnumerateImpl<Args...>::WithArg;

template<typename... Args>
struct ParamPack {
    template<template<size_t i, typename TArgument> class Functor,
             typename ExtraParams>
    static void InvokeWithoutArg(ExtraParams&& extra_params) {
        EnumerateWithoutArg<Args...>::template F<Functor, ExtraParams>(std::forward<ExtraParams>(extra_params));
    }
};

}// namespace parameter_pack

/*!
 * \brief Template class to get function signature of a function or functor.
 * \tparam T The function/functor type.
 */
template<typename T>
struct func_signature_helper {
    using FType = void;
};

template<typename T, typename R, typename... Args>
struct func_signature_helper<R (T::*)(Args...)> {
    using FType = R(Args...);
    using ParamType = parameter_pack::ParamPack<Args...>;
    using RetType = R;
    static_assert(!std::is_reference_v<R>, "TypedPackedFunc return reference");
};

template<typename T, typename R, typename... Args>
struct func_signature_helper<R (T::*)(Args...) const> {
    using FType = R(Args...);
    using ParamType = parameter_pack::ParamPack<Args...>;
    using RetType = R;
    static_assert(!std::is_reference_v<R>, "TypedPackedFunc return reference");
};

/*!
 * \brief Template class to get function signature of a function or functor.
 * \tparam T The function/functor type.
 */
template<typename T>
struct function_signature {
    using FType = typename func_signature_helper<decltype(&T::operator())>::FType;
    using ParamType = typename func_signature_helper<decltype(&T::operator())>::ParamType;
    using RetType = typename func_signature_helper<decltype(&T::operator())>::RetType;
};

// handle case of function.
template<typename R, typename... Args>
struct function_signature<R(Args...)> {
    using FType = R(Args...);
    using ParamType = parameter_pack::ParamPack<Args...>;
    using RetType = R;
    static_assert(!std::is_reference_v<R>, "TypedPackedFunc return reference");
};

// handle case of function ptr.
template<typename R, typename... Args>
struct function_signature<R (*)(Args...)> {
    using FType = R(Args...);
    using ParamType = parameter_pack::ParamPack<Args...>;
    using RetType = R;
    static_assert(!std::is_reference_v<R>, "TypedPackedFunc return reference");
};

template<typename TSignature>
struct SignaturePrinter;

namespace type2str {

template<typename T>
struct TypeSimplifier;

template<typename T>
struct Type2Str {
    template<typename = std::enable_if_t<std::is_base_of_v<ObjectRef, T>>>
    static std::string v() {
        return T::ContainerType::_type_key;
    }
};
template<>
struct Type2Str<int> {
    static std::string v() { return "int"; }
};
template<>
struct Type2Str<double> {
    static std::string v() { return "double"; }
};
template<>
struct Type2Str<int64_t> {
    static std::string v() { return "int64_t"; }
};
template<>
struct Type2Str<uint64_t> {
    static std::string v() { return "uint64_t"; }
};
template<>
struct Type2Str<bool> {
    static std::string v() { return "bool"; }
};
template<>
struct Type2Str<void> {
    static std::string v() { return "void"; }
};
template<>
struct Type2Str<std::basic_string<char>> {
    static std::string v() { return "basic_string<char>"; }
};
template<typename K, typename V>
struct Type2Str<Map<K, V>> {
    static std::string v() {
        return "Map<" + TypeSimplifier<K>::v() + ", " + TypeSimplifier<V>::v() + ">";
    }
};
template<>
struct Type2Str<DLDevice> {
    static std::string v() { return "DLDevice"; }
};
template<>
struct Type2Str<DLTensor> {
    static std::string v() { return "DLTensor"; }
};
template<>
struct Type2Str<DataType> {
    static std::string v() { return "DataType"; }
};
template<>
struct Type2Str<DLDataType> {
    static std::string v() { return "DLDataType"; }
};
template<>
struct Type2Str<TVMRetValue> {
    static std::string v() { return "TVMRetValue"; }
};
template<>
struct Type2Str<TVMArgValue> {
    static std::string v() { return "TVMArgValue"; }
};
template<>
struct Type2Str<TVMByteArray> {
    static std::string v() { return "TVMByteArray"; }
};
template<typename FType>
struct Type2Str<TypedPackedFunc<FType>> {
    static std::string v() { return SignaturePrinter<function_signature<FType>>::F(); }
};
template<typename T>
struct Type2Str<Array<T>> {
    static std::string v() { return "Array<" + TypeSimplifier<T>::v() + ">"; }
};

/*!
 * \brief Template class to remove const, pointer and reference of original type.
 * \tparam T The original type.
 */
template<typename T>
struct TypeSimplifier {
    static std::string v() {
        using U = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;
        return (std::is_const_v<T> ? "const " : "") + Type2Str<U>::v() +
               (std::is_pointer_v<T> ? "*" : "") + (std::is_reference_v<T> ? "&" : "");
    }
};

}// namespace type2str

/*!
 * \brief Template class to generate static function outputting signature of a function or functor.
 * \tparam TSignature The function/functor signature type generated by `function_signature`.
 */
template<typename TSignature>
struct SignaturePrinter {
    using ParamType = typename TSignature::ParamType;
    using RetType = typename TSignature::RetType;

    template<size_t i, typename TArgument>
    struct PrintParamType {
        static void F(std::ostream& os) {
            os << (i == 0 ? "" : ", ") << i << ": " << type2str::TypeSimplifier<TArgument>::v();
        }
    };

    static std::string F() {
        std::ostringstream oss;
        oss << "(";
        ParamType::template InvokeWithoutArg<PrintParamType>(oss);
        oss << ") -> " << type2str::TypeSimplifier<RetType>::v();
        return oss.str();
    }
};
}// namespace detail


/* \brief argument setter to PackedFunc */
class TVMArgsSetter {
public:
    TVMArgsSetter(TVMValue* values, int* type_codes) : values_(values), type_codes_(type_codes) {}

    // setters for POD types
    template<typename T,
             typename = std::enable_if_t<std::is_integral_v<T>>>
    TVM_ALWAYS_INLINE void operator()(size_t i, T value) const {
        values_[i].v_int64 = static_cast<int64_t>(value);
        type_codes_[i] = static_cast<int>(DLDataTypeCode::kDLInt);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, bool value) const {
        values_[i].v_int64 = value;
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMArgBool);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, uint64_t value) const {
        values_[i].v_int64 = static_cast<int64_t>(value);
        CHECK_LE(value, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
        type_codes_[i] = static_cast<int>(DLDataTypeCode::kDLInt);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, double value) const {
        values_[i].v_float64 = value;
        type_codes_[i] = static_cast<int>(DLDataTypeCode::kDLFloat);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, std::nullptr_t value) const {
        values_[i].v_handle = value;
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMNullptr);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, const TVMArgValue& value) const {
        values_[i] = value.value_;
        type_codes_[i] = value.type_code_;
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, void* value) const {
        values_[i].v_handle = value;
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMOpaqueHandle);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, DLTensor* value) const {
        values_[i].v_handle = value;
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMDLTensorHandle);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, Device value) const {
        values_[i].v_device = value;
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kDLDevice);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, DLDataType value) const {
        values_[i].v_type = value;
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMDataType);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, DataType dtype) const {
        operator()(i, dtype.operator DLDataType());
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, const char* value) const {
        values_[i].v_str = value;
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMStr);
    }

    // setters for container types
    TVM_ALWAYS_INLINE void operator()(size_t i, const std::string& value) const {
        values_[i].v_str = value.c_str();
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMStr);
    }

    TVM_ALWAYS_INLINE void operator()(size_t i, const TVMByteArray& value) const {
        values_[i].v_handle = const_cast<TVMByteArray*>(&value);
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMBytes);
    }

    template<typename FType>
    TVM_ALWAYS_INLINE void operator()(size_t i, const TypedPackedFunc<FType>& value) const {
        operator()(i, value.packed());
    }

    void operator()(size_t i, const TVMRetValue& value) const {
        if (value.type_code() == static_cast<int>(TVMArgTypeCode::kTVMStr)) {
            values_[i].v_str = value.ptr<std::string>()->c_str();
            type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMStr);
        } else {
            CHECK_NE(value.type_code(), static_cast<int>(TVMArgTypeCode::kTVMBytes)) << "not handled.";
            values_[i] = value.value_;
            type_codes_[i] = value.type_code();
        }
    }

    // ObjectRef handling
    template<typename TObjectRef,
             typename = std::enable_if_t<std::is_base_of_v<ObjectRef, TObjectRef>>>
    TVM_ALWAYS_INLINE void operator()(size_t i, const TObjectRef& value) const {
        this->SetObject(i, value);
    }

    template<typename TObjectRef,
             typename = std::enable_if_t<std::is_base_of_v<ObjectRef, std::remove_reference_t<TObjectRef>>>>
    TVM_ALWAYS_INLINE void operator()(size_t i, TObjectRef&& value) const {
        this->SetObject(i, std::forward<TObjectRef>(value));
    }

private:
    template<typename TObjectRef>
    void SetObject(size_t i, TObjectRef&& value) const;
    /*! \brief The values fields */
    TVMValue* values_;
    /*! \brief The type code fields */
    int* type_codes_;
};

template<typename... Args>
TVMRetValue PackedFunc::operator()(Args&&... args) const {
    const int kNumArgs = sizeof...(Args);
    const int kArraySize = kNumArgs > 0 ? kNumArgs : 1;
    TVMValue values[kArraySize];
    int type_codes[kArraySize];
    detail::for_each(TVMArgsSetter(values, type_codes), std::forward<Args>(args)...);
    TVMRetValue rv;
    static_cast<PackedFuncObj*>(data_.get())->CallPacked(TVMArgs(values, type_codes, kNumArgs), &rv);
    return rv;
}

template<size_t i, typename T>
struct TVMArgsSetterApply {
    static TVM_ALWAYS_INLINE void F(TVMArgsSetter* setter, T&& value) {
        (*setter)(i, std::forward<T>(value));
    }
};

template<typename... Args>
void TVM_ALWAYS_INLINE PackArgs(TVMValue* values, int* type_codes, Args&&... args) {
    TVMArgsSetter setter(values, type_codes);
    detail::parameter_pack::EnumerateWithArg<Args...>::template F<TVMArgsSetterApply>(
            &setter, std::forward<Args>(args)...);
}

namespace detail {
template<typename R, int nleft, int index, typename F>
struct unpack_call_dispatcher {
    template<typename... Args>
    TVM_ALWAYS_INLINE static void run(const std::string* optional_name, FSig* f_sig, const F& f,
                                      const TVMArgs& args_pack, TVMRetValue* rv,
                                      Args&&... unpacked_args) {
        // construct a movable argument value
        // which allows potential move of argument to the input of F.
        unpack_call_dispatcher<R, nleft - 1, index + 1, F>::run(
                optional_name, f_sig, f, args_pack, rv, std::forward<Args>(unpacked_args)...,
                TVMMovableArgValueWithContext_(args_pack.values[index], args_pack.type_codes[index], index,
                                               optional_name, f_sig));
    }
};

template<typename R, int index, typename F>
struct unpack_call_dispatcher<R, 0, index, F> {
    template<typename... Args>
    TVM_ALWAYS_INLINE static void run(const std::string* optional_name, FSig* f_sig, const F& f,
                                      const TVMArgs& args_pack, TVMRetValue* rv,
                                      Args&&... unpacked_args) {
        using RetType = decltype(f(std::forward<Args>(unpacked_args)...));
        if (std::is_same<RetType, R>::value) {
            *rv = f(std::forward<Args>(unpacked_args)...);
        } else {
            *rv = R(f(std::forward<Args>(unpacked_args)...));
        }
    }
};

template<int index, typename F>
struct unpack_call_dispatcher<void, 0, index, F> {
    template<typename... Args>
    TVM_ALWAYS_INLINE static void run(const std::string* optional_name, FSig* f_sig, const F& f,
                                      const TVMArgs& args_pack, TVMRetValue* rv,
                                      Args&&... unpacked_args) {
        f(std::forward<Args>(unpacked_args)...);
    }
};

template<typename R, int nargs, typename F>
TVM_ALWAYS_INLINE void unpack_call(const std::string* optional_name, const F& f,
                                   const TVMArgs& args, TVMRetValue* rv) {
    FSig* f_sig = detail::SignaturePrinter<detail::function_signature<F>>::F;
    CHECK_EQ(nargs, args.size()) << "Function "
                                 << (optional_name == nullptr ? "<anonymous>" : *optional_name)
                                 << (f_sig == nullptr ? "" : (*f_sig)()) << " expects " << nargs
                                 << " arguments but " << args.size() << " were provided";
    unpack_call_dispatcher<R, nargs, 0, F>::run(optional_name, f_sig, f, args, rv);
}

template<typename FType>
struct unpack_call_by_signature {};

template<typename R, typename... Args>
struct unpack_call_by_signature<R(Args...)> {
    template<typename F>
    TVM_ALWAYS_INLINE static void run(const F& f, const TVMArgs& args, TVMRetValue* rv) {
        unpack_call<R, sizeof...(Args)>(nullptr, f, args, rv);
    }
};

template<typename R, typename... Args>
TVM_ALWAYS_INLINE R call_packed(const PackedFunc& pf, Args&&... args) {
    return R(pf(std::forward<Args>(args)...));
}

template<typename R>
struct typed_packed_call_dispatcher {
    template<typename... Args>
    TVM_ALWAYS_INLINE static R run(const PackedFunc& pf, Args&&... args) {
        return pf(std::forward<Args>(args)...);
    }
};

template<>
struct typed_packed_call_dispatcher<void> {
    template<typename... Args>
    TVM_ALWAYS_INLINE static void run(const PackedFunc& pf, Args&&... args) {
        pf(std::forward<Args>(args)...);
    }
};
}// namespace detail

template<typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(PackedFunc packed) : packed_(std::move(packed)) {}

template<typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(const TVMRetValue& value)
    : packed_(value.operator PackedFunc()) {}

template<typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(const TVMArgValue& value)
    : packed_(value.operator PackedFunc()) {}

template<typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(TVMMovableArgValueWithContext_&& value)
    : packed_(value.operator PackedFunc()) {}

template<typename R, typename... Args>
template<typename FType>
void TypedPackedFunc<R(Args...)>::AssignTypedLambda(FType flambda, std::string name) {
    FSig* f_sig = detail::SignaturePrinter<detail::function_signature<FType>>::F;
    packed_ = PackedFunc([flambda, name, f_sig](const TVMArgs& args, TVMRetValue* rv) {
        if (args.size() != sizeof...(Args)) {
            LOG(FATAL) << "Function " << name << (f_sig == nullptr ? "" : (*f_sig)()) << " expects "
                       << sizeof...(Args) << " arguments, but " << args.size() << " were provided.";
        }
        detail::unpack_call<R, sizeof...(Args)>(&name, flambda, args, rv);
    });
}

template<typename R, typename... Args>
template<typename FType>
void TypedPackedFunc<R(Args...)>::AssignTypedLambda(FType flambda) {
    FSig* f_sig = detail::SignaturePrinter<detail::function_signature<FType>>::F;
    packed_ = PackedFunc([flambda, f_sig](const TVMArgs& args, TVMRetValue* rv) {
        if (args.size() != sizeof...(Args)) {
            LOG(FATAL) << "Function <anonymous> " << (*f_sig)() << " expects " << sizeof...(Args)
                       << " arguments, but " << args.size() << " were provided.";
        }
        detail::unpack_call<R, sizeof...(Args)>(nullptr, flambda, args, rv);
    });
}

template<typename R, typename... Args>
TVM_ALWAYS_INLINE R TypedPackedFunc<R(Args...)>::operator()(Args... args) const {
    return detail::typed_packed_call_dispatcher<R>::run(packed_, std::forward<Args>(args)...);
}


// ObjectRef related conversion handling
// Object can have three possible type codes:
//      kTVMNDArrayHandle, kTVMModuleHandle, kTVMObjectHandle
//
// We use type traits to eliminate un-necessary checks.
template<typename T>
void TVMArgsSetter::SetObject(size_t i, T&& value) const {
    using ContainerType = typename std::remove_reference_t<T>::ContainerType;
    if (!value.defined()) {
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMNullptr);
        values_[i].v_handle = nullptr;
        return;
    }

    Object* ptr = value.data_.data_;
    if constexpr (std::is_base_of_v<NDArray::ContainerType, ContainerType> ||
                  std::is_base_of_v<ContainerType, NDArray::ContainerType>) {
        if (std::is_base_of_v<NDArray::ContainerType, ContainerType> ||
            ptr->IsInstance<NDArray::ContainerType>()) {
            values_[i].v_handle = NDArray::FFIGetHandle(value);
            type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle);
            return;
        }
    }

    if constexpr (std::is_base_of_v<Module::ContainerType, ContainerType> ||
                  std::is_base_of_v<ContainerType, Module::ContainerType>) {
        if (std::is_base_of_v<Module::ContainerType, ContainerType> ||
            ptr->IsInstance<Module::ContainerType>()) {
            values_[i].v_handle = ptr;
            type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMModuleHandle);
            return;
        }
    }

    if constexpr (std::is_base_of_v<PackedFunc::ContainerType, ContainerType> ||
                  std::is_base_of_v<ContainerType, PackedFunc::ContainerType>) {
        if (std::is_base_of_v<PackedFunc::ContainerType, ContainerType> ||
            ptr->IsInstance<PackedFunc::ContainerType>()) {
            values_[i].v_handle = ptr;
            type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle);
            return;
        }
    }

    // Like with BoxInt, unwrap any BoxBool instances.  See the BoxInt
    // explanation for more detail.
    if constexpr (std::is_base_of_v<Bool::ContainerType, ContainerType> ||
                  std::is_base_of_v<ContainerType, Bool::ContainerType>) {
        if (std::is_base_of_v<Bool::ContainerType, ContainerType> ||
            ptr->IsInstance<Bool::ContainerType>()) {
            values_[i].v_int64 = static_cast<Bool::ContainerType*>(ptr)->value;
            type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMArgBool);
            return;
        }
    }

    // If a boxed integer is being returned, always unbox it to the
    // primitive type.  This must be checked at the PackedFunc level to
    // ensure that a boxed primitive argument is round-tripped correctly
    // when the boxing is no longer required.
    //
    // For example, consider a PackedFunc with signature `ObjectRef
    // func(Array<ObjectRef>)`, and returns the first element of that
    // array.  When passing a Python array `[5, 17.5, "hello"]`, the
    // items are converted to `[Box<i64>(5), Box<double>(17.5),
    // String("hello")]` in order to provide an `Array<ObjectRef>`.
    //
    // If we had no additional conversions, the caller would receive the
    // return value as a `Box<i64>(5)`, which would be unexpected and
    // require additional unwrapping.  We could perform this check
    // inside the PackedFunc, but that would require a large amount of
    // duplicated checked, and would require explicit handling of
    // `TVMRetValue`.  Instead, this conversion is checked in the FFI
    // return value, to ensure that boxing/unboxing is applied
    // consistently.
    if constexpr (std::is_base_of_v<Int::ContainerType, ContainerType> ||
                  std::is_base_of_v<ContainerType, Int::ContainerType>) {
        if (std::is_base_of_v<Int::ContainerType, ContainerType> ||
            ptr->IsInstance<Int::ContainerType>()) {
            values_[i].v_int64 = static_cast<Int::ContainerType*>(ptr)->value;
            type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMArgInt);
            return;
        }
    }

    // Like with BoxInt, unwrap any BoxFloat instances.  See the BoxInt
    // explanation for more detail.
    if constexpr (std::is_base_of_v<Float::ContainerType, ContainerType> ||
                  std::is_base_of_v<ContainerType, Float::ContainerType>) {
        if (std::is_base_of_v<Float::ContainerType, ContainerType> ||
            ptr->IsInstance<Float::ContainerType>()) {
            values_[i].v_float64 = static_cast<Float::ContainerType*>(ptr)->value;
            type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMArgFloat);
            return;
        }
    }

    // Final fallback, if the ObjectRef has no special cases that must
    // be expressed within the TVMRetValue.
    if constexpr (std::is_rvalue_reference_v<decltype(value)>) {
        values_[i].v_handle = const_cast<Object**>(&(value.data_.data_));
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMObjectRValueRefArg);
    } else {
        values_[i].v_handle = value.data_.data_;
        type_codes_[i] = static_cast<int>(TVMArgTypeCode::kTVMObjectHandle);
    }
}

template<typename Derived>
template<typename TObjectRef, typename>
bool TVMPODValue_CRTP_<Derived>::IsObjectRef() const {
    using ContainerType = typename TObjectRef::ContainerType;
    // NOTE: the following code can be optimized by constant folding.
    if constexpr (std::is_base_of_v<NDArray::ContainerType, ContainerType>) {
        return type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle) &&
               TVMArrayHandleToObjectHandle(static_cast<TVMArrayHandle>(value_.v_handle))->IsInstance<ContainerType>();
    }

    if constexpr (std::is_base_of_v<Module::ContainerType, ContainerType>) {
        return type_code_ == static_cast<int>(TVMArgTypeCode::kTVMModuleHandle) &&
               static_cast<Object*>(value_.v_handle)->IsInstance<ContainerType>();
    }

    if constexpr (std::is_base_of_v<PackedFunc::ContainerType, ContainerType>) {
        return type_code_ == static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle) &&
               static_cast<Object*>(value_.v_handle)->IsInstance<ContainerType>();
    }

    // NOTE: we don't pass NDArray and runtime::Module as RValue ref.
    if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMObjectRValueRefArg)) {
        return ObjectTypeChecker<TObjectRef>::Check(*static_cast<Object**>(value_.v_handle));
    }

    return (std::is_base_of_v<ContainerType, NDArray::ContainerType> && type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle)) ||
           (std::is_base_of_v<ContainerType, Module::ContainerType> && type_code_ == static_cast<int>(TVMArgTypeCode::kTVMModuleHandle)) ||
           (std::is_base_of_v<ContainerType, PackedFunc::ContainerType> && type_code_ == static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle)) ||
           (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMObjectHandle) && ObjectTypeChecker<TObjectRef>::Check(static_cast<Object*>(value_.v_handle)));
}

template<typename Derived>
template<typename TObjectRef>
TObjectRef TVMPODValue_CRTP_<Derived>::AsObjectRef() const {
    static_assert(std::is_base_of_v<ObjectRef, TObjectRef>, "Conversion only works for ObjectRef");
    using ContainerType = typename TObjectRef::ContainerType;

    if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {
        CHECK(TObjectRef::_type_is_nullable) << "Expect a not null value of " << ContainerType::_type_key;
        return TObjectRef(ObjectPtr<Object>(nullptr));
    }

    // NOTE: The following code uses "if constexpr" wherever possible to
    // minimize the number of runtime checks.
    if constexpr (std::is_base_of_v<NDArray::ContainerType, ContainerType>) {
        // Casting to a subclass of NDArray
        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle));
        ObjectPtr<Object> data = NDArray::FFIDataFromHandle(static_cast<TVMArrayHandle>(value_.v_handle));
        CHECK(data->IsInstance<ContainerType>())
                << "Expected " << ContainerType::_type_key << " but got " << data->GetTypeKey();
        return TObjectRef(data);
    }

    if constexpr (std::is_base_of_v<Module::ContainerType, ContainerType>) {
        // Casting to a subclass of Module
        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMModuleHandle));
        ObjectPtr<Object> data = GetObjectPtr<Object>(static_cast<Object*>(value_.v_handle));
        CHECK(data->IsInstance<ContainerType>())
                << "Expected " << ContainerType::_type_key << " but got " << data->GetTypeKey();
        return TObjectRef(data);
    }

    if constexpr (std::is_base_of_v<PackedFunc::ContainerType, ContainerType>) {
        // Casting to a subclass of PackedFunc
        TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle));
        ObjectPtr<Object> data = GetObjectPtr<Object>(static_cast<Object*>(value_.v_handle));
        CHECK(data->IsInstance<ContainerType>())
                << "Expected " << ContainerType::_type_key << " but got " << data->GetTypeKey();
        return TObjectRef(data);
    }

    if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMObjectHandle)) {
        // normal object type check.
        auto* ptr = static_cast<Object*>(value_.v_handle);
        Optional<String> checked_type = ObjectTypeChecker<TObjectRef>::CheckAndGetMismatch(ptr);
        CHECK(!checked_type.defined()) << "Expected " << ObjectTypeChecker<TObjectRef>::TypeName()
                                       << ", but got " << checked_type.value();
        return TObjectRef(GetObjectPtr<Object>(ptr));
    }

    if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMObjectRValueRefArg)) {
        Object* ptr = *static_cast<Object**>(value_.v_handle);
        Optional<String> checked_type = ObjectTypeChecker<TObjectRef>::CheckAndGetMismatch(ptr);
        CHECK(!checked_type.defined()) << "Expected " << ObjectTypeChecker<TObjectRef>::TypeName()
                                       << ", but got " << checked_type.value();
        return TObjectRef(GetObjectPtr<Object>(ptr));
    }

    if constexpr (std::is_base_of_v<ContainerType, NDArray::ContainerType>) {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNDArrayHandle)) {
            // Casting to a base class that NDArray can sub-class
            ObjectPtr<Object> data = NDArray::FFIDataFromHandle(static_cast<TVMArrayHandle>(value_.v_handle));
            return TObjectRef(data);
        }
    }

    if constexpr (std::is_base_of_v<ContainerType, Module::ContainerType>) {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMModuleHandle)) {
            // Casting to a base class that Module can sub-class
            return TObjectRef(GetObjectPtr<Object>(static_cast<Object*>(value_.v_handle)));
        }
    }

    if constexpr (std::is_base_of_v<ContainerType, PackedFunc::ContainerType>) {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMPackedFuncHandle)) {
            // Casting to a base class that PackedFunc can sub-class
            return TObjectRef(GetObjectPtr<Object>(static_cast<Object*>(value_.v_handle)));
        }
    }

    if constexpr (std::is_base_of_v<TObjectRef, Int>) {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMArgInt)) {
            return Int(value_.v_int64);
        }
    }

    if constexpr (std::is_base_of_v<TObjectRef, Float>) {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMArgFloat)) {
            return Float(value_.v_float64);
        }
    }

    if constexpr (std::is_base_of_v<TObjectRef, Bool>) {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMArgBool)) {
            return Bool(value_.v_int64);
        }
    }

    if constexpr (std::is_base_of_v<TObjectRef, String>) {
        if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMStr) ||
            type_code_ == static_cast<int>(TVMArgTypeCode::kTVMBytes)) {
            // This step is the reason why `AsObjectRef` cannot be provided
            // in the base `TVMPODValue_` class.  Because `TVMArgValue` and
            // `TVMRetValue` have different implementations of `operator
            // std::string`, with different interpretations of `kTVMStr` and
            // `kTVMBytes`, we must delegate to those implementations.
            //
            // This could be done with a pure virtual method in
            // `TVMPODValue_`, but that would require a vtable lookup during
            // FFI conversions, imposing a runtime overhead.
            return String(static_cast<const Derived*>(this)->operator std::string());
        }
    }

    TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMObjectHandle));
    return TObjectRef(ObjectPtr<Object>(nullptr));
}

// specializations of PackedFuncValueConverter
template<>
struct PackedFuncValueConverter<String> {
    template<typename PODSubclass>
    static String From(const PODSubclass& val) {
        if (val.template IsObjectRef<String>()) {
            return val.template AsObjectRef<String>();
        }

        return String(val.operator std::string());
    }
};

template<typename T>
struct PackedFuncValueConverter<Array<T>> {
    static Array<T> From(const TVMArgValue& val) {
        auto untyped_array = val.AsObjectRef<Array<ObjectRef>>();

        if constexpr (std::is_same_v<T, ObjectRef>) {
            return untyped_array;
        }

        // Attempt to convert each item of the array into the desired
        // type.  If the items do not require a conversion, no copies are
        // made.
        return untyped_array.Map([](ObjectRef item) {
            // Recursively apply any conversions that have been registered
            // with TVM's FFI.
            //
            // For example, a function that accepts `Array<PrimExpr>` may
            // be called from python with argument `[1,2]`.  By the time
            // `PackedFuncValueConverter::From` is called, the python list
            // has been converted to `Array<ObjectRef>`, with contents
            // converted into `runtime::Int`.  Converting the `ObjectRef`
            // to `TVMArgValue` unboxes the `runtime::Int` back into a
            // primitive with type code `kTVMArgInt`.  This primitive can
            // then be converted to a PrimExpr using
            // `PackedFuncValueConverter<PrimExpr>::From`.
            //
            // The use of two conversions, first from python `int` to
            // `runtime::Int` and then from `runtime::Int` to `PrimExpr`,
            // is a result of the split between `libtvm_runtime.so` and
            // `libtvm.so`.  The FFI must function correctly in both
            // cases, and so conversions applied by default in the Python
            // FFI implementation may only produce types that are
            // available in both libraries.  In the C++ FFI implementation
            // (i.e. this file), libtvm.so may apply additional
            // conversions that are not present in libtvm_runtime.so.
            TVMValue value;
            int type_code;
            TVMArgsSetter setter(&value, &type_code);
            setter(0, item);
            TVMArgValue arg(value, type_code);
            return PackedFuncValueConverter<T>::From(arg);
        });
    }

    static Array<T> From(const TVMRetValue& val) {
        auto untyped_array = val.AsObjectRef<Array<ObjectRef>>();

        if constexpr (std::is_same_v<T, ObjectRef>) {
            return untyped_array;
        }

        return untyped_array.Map([](ObjectRef item) {
            TVMRetValue item_val;
            item_val = std::move(item);
            return PackedFuncValueConverter<T>::From(item_val);
        });
    }
};

template<typename T, typename U>
struct PackedFuncValueConverter<Map<T, U>> {
    static Map<T, U> From(const TVMArgValue& val) {
        auto untyped_map = val.AsObjectRef<Map<ObjectRef, ObjectRef>>();

        if constexpr (std::is_same_v<T, ObjectRef> && std::is_same_v<U, ObjectRef>) {
            return Downcast<Map<T, U>>(untyped_map);
        }

        if (ObjectTypeChecker<Map<T, U>>::Check(untyped_map.get())) {
            // Early bail-out for common case where no type conversions are
            // required.
            return Downcast<Map<T, U>>(untyped_map);
        }

        Map<T, U> output;
        for (const auto& kv: untyped_map) {
            T new_key = [&]() {
                if constexpr (std::is_same_v<T, ObjectRef>) {
                    return kv.first;
                } else {
                    TVMValue pod_value;
                    int type_code;
                    TVMArgsSetter setter(&pod_value, &type_code);
                    setter(0, kv.first);
                    TVMArgValue pod_arg(pod_value, type_code);
                    return PackedFuncValueConverter<T>::From(pod_arg);
                }
            }();
            U new_value = [&]() {
                if constexpr (std::is_same_v<U, ObjectRef>) {
                    return kv.second;
                } else {
                    TVMValue pod_value;
                    int type_code;
                    TVMArgsSetter setter(&pod_value, &type_code);
                    setter(0, kv.second);
                    TVMArgValue key_arg(pod_value, type_code);
                    return PackedFuncValueConverter<U>::From(key_arg);
                }
            }();
            output.Set(new_key, new_value);
        }
        return output;
    }
    static Map<T, U> From(const TVMRetValue& val) {
        auto untyped_map = val.AsObjectRef<Map<ObjectRef, ObjectRef>>();

        if constexpr (std::is_same_v<T, ObjectRef> && std::is_same_v<U, ObjectRef>) {
            return Downcast<Map<T, U>>(untyped_map);
        }

        if (ObjectTypeChecker<Map<T, U>>::Check(untyped_map.get())) {
            // Early bail-out for common case where no type conversions are
            // required.
            return Downcast<Map<T, U>>(untyped_map);
        }

        Map<T, U> output;
        for (const auto& kv: untyped_map) {
            T new_key = [&]() {
                if constexpr (std::is_same_v<T, ObjectRef>) {
                    return kv.first;
                } else {
                    TVMRetValue pod;
                    pod = kv.first;
                    return PackedFuncValueConverter<T>::From(pod);
                }
            }();
            U new_value = [&]() {
                if constexpr (std::is_same_v<U, ObjectRef>) {
                    return kv.second;
                } else {
                    TVMRetValue pod;
                    pod = kv.second;
                    return PackedFuncValueConverter<U>::From(pod);
                }
            }();
            output.Set(new_key, new_value);
        }
        return output;
    }
};

template<typename T>
struct PackedFuncValueConverter<Optional<T>> {
    static Optional<T> From(const TVMArgValue& val) {
        if (val.type_code() == static_cast<int>(TVMArgTypeCode::kTVMNullptr))
            return Optional<T>(nullptr);
        return PackedFuncValueConverter<T>::From(val);
    }
    static Optional<T> From(const TVMRetValue& val) {
        if (val.type_code() == static_cast<int>(TVMArgTypeCode::kTVMNullptr)) return Optional<T>(nullptr);
        return PackedFuncValueConverter<T>::From(val);
    }
};

template<typename... VariantTypes>
struct PackedFuncValueConverter<Variant<VariantTypes...>> {
    using VType = Variant<VariantTypes...>;

    // Can't just take `const TVMPODValue&` as an argument, because
    // `TVMArgValue` and `TVMRetValue` have different implementations
    // for `operator std::string()`.
    template<typename PODSubclass>
    static VType From(const PODSubclass& val) {
        if (auto opt = TryAsObjectRef<VariantTypes...>(val)) {
            return opt.value();
        }

        if (auto opt = TryValueConverter<VariantTypes...>(val)) {
            return opt.value();
        }

        LOG(FATAL) << "Expected one of "
                   << static_cast<const std::stringstream&>(
                              (std::stringstream() << ... << VariantTypes::ContainerType::_type_key))
                              .str()
                   << " but got " << ArgTypeCode2Str(val.type_code());
    }

    template<typename VarFirst, typename... VarRest, typename PODSubclass>
    static Optional<VType> TryAsObjectRef(const PODSubclass& val) {
        if (val.template IsObjectRef<VarFirst>()) {
            return VType(val.template AsObjectRef<VarFirst>());
        }
        if constexpr (sizeof...(VarRest)) {
            return TryAsObjectRef<VarRest...>(val);
        } else {
            return NullOpt;
        }
    }

    template<typename VarFirst, typename... VarRest, typename PODSubclass>
    static Optional<VType> TryValueConverter(const PODSubclass& val) {
        try {
            return VType(PackedFuncValueConverter<VarFirst>::From(val));
        } catch (const Error&) {
        }

        if constexpr (sizeof...(VarRest)) {
            return TryValueConverter<VarRest...>(val);
        } else {
            return NullOpt;
        }
    }
};

inline bool String::CanConvertFrom(const TVMArgValue& val) {
    return val.type_code() == static_cast<int>(TVMArgTypeCode::kTVMStr) ||
           val.IsObjectRef<String>();
}

template<typename T, typename>
TVMArgValue::operator T() const {
    return PackedFuncValueConverter<T>::From(*this);
}

inline TVMArgValue::operator DLDataType() const {
    if (String::CanConvertFrom(*this)) {
        return String2DLDataType(PackedFuncValueConverter<String>::From(*this).operator std::string());
    }

    // None type
    if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMNullptr)) {
        DLDataType t;
        t.code = static_cast<uint8_t>(TVMArgTypeCode::kTVMOpaqueHandle);
        t.bits = 0;
        t.lanes = 0;
        return t;
    }
    TVM_CHECK_TYPE_CODE(type_code_, static_cast<int>(TVMArgTypeCode::kTVMDataType));
    return value_.v_type;
}

inline TVMArgValue::operator DataType() const {
    return DataType(operator DLDataType());
}

template<typename T, typename>
TVMMovableArgValue_::operator T() const {
    if (type_code_ == static_cast<int>(TVMArgTypeCode::kTVMObjectRValueRefArg)) {
        auto** ref = static_cast<Object**>(value_.v_handle);
        if (ObjectTypeChecker<T>::Check(*ref)) {
            return T(ObjectPtr<Object>::MoveFromRValueRefArg(ref));
        }
    }
    // fallback
    return PackedFuncValueConverter<T>::From(AsArgValue());
}

template<typename TObjectRef, typename>
TVMRetValue& TVMRetValue::operator=(TObjectRef other) {
    using ContainerType = typename TObjectRef::ContainerType;
    const Object* ptr = other.get();

    if (ptr) {
        // Check for special cases of ObjectRef that have explicit
        // representation within the TVMRetValue structure.
        // (e.g. Unboxing of `runtime::Int` into a primitive integer
        // with type code kTVMArgInt.)  The checks below are written to
        // handle three distinct cases.
        //
        // 1. If TObjectRef is a subclass of TSpecialCase, the special
        //    case applies, and can be handled without a runtime check.
        //    No runtime checks should be performed.
        //
        // 2. If TSpecialCase is a subclass of TObjectRef, the special
        //    case might apply, and requires a runtime check.
        //
        // 3. If neither TObjectRef nor TSpecialCase is a subclass of
        //    the other, then the special case does not apply.  No
        //    runtime checks should be performed.
        //
        // Use of `if constexpr` ensures that the C++ subclass checks
        // are applied when compiling TVM, and runtime overhead are only
        // present when they may be applicable.

        if constexpr (std::is_base_of_v<ContainerType, NDArray::ContainerType> ||
                      std::is_base_of_v<NDArray::ContainerType, ContainerType>) {
            if (std::is_base_of_v<NDArray::ContainerType, ContainerType> ||
                ptr->IsInstance<NDArray::ContainerType>()) {
                return operator=(NDArray(std::move(other.data_)));
            }
        }

        if constexpr (std::is_base_of_v<ContainerType, Module::ContainerType> ||
                      std::is_base_of_v<Module::ContainerType, ContainerType>) {
            if (std::is_base_of_v<Module::ContainerType, ContainerType> ||
                ptr->IsInstance<Module::ContainerType>()) {
                return operator=(Module(std::move(other.data_)));
            }
        }

        if constexpr (std::is_base_of_v<ContainerType, PackedFunc::ContainerType> ||
                      std::is_base_of_v<PackedFunc::ContainerType, ContainerType>) {
            if (std::is_base_of_v<PackedFunc::ContainerType, ContainerType> ||
                ptr->IsInstance<PackedFunc::ContainerType>()) {
                return operator=(PackedFunc(std::move(other.data_)));
            }
        }

        if constexpr (std::is_base_of_v<Bool, TObjectRef> || std::is_base_of_v<TObjectRef, Bool>) {
            if (std::is_base_of_v<Bool, TObjectRef> || ptr->IsInstance<Bool::ContainerType>()) {
                bool value = static_cast<const Bool::ContainerType*>(ptr)->value;
                return operator=(value);
            }
        }

        if constexpr (std::is_base_of_v<Int, TObjectRef> || std::is_base_of_v<TObjectRef, Int>) {
            if (std::is_base_of_v<Int, TObjectRef> || ptr->IsInstance<Int::ContainerType>()) {
                int64_t value = static_cast<const Int::ContainerType*>(ptr)->value;
                return operator=(value);
            }
        }

        if constexpr (std::is_base_of_v<Float, TObjectRef> || std::is_base_of_v<TObjectRef, Float>) {
            if (std::is_base_of_v<Float, TObjectRef> || ptr->IsInstance<Float::ContainerType>()) {
                double value = static_cast<const Float::ContainerType*>(ptr)->value;
                return operator=(value);
            }
        }

        // If the object being stored is not one of the special cases,
        // it is stored as an ObjectRef.
        SwitchToObject(static_cast<int>(TVMArgTypeCode::kTVMObjectHandle), std::move(other.data_));

    } else {
        // No object is present, set to an explicitly null handle.  When
        // returning to a Python callee, this will be converted to
        // `None`.
        SwitchToPOD(static_cast<int>(TVMArgTypeCode::kTVMNullptr));
        value_.v_handle = nullptr;
    }

    return *this;
}

template<typename T, typename>
TVMRetValue::operator T() const {
    return PackedFuncValueConverter<T>::From(*this);
}

inline PackedFunc Module::GetFunction(const String& name, bool query_imports) {
    return (*this)->GetFunction(name, query_imports);
}

}// namespace litetvm::runtime

#endif//PACKED_FUNC_H
