/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/VariantType.h>
#include <type_traits>

class xiiRTTI;
class xiiReflectedClass;
class xiiVariant;

/// Flags that describe a reflected type.
struct xiiTypeFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    StandardType = XII_BIT(0), ///< Anything that can be stored inside a xiiVariant except for pointers and containers.
    IsEnum       = XII_BIT(1), ///< enum struct used for xiiEnum.
    Bitflags     = XII_BIT(2), ///< bitflags struct used for xiiBitflags.
    Class        = XII_BIT(3), ///< A class or struct. The above flags are mutually exclusive.

    Abstract = XII_BIT(4), ///< Type is abstract.
    Phantom  = XII_BIT(5), ///< De-serialized type information that cannot be created on this process.
    Minimal  = XII_BIT(6), ///< Does not contain any property, function or attribute information. Used only for versioning.
    Default  = 0
  };

  struct Bits
  {
    StorageType StandardType : 1;
    StorageType IsEnum : 1;
    StorageType Bitflags : 1;
    StorageType Class : 1;
    StorageType Abstract : 1;
    StorageType Phantom : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiTypeFlags)


// ****************************************************
// ***** Templates for accessing static RTTI data *****

namespace xiiInternal
{
  /// [internal] Helper struct for accessing static RTTI data.
  template <typename T>
  struct xiiStaticRTTI
  {
  };

  // Special implementation for types that have no base
  template <>
  struct xiiStaticRTTI<xiiNoBase>
  {
    static const xiiRTTI* GetRTTI() { return nullptr; }
  };

  // Special implementation for void to make function reflection compile void return values without further specialization.
  template <>
  struct xiiStaticRTTI<void>
  {
    static const xiiRTTI* GetRTTI() { return nullptr; }
  };

  template <typename T>
  XII_ALWAYS_INLINE const xiiRTTI* GetStaticRTTI(xiiTraitInt<1>) // class derived from xiiReflectedClass
  {
    return T::GetStaticRTTI();
  }

  template <typename T>
  XII_ALWAYS_INLINE const xiiRTTI* GetStaticRTTI(xiiTraitInt<0>) // static rtti
  {
    // Since this is pure C++ and no preprocessor macro, calling it with types such as 'int' and 'xiiInt32' will
    // actually return the same RTTI object, which would not be possible with a purely macro based solution

    return xiiStaticRTTI<T>::GetRTTI();
  }

  template <typename Type>
  xiiBitflags<xiiTypeFlags> DetermineTypeFlags()
  {
    xiiBitflags<xiiTypeFlags> flags;
    xiiVariantType::Enum      type =
      static_cast<xiiVariantType::Enum>(xiiVariantTypeDeduction<typename xiiTypeTraits<Type>::NonConstReferenceType>::value);
    if ((type >= xiiVariantType::FirstStandardType && type <= xiiVariantType::LastStandardType) || XII_IS_SAME_TYPE(xiiVariant, Type))
      flags.Add(xiiTypeFlags::StandardType);
    else
      flags.Add(xiiTypeFlags::Class);

    if (std::is_abstract<Type>::value)
      flags.Add(xiiTypeFlags::Abstract);

    return flags;
  }

  template <>
  XII_ALWAYS_INLINE xiiBitflags<xiiTypeFlags> DetermineTypeFlags<xiiVariant>()
  {
    return xiiTypeFlags::StandardType;
  }

  template <typename T>
  struct xiiStaticRTTIWrapper
  {
    static_assert(sizeof(T) == 0, "Type has not been declared as reflectable (use XII_DECLARE_REFLECTABLE_TYPE macro)");
  };
} // namespace xiiInternal

/// Use this function, specialized with the type that you are interested in, to get the static RTTI data for some type.
template <typename T>
XII_ALWAYS_INLINE const xiiRTTI* xiiGetStaticRTTI()
{
  return xiiInternal::GetStaticRTTI<T>(xiiTraitInt<XII_IS_DERIVED_FROM_STATIC(xiiReflectedClass, T)>());
}

// **************************************************
// ***** Macros for declaring types reflectable *****

#define XII_NO_LINKAGE

/// Declares a type to be statically reflectable. Insert this into the header of a type to enable reflection on it.
/// This is not needed if the type is already dynamically reflectable.
#define XII_DECLARE_REFLECTABLE_TYPE(Linkage, TYPE)                   \
  namespace xiiInternal                                               \
  {                                                                   \
    template <>                                                       \
    struct Linkage xiiStaticRTTIWrapper<TYPE>                         \
    {                                                                 \
      static xiiRTTI s_RTTI;                                          \
    };                                                                \
                                                                      \
    /* This specialization calls the function to get the RTTI data */ \
    /* This code might get duplicated in different DLLs, but all   */ \
    /* will call the same function, so the RTTI object is unique   */ \
    template <>                                                       \
    struct xiiStaticRTTI<TYPE>                                        \
    {                                                                 \
      XII_ALWAYS_INLINE static const xiiRTTI* GetRTTI()               \
      {                                                               \
        return &xiiStaticRTTIWrapper<TYPE>::s_RTTI;                   \
      }                                                               \
    };                                                                \
  }

/// Insert this into a class/struct to enable properties that are private members.
/// All types that have dynamic reflection (\see XII_ADD_DYNAMIC_REFLECTION) already have this ability.
#define XII_ALLOW_PRIVATE_PROPERTIES(SELF) friend xiiRTTI GetRTTI(SELF*)

/// \cond
// Internal helper macro.
#define XII_RTTIINFO_DECL(Type, BaseType, Version) \
                                                   \
  xiiStringView GetTypeName(Type*)                 \
  {                                                \
    return #Type;                                  \
  }                                                \
  xiiUInt32 GetTypeVersion(Type*)                  \
  {                                                \
    return Version;                                \
  }                                                \
                                                   \
  xiiRTTI GetRTTI(Type*);

// internal helper macro
#define XII_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, BaseType, AllocatorType)                                      \
  xiiRTTI GetRTTI(Type*)                                                                                    \
  {                                                                                                         \
    using OwnType     = Type;                                                                               \
    using OwnBaseType = BaseType;                                                                           \
    static AllocatorType                                   Allocator;                                       \
    static xiiBitflags<xiiTypeFlags>                       flags = xiiInternal::DetermineTypeFlags<Type>(); \
    static xiiArrayPtr<const xiiAbstractProperty*>         Properties;                                      \
    static xiiArrayPtr<const xiiAbstractFunctionProperty*> Functions;                                       \
    static xiiArrayPtr<const xiiPropertyAttribute*>        Attributes;                                      \
    static xiiArrayPtr<xiiAbstractMessageHandler*>         MessageHandlers;                                 \
    static xiiArrayPtr<xiiMessageSenderInfo>               MessageSenders;

/// \endcond

/// Implements the necessary functionality for a type to be statically reflectable.
///
/// \param Type
///   The type for which the reflection functionality should be implemented.
/// \param BaseType
///   The base class type of \a Type. If it has no base class, pass xiiNoBase
/// \param Version
///   The version of \a Type. Must be increased when the class serialization changes.
/// \param AllocatorType
///   The type of a xiiRTTIAllocator that can be used to create and destroy instances
///   of \a Type. Pass xiiRTTINoAllocator for types that should not be created dynamically.
///   Pass xiiRTTIDefaultAllocator<Type> for types that should be created on the default heap.
///   Pass a custom xiiRTTIAllocator type to handle allocation differently.
#define XII_BEGIN_STATIC_REFLECTED_TYPE(Type, BaseType, Version, AllocatorType) \
  XII_RTTIINFO_DECL(Type, BaseType, Version)                                    \
  xiiRTTI xiiInternal::xiiStaticRTTIWrapper<Type>::s_RTTI = GetRTTI((Type*)0);  \
  XII_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, BaseType, AllocatorType)


/// Ends the reflection code block that was opened with XII_BEGIN_STATIC_REFLECTED_TYPE.
#define XII_END_STATIC_REFLECTED_TYPE                                                                                                                      \
  ;                                                                                                                                                        \
  return xiiRTTI(GetTypeName((OwnType*)0), xiiGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion((OwnType*)0),                                  \
                 xiiVariantTypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, nullptr); \
  }


/// Within a XII_BEGIN_REFLECTED_TYPE / XII_END_REFLECTED_TYPE block, use this to start the block that declares all the properties.
#define XII_BEGIN_PROPERTIES static const xiiAbstractProperty* PropertyList[] =



/// Ends the block to declare properties that was started with XII_BEGIN_PROPERTIES.
#define XII_END_PROPERTIES \
  ;                        \
  Properties = PropertyList

/// Within a XII_BEGIN_REFLECTED_TYPE / XII_END_REFLECTED_TYPE block, use this to start the block that declares all the functions.
#define XII_BEGIN_FUNCTIONS static const xiiAbstractFunctionProperty* FunctionList[] =



/// Ends the block to declare functions that was started with XII_BEGIN_FUNCTIONS.
#define XII_END_FUNCTIONS \
  ;                       \
  Functions = FunctionList

/// Within a XII_BEGIN_REFLECTED_TYPE / XII_END_REFLECTED_TYPE block, use this to start the block that declares all the attributes.
#define XII_BEGIN_ATTRIBUTES static const xiiPropertyAttribute* AttributeList[] =



/// Ends the block to declare attributes that was started with XII_BEGIN_ATTRIBUTES.
#define XII_END_ATTRIBUTES \
  ;                        \
  Attributes = AttributeList

/// Within a XII_BEGIN_FUNCTIONS / XII_END_FUNCTIONS; block, this adds a member or static function property stored inside the RTTI
/// data.
///
/// \param Function
///   The function to be executed, must match the C++ function name.
#define XII_FUNCTION_PROPERTY(Function) (new xiiFunctionProperty<decltype(&OwnType::Function)>(XII_PP_STRINGIFY(Function), &OwnType::Function))

/// Within a XII_BEGIN_FUNCTIONS / XII_END_FUNCTIONS; block, this adds a member or static function property stored inside the RTTI
/// data. Use this version if you need to change the name of the function or need to cast the function to one of its overload versions.
///
/// \param PropertyName
///   The name under which the property should be registered.
///
/// \param Function
///   The function to be executed, must match the C++ function name including the class name e.g. 'CLASS::NAME'.
#define XII_FUNCTION_PROPERTY_EX(PropertyName, Function) (new xiiFunctionProperty<decltype(&Function)>(PropertyName, &Function))

/// \internal Used by XII_SCRIPT_FUNCTION_PROPERTY
#define _XII_SCRIPT_FUNCTION_PARAM(type, name) xiiScriptableFunctionAttribute::ArgType::type, name

/// Convenience macro to declare a function that can be called from scripts.
///
/// \param Function
///   The function to be executed, must match the C++ function name including the class name e.g. 'CLASS::NAME'.
///
/// Internally this calls XII_FUNCTION_PROPERTY and adds a xiiScriptableFunctionAttribute.
/// Use the variadic arguments in pairs to configure how each function parameter gets exposed.
///   Use 'In', 'Out' or 'Inout' to specify whether a function parameter is only read, or also written back to.
///   Follow it with a string to specify the name under which the parameter should show up.
///
/// Example:
///   XII_SCRIPT_FUNCTION_PROPERTY(MyFunc1NoParams)
///   XII_SCRIPT_FUNCTION_PROPERTY(MyFunc2FloatInDoubleOut, In, "FloatValue", Out, "DoubleResult")
#define XII_SCRIPT_FUNCTION_PROPERTY(Function, ...) \
  XII_FUNCTION_PROPERTY(Function)->AddAttributes(new xiiScriptableFunctionAttribute(XII_EXPAND_ARGS_PAIR_COMMA(_XII_SCRIPT_FUNCTION_PARAM, ##__VA_ARGS__)))

/// Within a XII_BEGIN_FUNCTIONS / XII_END_FUNCTIONS; block, this adds a constructor function property stored inside the RTTI data.
///
/// \param Function
///   The function to be executed in the form of CLASS::FUNCTION_NAME.
#define XII_CONSTRUCTOR_PROPERTY(...) (new xiiConstructorFunctionProperty<OwnType, ##__VA_ARGS__>())


// [internal] Helper macro to get the return type of a getter function.
#define XII_GETTER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc())

/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a property that uses custom getter / setter functions.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param Getter
///   The getter function for this property.
/// \param Setter
///   The setter function for this property.
///
/// \note There does not actually need to be a variable for this type of properties, as all accesses go through functions.
/// Thus you can for example expose a 'vector' property that is actually stored as a column of a matrix.
#define XII_ACCESSOR_PROPERTY(PropertyName, Getter, Setter) \
  (new xiiAccessorProperty<OwnType, XII_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// Same as XII_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define XII_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, Getter) \
  (new xiiAccessorProperty<OwnType, XII_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))

// [internal] Helper macro to get the return type of a array getter function.
#define XII_ARRAY_GETTER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc(0))

/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a property that uses custom functions to access an array.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetCount
///   Function signature: xiiUInt32 GetCount() const;
/// \param Getter
///   Function signature: Type GetValue(xiiUInt32 uiIndex) const;
/// \param Setter
///   Function signature: void SetValue(xiiUInt32 uiIndex, Type value);
/// \param Insert
///   Function signature: void Insert(xiiUInt32 uiIndex, Type value);
/// \param Remove
///   Function signature: void Remove(xiiUInt32 uiIndex);
#define XII_ARRAY_ACCESSOR_PROPERTY(PropertyName, GetCount, Getter, Setter, Insert, Remove) \
  (new xiiAccessorArrayProperty<OwnType, XII_ARRAY_GETTER_TYPE(OwnType, OwnType::Getter)>(  \
    PropertyName, &OwnType::GetCount, &OwnType::Getter, &OwnType::Setter, &OwnType::Insert, &OwnType::Remove))

/// Same as XII_ARRAY_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetCount, Getter)              \
  (new xiiAccessorArrayProperty<OwnType, XII_ARRAY_GETTER_TYPE(OwnType, OwnType::Getter)>( \
    PropertyName, &OwnType::GetCount, &OwnType::Getter, nullptr, nullptr, nullptr))

#define XII_SET_CONTAINER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc())

#define XII_SET_CONTAINER_SUB_TYPE(Class, GetterFunc) \
  xiiContainerSubTypeResolver<xiiTypeTraits<decltype(std::declval<Class>().GetterFunc())>::NonConstReferenceType>::Type

/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a property that uses custom functions to access a set.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetValues
///   Function signature: Container<Type> GetValues() const;
/// \param Insert
///   Function signature: void Insert(Type value);
/// \param Remove
///   Function signature: void Remove(Type value);
///
/// \note Container<Type> can be any container that can be iterated via range based for loops.
#define XII_SET_ACCESSOR_PROPERTY(PropertyName, GetValues, Insert, Remove)                                             \
  (new xiiAccessorSetProperty<OwnType, xiiFunctionParameterTypeResolver<0, decltype(&OwnType::Insert)>::ParameterType, \
                              XII_SET_CONTAINER_TYPE(OwnType, GetValues)>(PropertyName, &OwnType::GetValues, &OwnType::Insert, &OwnType::Remove))

/// Same as XII_SET_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define XII_SET_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetValues)                                                                \
  (new xiiAccessorSetProperty<OwnType, XII_SET_CONTAINER_SUB_TYPE(OwnType, GetValues), XII_SET_CONTAINER_TYPE(OwnType, GetValues)>( \
    PropertyName, &OwnType::GetValues, nullptr, nullptr))

/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a property that uses custom functions to for write access to a
/// map.
///   Use this if you have a xiiHashTable or xiiMap to expose directly and just want to be informed of write operations.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetContainer
///   Function signature: const Container<Key, Type>& GetValues() const;
/// \param Insert
///   Function signature: void Insert(xiiStringView sKey, Type value);
/// \param Remove
///   Function signature: void Remove(xiiStringView sKey);
///
/// \note Container can be xiiMap or xiiHashTable
#define XII_MAP_WRITE_ACCESSOR_PROPERTY(PropertyName, GetContainer, Insert, Remove)                                         \
  (new xiiWriteAccessorMapProperty<OwnType, xiiFunctionParameterTypeResolver<1, decltype(&OwnType::Insert)>::ParameterType, \
                                   XII_SET_CONTAINER_TYPE(OwnType, GetContainer)>(PropertyName, &OwnType::GetContainer, &OwnType::Insert, &OwnType::Remove))

/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a property that uses custom functions to access a map.
///   Use this if you you want to hide the implementation details of the map from the user.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetKeyRange
///   Function signature: const Range GetValues() const;
///   Range has to be an object that a ranged based for-loop can iterate over containing the keys
///   implicitly convertible to Type / xiiString.
/// \param GetValue
///   Function signature: bool GetValue(xiiStringView sKey, Type& value) const;
///   Returns whether the the key existed. value must be a non const ref as it is written to.
/// \param Insert
///   Function signature: void Insert(xiiStringView sKey, Type value);
///   value can also be const and/or a reference.
/// \param Remove
///   Function signature: void Remove(xiiStringView sKey);
///
/// \note Container can be xiiMap or xiiHashTable
#define XII_MAP_ACCESSOR_PROPERTY(PropertyName, GetKeyRange, GetValue, Insert, Remove)                                 \
  (new xiiAccessorMapProperty<OwnType, xiiFunctionParameterTypeResolver<1, decltype(&OwnType::Insert)>::ParameterType, \
                              XII_SET_CONTAINER_TYPE(OwnType, GetKeyRange)>(PropertyName, &OwnType::GetKeyRange, &OwnType::GetValue, &OwnType::Insert, &OwnType::Remove))

/// Same as XII_MAP_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define XII_MAP_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetKeyRange, GetValue)                                                                      \
  (new xiiAccessorMapProperty<OwnType,                                                                                                                \
                              xiiTypeTraits<xiiFunctionParameterTypeResolver<1, decltype(&OwnType::GetValue)>::ParameterType>::NonConstReferenceType, \
                              XII_SET_CONTAINER_TYPE(OwnType, GetKeyRange)>(PropertyName, &OwnType::GetKeyRange, &OwnType::GetValue, nullptr, nullptr))



/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a property that uses custom getter / setter functions.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param EnumType
///   The name of the enum struct used by xiiEnum.
/// \param Getter
///   The getter function for this property.
/// \param Setter
///   The setter function for this property.
#define XII_ENUM_ACCESSOR_PROPERTY(PropertyName, EnumType, Getter, Setter) \
  (new xiiEnumAccessorProperty<OwnType, EnumType, XII_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// Same as XII_ENUM_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define XII_ENUM_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, EnumType, Getter) \
  (new xiiEnumAccessorProperty<OwnType, EnumType, XII_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))

/// Same as XII_ENUM_ACCESSOR_PROPERTY, but for bitfields.
#define XII_BITFLAGS_ACCESSOR_PROPERTY(PropertyName, BitflagsType, Getter, Setter) \
  (new xiiBitflagsAccessorProperty<OwnType, BitflagsType, XII_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// Same as XII_BITFLAGS_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define XII_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, BitflagsType, Getter) \
  (new xiiBitflagsAccessorProperty<OwnType, BitflagsType, XII_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))


// [internal] Helper macro to get the type of a class member.
#define XII_MEMBER_TYPE(Class, Member) decltype(std::declval<Class>().Member)

#define XII_MEMBER_CONTAINER_SUB_TYPE(Class, Member) \
  xiiContainerSubTypeResolver<xiiTypeTraits<decltype(std::declval<Class>().Member)>::NonConstReferenceType>::Type

/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a XII_ENUM_ACCESSOR_PROPERTY instead.
#define XII_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                                                        \
  (new xiiMemberProperty<OwnType, XII_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                                                                                        \
                                                                        &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
                                                                        &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
                                                                        &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as XII_MEMBER_PROPERTY, but the property is read-only.
#define XII_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                                                                                       \
  (new xiiMemberProperty<OwnType, XII_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                                                                                                 \
                                                                        &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
                                                                        &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as XII_MEMBER_PROPERTY, but the property is an array (xiiHybridArray, xiiDynamicArray or xiiDeque).
#define XII_ARRAY_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                                                                                                                         \
  (new xiiMemberArrayProperty<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), XII_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName,                                                                                                      \
                                                                                                                                 &xiiArrayPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
                                                                                                                                 &xiiArrayPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// Same as XII_MEMBER_PROPERTY, but the property is a read-only array (xiiArrayPtr, xiiHybridArray, xiiDynamicArray or xiiDeque).
#define XII_ARRAY_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                                     \
  (new xiiMemberArrayReadOnlyProperty<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), XII_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &xiiArrayPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer))

/// Same as XII_MEMBER_PROPERTY, but the property is a set (xiiSet, xiiHashSet).
#define XII_SET_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                                                                                                                       \
  (new xiiMemberSetProperty<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), XII_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName,                                                                                                    \
                                                                                                                               &xiiSetPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
                                                                                                                               &xiiSetPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// Same as XII_MEMBER_PROPERTY, but the property is a read-only set (xiiSet, xiiHashSet).
#define XII_SET_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                             \
  (new xiiMemberSetProperty<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), XII_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &xiiSetPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, nullptr))

/// Same as XII_MEMBER_PROPERTY, but the property is a map (xiiMap, xiiHashTable).
#define XII_MAP_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                                                                                                                       \
  (new xiiMemberMapProperty<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), XII_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName,                                                                                                    \
                                                                                                                               &xiiMapPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
                                                                                                                               &xiiMapPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// Same as XII_MEMBER_PROPERTY, but the property is a read-only map (xiiMap, xiiHashTable).
#define XII_MAP_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                             \
  (new xiiMemberMapProperty<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), XII_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &xiiMapPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, nullptr))

/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param EnumType
///   Name of the struct used by xiiEnum.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a XII_ACCESSOR_PROPERTY instead.
#define XII_ENUM_MEMBER_PROPERTY(PropertyName, EnumType, MemberName)                                                                                                                       \
  (new xiiEnumMemberProperty<OwnType, EnumType, XII_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                                                                                        \
                                                                                      &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
                                                                                      &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
                                                                                      &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as XII_ENUM_MEMBER_PROPERTY, but the property is read-only.
#define XII_ENUM_MEMBER_PROPERTY_READ_ONLY(PropertyName, EnumType, MemberName)                                                                                                                      \
  (new xiiEnumMemberProperty<OwnType, EnumType, XII_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                                                                                                 \
                                                                                      &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
                                                                                      &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as XII_ENUM_MEMBER_PROPERTY, but for bitfields.
#define XII_BITFLAGS_MEMBER_PROPERTY(PropertyName, BitflagsType, MemberName)                                                                                                                       \
  (new xiiBitflagsMemberProperty<OwnType, BitflagsType, XII_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                                                                                        \
                                                                                              &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
                                                                                              &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
                                                                                              &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// Same as XII_ENUM_MEMBER_PROPERTY_READ_ONLY, but for bitfields.
#define XII_BITFLAGS_MEMBER_PROPERTY_READ_ONLY(PropertyName, BitflagsType, MemberName)                                                                                                                      \
  (new xiiBitflagsMemberProperty<OwnType, BitflagsType, XII_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                                                                                                 \
                                                                                              &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
                                                                                              &xiiPropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))



/// Within a XII_BEGIN_PROPERTIES / XII_END_PROPERTIES; block, this adds a constant property stored inside the RTTI data.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param Value
///   The constant value to be stored.
#define XII_CONSTANT_PROPERTY(PropertyName, Value) (new xiiConstantProperty<decltype(Value)>(PropertyName, Value))



// [internal] Helper macro
#define XII_ENUM_VALUE_TO_CONSTANT_PROPERTY(name) XII_CONSTANT_PROPERTY(XII_PP_STRINGIFY(name), (Storage)name),

/// Within a XII_BEGIN_STATIC_REFLECTED_ENUM / XII_END_STATIC_REFLECTED_ENUM block, this converts a
/// list of enum values into constant RTTI properties.
#define XII_ENUM_CONSTANTS(...) XII_EXPAND_ARGS(XII_ENUM_VALUE_TO_CONSTANT_PROPERTY, ##__VA_ARGS__)

/// Within a XII_BEGIN_STATIC_REFLECTED_ENUM / XII_END_STATIC_REFLECTED_ENUM block, this converts a
/// an enum value into a constant RTTI property.
#define XII_ENUM_CONSTANT(Value) XII_CONSTANT_PROPERTY(XII_PP_STRINGIFY(Value), (Storage)Value)

/// Within a XII_BEGIN_STATIC_REFLECTED_BITFLAGS / XII_END_STATIC_REFLECTED_BITFLAGS block, this converts a
/// list of bitflags into constant RTTI properties.
#define XII_BITFLAGS_CONSTANTS(...) XII_EXPAND_ARGS(XII_ENUM_VALUE_TO_CONSTANT_PROPERTY, ##__VA_ARGS__)

/// Within a XII_BEGIN_STATIC_REFLECTED_BITFLAGS / XII_END_STATIC_REFLECTED_BITFLAGS block, this converts a
/// an bitflags into a constant RTTI property.
#define XII_BITFLAGS_CONSTANT(Value) XII_CONSTANT_PROPERTY(XII_PP_STRINGIFY(Value), (Storage)Value)



/// Implements the necessary functionality for an enum to be statically reflectable.
///
/// \param Type
///   The enum struct used by xiiEnum for which reflection should be defined.
/// \param Version
///   The version of \a Type. Must be increased when the class changes.
#define XII_BEGIN_STATIC_REFLECTED_ENUM(Type, Version)                            \
  XII_BEGIN_STATIC_REFLECTED_TYPE(Type, xiiEnumBase, Version, xiiRTTINoAllocator) \
    ;                                                                             \
    using Storage = Type::StorageType;                                            \
    XII_BEGIN_PROPERTIES                                                          \
      {                                                                           \
        XII_CONSTANT_PROPERTY(XII_PP_STRINGIFY(Type::Default), (Storage)Type::Default),

#define XII_END_STATIC_REFLECTED_ENUM \
  }                                   \
  XII_END_PROPERTIES                  \
  ;                                   \
  flags |= xiiTypeFlags::IsEnum;      \
  flags.Remove(xiiTypeFlags::Class);  \
  XII_END_STATIC_REFLECTED_TYPE


/// Implements the necessary functionality for bitflags to be statically reflectable.
///
/// \param Type
///   The bitflags struct used by xiiBitflags for which reflection should be defined.
/// \param Version
///   The version of \a Type. Must be increased when the class changes.
#define XII_BEGIN_STATIC_REFLECTED_BITFLAGS(Type, Version)                            \
  XII_BEGIN_STATIC_REFLECTED_TYPE(Type, xiiBitflagsBase, Version, xiiRTTINoAllocator) \
    ;                                                                                 \
    using Storage = Type::StorageType;                                                \
    XII_BEGIN_PROPERTIES                                                              \
      {                                                                               \
        XII_CONSTANT_PROPERTY(XII_PP_STRINGIFY(Type::Default), (Storage)Type::Default),

#define XII_END_STATIC_REFLECTED_BITFLAGS \
  }                                       \
  XII_END_PROPERTIES                      \
  ;                                       \
  flags |= xiiTypeFlags::Bitflags;        \
  flags.Remove(xiiTypeFlags::Class);      \
  XII_END_STATIC_REFLECTED_TYPE



/// Within an XII_BEGIN_REFLECTED_TYPE / XII_END_REFLECTED_TYPE block, use this to start the block that declares all the message
/// handlers.
#define XII_BEGIN_MESSAGEHANDLERS static xiiAbstractMessageHandler* HandlerList[] =


/// Ends the block to declare message handlers that was started with XII_BEGIN_MESSAGEHANDLERS.
#define XII_END_MESSAGEHANDLERS \
  ;                             \
  MessageHandlers = HandlerList


/// Within an XII_BEGIN_MESSAGEHANDLERS / XII_END_MESSAGEHANDLERS; block, this adds another message handler.
///
/// \param MessageType
///   The type of message that this handler function accepts. You may add 'const' in front of it.
/// \param FunctionName
///   The actual C++ name of the message handler function.
///
/// \note A message handler is a function that takes one parameter of type xiiMessage (or a derived type) and returns void.
#define XII_MESSAGE_HANDLER(MessageType, FunctionName)                                                                                    \
  new xiiInternal::MessageHandler<XII_IS_CONST_MESSAGE_HANDLER(OwnType, MessageType, &OwnType::FunctionName)>::Impl<OwnType, MessageType, \
                                                                                                                    &OwnType::FunctionName>()


/// Within an XII_BEGIN_REFLECTED_TYPE / XII_END_REFLECTED_TYPE block, use this to start the block that declares all the message
/// senders.
#define XII_BEGIN_MESSAGESENDERS static xiiMessageSenderInfo SenderList[] =


/// Ends the block to declare message senders that was started with XII_BEGIN_MESSAGESENDERS.
#define XII_END_MESSAGESENDERS \
  ;                            \
  MessageSenders = SenderList;

/// Within an XII_BEGIN_MESSAGESENDERS / XII_END_MESSAGESENDERS block, this adds another message sender.
///
/// \param MemberName
///   The name of the member variable that should get exposed as a message sender.
///
/// \note A message sender must be derived from xiiMessageSenderBase.
#define XII_MESSAGE_SENDER(MemberName) \
  {                                    \
    #MemberName, xiiGetStaticRTTI<XII_MEMBER_TYPE(OwnType, MemberName)::MessageType>()}
