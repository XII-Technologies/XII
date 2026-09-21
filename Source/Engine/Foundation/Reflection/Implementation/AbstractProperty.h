/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

class xiiRTTI;
class xiiPropertyAttribute;

/// Determines whether a type is xiiIsBitflags.
template <typename T>
struct xiiIsBitflags
{
  static constexpr bool value = false;
};

template <typename T>
struct xiiIsBitflags<xiiBitflags<T>>
{
  static constexpr bool value = true;
};

/// Determines whether a type is xiiIsBitflags.
template <typename T>
struct xiiIsEnum
{
  static constexpr bool value = std::is_enum<T>::value;
};

template <typename T>
struct xiiIsEnum<xiiEnum<T>>
{
  static constexpr bool value = true;
};

/// Flags used to describe a property and its type.
struct xiiPropertyFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    StandardType = XII_BIT(0), ///< Anything that can be stored inside a xiiVariant except for pointers and containers.
    IsEnum       = XII_BIT(1), ///< enum property, cast to xiiAbstractEnumerationProperty.
    Bitflags     = XII_BIT(2), ///< Bitflags property, cast to xiiAbstractEnumerationProperty.
    Class        = XII_BIT(3), ///< A struct or class. All of the above are mutually exclusive.

    Const     = XII_BIT(4), ///< Property value is const.
    Reference = XII_BIT(5), ///< Property value is a reference.
    Pointer   = XII_BIT(6), ///< Property value is a pointer.

    PointerOwner = XII_BIT(7),  ///< This pointer property takes ownership of the passed pointer.
    ReadOnly     = XII_BIT(8),  ///< Can only be read but not modified.
    Hidden       = XII_BIT(9),  ///< This property should not appear in the UI.
    Phantom      = XII_BIT(10), ///< Phantom types are mirrored types on the editor side. Ie. they do not exist as actual classes in the process. Also used
                                ///< for data driven types, e.g. by the Visual Shader asset.

    VarOut   = XII_BIT(11), ///< Tag for non-const-ref function parameters to indicate usage 'out'
    VarInOut = XII_BIT(12), ///< Tag for non-const-ref function parameters to indicate usage 'inout'

    PureFunction = Const, ///< The visual script function doesn't need an execution pin.

    Default = 0,
    Void    = 0
  };

  struct Bits
  {
    StorageType StandardType : 1;
    StorageType IsEnum : 1;
    StorageType Bitflags : 1;
    StorageType Class : 1;

    StorageType Const : 1;
    StorageType Reference : 1;
    StorageType Pointer : 1;

    StorageType PointerOwner : 1;
    StorageType ReadOnly : 1;
    StorageType Hidden : 1;
    StorageType Phantom : 1;

    StorageType VarOut : 1;
    StorageType VarInOut : 1;

    StorageType PureFunction : 1;
  };

  template <class Type>
  static xiiBitflags<xiiPropertyFlags> GetParameterFlags()
  {
    using CleanType = typename xiiTypeTraits<Type>::NonConstReferencePointerType;

    xiiBitflags<xiiPropertyFlags>  flags;
    constexpr xiiVariantType::Enum type = static_cast<xiiVariantType::Enum>(xiiVariantTypeDeduction<CleanType>::value);
    if constexpr (std::is_same<CleanType, xiiVariant>::value || (type >= xiiVariantType::FirstStandardType && type <= xiiVariantType::LastStandardType) || std::is_same<Type, const char*>::value) // We treat const char* as a basic type and not a pointer.
      flags.Add(xiiPropertyFlags::StandardType);
    else if constexpr (xiiIsEnum<CleanType>::value)
      flags.Add(xiiPropertyFlags::IsEnum);
    else if constexpr (xiiIsBitflags<CleanType>::value)
      flags.Add(xiiPropertyFlags::Bitflags);
    else
      flags.Add(xiiPropertyFlags::Class);

    if constexpr (std::is_const<typename xiiTypeTraits<Type>::NonReferencePointerType>::value)
      flags.Add(xiiPropertyFlags::Const);

    if constexpr (std::is_pointer<Type>::value && !std::is_same<Type, const char*>::value)
      flags.Add(xiiPropertyFlags::Pointer);

    if constexpr (std::is_reference<Type>::value)
      flags.Add(xiiPropertyFlags::Reference);

    return flags;
  }
};

template <>
inline xiiBitflags<xiiPropertyFlags> xiiPropertyFlags::GetParameterFlags<void>()
{
  return xiiBitflags<xiiPropertyFlags>();
}

XII_DECLARE_FLAGS_OPERATORS(xiiPropertyFlags)

/// Describes what category a property belongs to.
struct xiiPropertyCategory
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Constant, ///< The property is a constant value that is stored inside the RTTI data.
    Member,   ///< The property is a 'member property', i.e. it represents some accessible value. Cast to xiiAbstractMemberProperty.
    Function, ///< The property is a function which can be called. Cast to xiiAbstractFunctionProperty.
    Array,    ///< The property is actually an array of values. The array dimensions might be changeable. Cast to xiiAbstractArrayProperty.
    Set,      ///< The property is actually a set of values. Cast to xiiAbstractSetProperty.
    Map,      ///< The property is actually a map from string to values. Cast to xiiAbstractMapProperty.

    Default = Member
  };
};

/// This is the base interface for all properties in the reflection system. It provides enough information to cast to the next better
/// base class.
class XII_FOUNDATION_DLL xiiAbstractProperty
{
public:
  /// The constructor must get the name of the property. The string must be a compile-time constant.
  xiiAbstractProperty(xiiStringView sPropertyName) { m_sPropertyName = sPropertyName; }

  virtual ~xiiAbstractProperty();

  /// Returns the name of the property.
  xiiStringView GetPropertyName() const { return m_sPropertyName; }

  /// Returns the type information of the constant property. Use this to cast this property to a specific version of
  /// xiiTypedConstantProperty.
  virtual const xiiRTTI* GetSpecificType() const = 0;

  /// Returns the category of this property. Cast this property to the next higher type for more information.
  virtual xiiPropertyCategory::Enum GetCategory() const = 0; // [tested]

  /// Returns the flags of the property.
  const xiiBitflags<xiiPropertyFlags>& GetFlags() const { return m_Flags; };

  /// Adds flags to the property. Returns itself to allow to be called during initialization.
  xiiAbstractProperty* AddFlags(xiiBitflags<xiiPropertyFlags> flags)
  {
    m_Flags.Add(flags);
    return this;
  };

  /// Adds attributes to the property. Returns itself to allow to be called during initialization. Allocate an attribute using
  /// standard 'new'.
  xiiAbstractProperty* AddAttributes(xiiPropertyAttribute* pAttrib1, xiiPropertyAttribute* pAttrib2 = nullptr, xiiPropertyAttribute* pAttrib3 = nullptr, xiiPropertyAttribute* pAttrib4 = nullptr, xiiPropertyAttribute* pAttrib5 = nullptr, xiiPropertyAttribute* pAttrib6 = nullptr)
  {
    XII_ASSERT_DEV(pAttrib1 != nullptr, "invalid attribute");

    m_Attributes.PushBack(pAttrib1);
    if (pAttrib2)
      m_Attributes.PushBack(pAttrib2);
    if (pAttrib3)
      m_Attributes.PushBack(pAttrib3);
    if (pAttrib4)
      m_Attributes.PushBack(pAttrib4);
    if (pAttrib5)
      m_Attributes.PushBack(pAttrib5);
    if (pAttrib6)
      m_Attributes.PushBack(pAttrib6);
    return this;
  };

  /// Returns the array of property attributes.
  xiiArrayPtr<const xiiPropertyAttribute* const> GetAttributes() const { return m_Attributes; }

  /// Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template <typename Type>
  const Type* GetAttributeByType() const;

protected:
  xiiBitflags<xiiPropertyFlags>                                              m_Flags;
  xiiStringView                                                              m_sPropertyName;
  xiiHybridArray<const xiiPropertyAttribute*, 2U, xiiStaticAllocatorWrapper> m_Attributes; // Do not track RTTI data.
};

/// This is the base class for all constant properties that are stored inside the RTTI data.
class XII_FOUNDATION_DLL xiiAbstractConstantProperty : public xiiAbstractProperty
{
public:
  /// Passes the property name through to xiiAbstractProperty.
  xiiAbstractConstantProperty(xiiStringView sPropertyName) :
    xiiAbstractProperty(sPropertyName)
  {
  }

  /// Returns xiiPropertyCategory::Constant.
  virtual xiiPropertyCategory::Enum GetCategory() const override { return xiiPropertyCategory::Constant; } // [tested]

  /// Returns a pointer to the constant data or nullptr. See xiiAbstractMemberProperty::GetPropertyPointer for more information.
  virtual void* GetPropertyPointer() const = 0;

  /// Returns the constant value as a xiiVariant
  virtual xiiVariant GetConstant() const = 0;
};

/// This is the base class for all properties that are members of a class. It provides more information about the actual type.
///
/// If xiiPropertyFlags::Pointer is set as a flag, you must not cast this property to xiiTypedMemberProperty, instead use GetValuePtr and
/// SetValuePtr. This is because reference and const-ness of the property are only fixed for the pointer but not the type, so the actual
/// property type cannot be derived.
class XII_FOUNDATION_DLL xiiAbstractMemberProperty : public xiiAbstractProperty
{
public:
  /// Passes the property name through to xiiAbstractProperty.
  xiiAbstractMemberProperty(xiiStringView sPropertyName) :
    xiiAbstractProperty(sPropertyName)
  {
  }

  /// Returns xiiPropertyCategory::Member.
  virtual xiiPropertyCategory::Enum GetCategory() const override { return xiiPropertyCategory::Member; }

  /// Returns a pointer to the property data or nullptr. If a valid pointer is returned, that pointer and the information from
  /// GetSpecificType() can be used to step deeper into the type (if required).
  ///
  /// You need to pass the pointer to an object on which you are operating. This function is mostly of interest when the property itself is
  /// a compound type (a struct or class). If it is a simple type (int, float, etc.) it doesn't make much sense to retrieve the pointer.
  ///
  /// For example GetSpecificType() might return that a property is of type xiiVec3. In that case one might either stop and just use the code
  /// to handle xiiVec3 types, or one might continue and enumerate all sub-properties (x, y and z) as well.
  ///
  /// \note There is no guarantee that this function returns a non-nullptr pointer, independent of the type. When a property uses custom
  /// 'accessors' (functions to get / set the property value), it is not possible (or useful) to get the property pointer.
  virtual void* GetPropertyPointer(const void* pInstance) const = 0;

  /// Writes the value of this property in pInstance to pObject.
  /// pObject needs to point to an instance of this property's type.
  virtual void GetValuePtr(const void* pInstance, void* out_pObject) const = 0;

  /// Sets the value of pObject to the property in pInstance.
  /// pObject needs to point to an instance of this property's type.
  virtual void SetValuePtr(void* pInstance, const void* pObject) const = 0;
};


/// The base class for a property that represents an array of values.
class XII_FOUNDATION_DLL xiiAbstractArrayProperty : public xiiAbstractProperty
{
public:
  /// Passes the property name through to xiiAbstractProperty.
  xiiAbstractArrayProperty(xiiStringView sPropertyName) :
    xiiAbstractProperty(sPropertyName)
  {
  }

  /// Returns xiiPropertyCategory::Array.
  virtual xiiPropertyCategory::Enum GetCategory() const override { return xiiPropertyCategory::Array; }

  /// Returns number of elements.
  virtual xiiUInt32 GetCount(const void* pInstance) const = 0;

  /// Writes element at index uiIndex to the target of pObject.
  virtual void GetValue(const void* pInstance, xiiUInt32 uiIndex, void* pObject) const = 0;

  /// Writes the target of pObject to the element at index uiIndex.
  virtual void SetValue(void* pInstance, xiiUInt32 uiIndex, const void* pObject) const = 0;

  /// Inserts the target of pObject into the array at index uiIndex.
  virtual void Insert(void* pInstance, xiiUInt32 uiIndex, const void* pObject) const = 0;

  /// Removes the element in the array at index uiIndex.
  virtual void Remove(void* pInstance, xiiUInt32 uiIndex) const = 0;

  /// Clears the array.
  virtual void Clear(void* pInstance) const = 0;

  /// Resizes the array to uiCount.
  virtual void SetCount(void* pInstance, xiiUInt32 uiCount) const = 0;

  virtual void* GetValuePointer(void* pInstance, xiiUInt32 uiIndex) const
  {
    XII_IGNORE_UNUSED(pInstance);
    XII_IGNORE_UNUSED(uiIndex);
    return nullptr;
  }
};


/// The base class for a property that represents a set of values.
///
/// The element type must either be a standard type or a pointer.
class XII_FOUNDATION_DLL xiiAbstractSetProperty : public xiiAbstractProperty
{
public:
  /// Passes the property name through to xiiAbstractProperty.
  xiiAbstractSetProperty(xiiStringView sPropertyName) :
    xiiAbstractProperty(sPropertyName)
  {
  }

  /// Returns xiiPropertyCategory::Set.
  virtual xiiPropertyCategory::Enum GetCategory() const override { return xiiPropertyCategory::Set; }

  /// Returns whether the set is empty.
  virtual bool IsEmpty(const void* pInstance) const = 0;

  /// Clears the set.
  virtual void Clear(void* pInstance) const = 0;

  /// Inserts the target of pObject into the set.
  virtual void Insert(void* pInstance, const void* pObject) const = 0;

  /// Removes the target of pObject from the set.
  virtual void Remove(void* pInstance, const void* pObject) const = 0;

  /// Returns whether the target of pObject is in the set.
  virtual bool Contains(const void* pInstance, const void* pObject) const = 0;

  /// Writes the content of the set to out_keys.
  virtual void GetValues(const void* pInstance, xiiDynamicArray<xiiVariant>& out_keys) const = 0;
};


/// The base class for a property that represents a set of values.
///
/// The element type must either be a standard type or a pointer.
class XII_FOUNDATION_DLL xiiAbstractMapProperty : public xiiAbstractProperty
{
public:
  /// Passes the property name through to xiiAbstractProperty.
  xiiAbstractMapProperty(xiiStringView sPropertyName) :
    xiiAbstractProperty(sPropertyName)
  {
  }

  /// Returns xiiPropertyCategory::Map.
  virtual xiiPropertyCategory::Enum GetCategory() const override { return xiiPropertyCategory::Map; }

  /// Returns whether the set is empty.
  virtual bool IsEmpty(const void* pInstance) const = 0;

  /// Clears the set.
  virtual void Clear(void* pInstance) const = 0;

  /// Inserts the target of pObject into the set.
  virtual void Insert(void* pInstance, xiiStringView sKey, const void* pObject) const = 0;

  /// Removes the target of pObject from the set.
  virtual void Remove(void* pInstance, xiiStringView sKey) const = 0;

  /// Returns whether the target of pObject is in the set.
  virtual bool Contains(const void* pInstance, xiiStringView sKey) const = 0;

  /// Writes element at index uiIndex to the target of pObject.
  virtual bool GetValue(const void* pInstance, xiiStringView sKey, void* pObject) const = 0;

  /// Writes the content of the set to out_keys.
  virtual void GetKeys(const void* pInstance, xiiHybridArray<xiiString, 16>& out_keys) const = 0;
};

/// Use getArgument<N, Args...>::Type to get the type of the Nth argument in Args.
template <xiiInt32 _Index, class... Args>
struct getArgument;

template <class Head, class... Tail>
struct getArgument<0, Head, Tail...>
{
  using Type = Head;
};

template <xiiInt32 _Index, class Head, class... Tail>
struct getArgument<_Index, Head, Tail...>
{
  using Type = typename getArgument<_Index - 1, Tail...>::Type;
};

/// Template that allows to probe a function for a parameter and return type.
template <xiiInt32 I, typename FUNC>
struct xiiFunctionParameterTypeResolver
{
};

template <xiiInt32 I, typename R, typename... P>
struct xiiFunctionParameterTypeResolver<I, R (*)(P...)>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  static_assert(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType    = R;
};

template <xiiInt32 I, class Class, typename R, typename... P>
struct xiiFunctionParameterTypeResolver<I, R (Class::*)(P...)>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  static_assert(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType    = R;
};

template <xiiInt32 I, class Class, typename R, typename... P>
struct xiiFunctionParameterTypeResolver<I, R (Class::*)(P...) const>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  static_assert(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType    = R;
};

/// Template that allows to probe a single parameter function for parameter and return type.
template <typename FUNC>
struct xiiMemberFunctionParameterTypeResolver
{
};

template <class Class, typename R, typename P>
struct xiiMemberFunctionParameterTypeResolver<R (Class::*)(P)>
{
  using ParameterType = P;
  using ReturnType    = R;
};

/// Template that allows to probe a container for its element type.
template <typename CONTAINER>
struct xiiContainerSubTypeResolver
{
};

template <typename T>
struct xiiContainerSubTypeResolver<xiiArrayPtr<T>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct xiiContainerSubTypeResolver<xiiDynamicArray<T>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename T, xiiUInt32 Size>
struct xiiContainerSubTypeResolver<xiiHybridArray<T, Size>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename T, xiiUInt32 Size>
struct xiiContainerSubTypeResolver<xiiStaticArray<T, Size>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename T, xiiUInt16 Size>
struct xiiContainerSubTypeResolver<xiiSmallArray<T, Size>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct xiiContainerSubTypeResolver<xiiDeque<T>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct xiiContainerSubTypeResolver<xiiSet<T>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct xiiContainerSubTypeResolver<xiiHashSet<T>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename K, typename T>
struct xiiContainerSubTypeResolver<xiiHashTable<K, T>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};

template <typename K, typename T>
struct xiiContainerSubTypeResolver<xiiMap<K, T>>
{
  using Type = typename xiiTypeTraits<T>::NonConstReferenceType;
};


/// Describes what kind of function a property is.
struct xiiFunctionType
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Member,       ///< A normal member function, a valid instance pointer must be provided to call.
    StaticMember, ///< A static member function, instance pointer will be ignored.
    Constructor,  ///< A constructor. Return value is a void* pointing to the new instance allocated with the default allocator.

    Default = Member
  };
};

/// The base class for a property that represents a function.
class XII_FOUNDATION_DLL xiiAbstractFunctionProperty : public xiiAbstractProperty
{
public:
  /// Passes the property name through to xiiAbstractProperty.
  xiiAbstractFunctionProperty(xiiStringView sPropertyName) :
    xiiAbstractProperty(sPropertyName)
  {
  }

  virtual xiiPropertyCategory::Enum GetCategory() const override { return xiiPropertyCategory::Function; }

  /// Returns the type of function, see xiiFunctionPropertyType::Enum.
  virtual xiiFunctionType::Enum GetFunctionType() const = 0;

  /// Returns the type of the return value.
  virtual const xiiRTTI* GetReturnType() const = 0;

  /// Returns property flags of the return value.
  virtual xiiBitflags<xiiPropertyFlags> GetReturnFlags() const = 0;

  /// Returns the number of arguments.
  virtual xiiUInt32 GetArgumentCount() const = 0;

  /// Returns the type of the given argument.
  virtual const xiiRTTI* GetArgumentType(xiiUInt32 uiParamIndex) const = 0;

  /// Returns the property flags of the given argument.
  virtual xiiBitflags<xiiPropertyFlags> GetArgumentFlags(xiiUInt32 uiParamIndex) const = 0;

  /// Calls the function. Provide the instance on which the function is supposed to be called.
  ///
  /// arguments must be the size of GetArgumentCount, the following rules apply for both arguments and return value:
  /// Any standard type must be provided by value, even if it is a pointer to one. Types must match exactly, no ConvertTo is called.
  /// enum and bitflags are supported if xiiEnum / xiiBitflags is used, value must be provided as xiiInt64.
  /// Out values (&, *) are written back to the variant they were read from.
  /// Any class is provided by pointer, regardless of whether it is a pointer or not.
  /// The returnValue must only be valid if the return value is a ref or by value class. In that case
  /// returnValue must be a ptr to a valid class instance of the returned type.
  /// An invalid variant is equal to a nullptr, except for if the argument is of type xiiVariant, in which case
  /// it is impossible to pass along a nullptr.
  virtual void Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& out_returnValue) const = 0;

  virtual const xiiRTTI* GetSpecificType() const override { return GetReturnType(); }

  /// Adds flags to the property. Returns itself to allow to be called during initialization.
  xiiAbstractFunctionProperty* AddFlags(xiiBitflags<xiiPropertyFlags> flags)
  {
    return static_cast<xiiAbstractFunctionProperty*>(xiiAbstractProperty::AddFlags(flags));
  }

  /// Adds attributes to the property. Returns itself to allow to be called during initialization. Allocate an attribute using
  /// standard 'new'.
  xiiAbstractFunctionProperty* AddAttributes(xiiPropertyAttribute* pAttrib1, xiiPropertyAttribute* pAttrib2 = nullptr, xiiPropertyAttribute* pAttrib3 = nullptr, xiiPropertyAttribute* pAttrib4 = nullptr, xiiPropertyAttribute* pAttrib5 = nullptr, xiiPropertyAttribute* pAttrib6 = nullptr)
  {
    return static_cast<xiiAbstractFunctionProperty*>(xiiAbstractProperty::AddAttributes(pAttrib1, pAttrib2, pAttrib3, pAttrib4, pAttrib5, pAttrib6));
  }
};
