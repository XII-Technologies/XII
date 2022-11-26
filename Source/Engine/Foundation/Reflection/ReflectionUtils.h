#pragma once

#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>

class xiiVariant;
class xiiAbstractProperty;

/// \brief Helper functions for handling reflection related operations.
class XII_FOUNDATION_DLL xiiReflectionUtils
{
public:
  static const xiiRTTI* GetCommonBaseType(const xiiRTTI* pRtti1, const xiiRTTI* pRtti2);

  /// \brief Returns whether a type can be stored directly inside a xiiVariant.
  static bool IsBasicType(const xiiRTTI* pRtti);

  /// \brief Returns whether the property is a non-ptr basic type or custom type.
  static bool IsValueType(const xiiAbstractProperty* pProp);

  /// \brief Returns the RTTI type matching the variant's type.
  static const xiiRTTI* GetTypeFromVariant(const xiiVariant& value);
  static const xiiRTTI* GetTypeFromVariant(xiiVariantType::Enum type);

  /// \brief Sets the Nth component of the vector to the given value.
  ///
  /// vector's type needs to be in between xiiVariant::Type::Vector2 and xiiVariant::Type::Vector4U.
  static xiiUInt32 GetComponentCount(xiiVariantType::Enum type);
  static void      SetComponent(xiiVariant& vector, xiiUInt32 iComponent, double fValue); // [tested]
  static double    GetComponent(const xiiVariant& vector, xiiUInt32 iComponent);

  static xiiVariant GetMemberPropertyValue(const xiiAbstractMemberProperty* pProp, const void* pObject);              // [tested] via ToolsFoundation
  static void       SetMemberPropertyValue(xiiAbstractMemberProperty* pProp, void* pObject, const xiiVariant& value); // [tested] via ToolsFoundation

  static xiiVariant GetArrayPropertyValue(const xiiAbstractArrayProperty* pProp, const void* pObject, xiiUInt32 uiIndex);
  static void       SetArrayPropertyValue(xiiAbstractArrayProperty* pProp, void* pObject, xiiUInt32 uiIndex, const xiiVariant& value);

  static void InsertSetPropertyValue(xiiAbstractSetProperty* pProp, void* pObject, const xiiVariant& value);
  static void RemoveSetPropertyValue(xiiAbstractSetProperty* pProp, void* pObject, const xiiVariant& value);

  static xiiVariant GetMapPropertyValue(const xiiAbstractMapProperty* pProp, const void* pObject, const char* szKey);
  static void       SetMapPropertyValue(xiiAbstractMapProperty* pProp, void* pObject, const char* szKey, const xiiVariant& value);

  static void InsertArrayPropertyValue(xiiAbstractArrayProperty* pProp, void* pObject, const xiiVariant& value, xiiUInt32 uiIndex);
  static void RemoveArrayPropertyValue(xiiAbstractArrayProperty* pProp, void* pObject, xiiUInt32 uiIndex);

  static xiiAbstractMemberProperty* GetMemberProperty(const xiiRTTI* pRtti, xiiUInt32 uiPropertyIndex);
  static xiiAbstractMemberProperty* GetMemberProperty(const xiiRTTI* pRtti, const char* szPropertyName); // [tested] via ToolsFoundation

  /// \brief Gathers all RTTI types that are derived from pRtti.
  ///
  /// This includes all classes that have pRtti as a base class, either direct or indirect.
  /// If bIncludeDependencies is set to true, the resulting set will also contain all dependent types.
  ///
  /// \sa GatherDependentTypes
  static void GatherTypesDerivedFromClass(const xiiRTTI* pRtti, xiiSet<const xiiRTTI*>& out_types, bool bIncludeDependencies);

  /// \brief Gathers all RTTI types that pRtti depends on and adds them to inout_types.
  ///
  /// Dependencies are either member properties or base classes. The output contains the transitive closure of the dependencies.
  /// Note that inout_types is not cleared when this function is called.
  static void GatherDependentTypes(const xiiRTTI* pRtti, xiiSet<const xiiRTTI*>& inout_types);

  /// \brief Sorts the input types according to their dependencies.
  ///
  /// Types that have no dependences come first in the output followed by types that have their dependencies met by
  /// the previous entries in the output.
  /// If circular dependencies are found the function returns false.
  static bool CreateDependencySortedTypeArray(const xiiSet<const xiiRTTI*>& types, xiiDynamicArray<const xiiRTTI*>& out_sortedTypes);

  struct EnumConversionMode
  {
    enum Enum
    {
      FullyQualifiedName,
      ValueNameOnly,
      Default = FullyQualifiedName
    };

    using StorageType = xiiUInt8;
  };

  /// \brief Converts an enum or bitfield value into its string representation.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of out_sOutput equals MSVC debugger output.
  static bool EnumerationToString(const xiiRTTI* pEnumerationRtti, xiiInt64 iValue, xiiStringBuilder& out_sOutput,
                                  xiiEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default); // [tested]

  /// \brief Helper template to shorten the call for xiiEnums
  template <typename T>
  static bool EnumerationToString(xiiEnum<T> value, xiiStringBuilder& out_sOutput, xiiEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default)
  {
    return EnumerationToString(xiiGetStaticRTTI<T>(), value.GetValue(), out_sOutput, conversionMode);
  }

  /// \brief Helper template to shorten the call for xiiBitflags
  template <typename T>
  static bool BitflagsToString(xiiBitflags<T> value, xiiStringBuilder& out_sOutput, xiiEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default)
  {
    return EnumerationToString(xiiGetStaticRTTI<T>(), value.GetValue(), out_sOutput, conversionMode);
  }

  struct EnumKeyValuePair
  {
    xiiString m_sKey;
    xiiInt32  m_iValue = 0;
  };

  /// \brief If the given type is an enum, \a entries will be filled with all available keys (strings) and values (integers).
  static void GetEnumKeysAndValues(const xiiRTTI* pEnumerationRtti, xiiDynamicArray<EnumKeyValuePair>& entries, xiiEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default);

  /// \brief Converts an enum or bitfield in its string representation to its value.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of szValue must equal the MSVC debugger output.
  static bool StringToEnumeration(const xiiRTTI* pEnumerationRtti, const char* szValue, xiiInt64& out_iValue); // [tested]

  /// \brief Helper template to shorten the call for xiiEnums
  template <typename T>
  static bool StringToEnumeration(const char* szValue, xiiEnum<T>& out_iValue)
  {
    xiiInt64   value;
    const auto retval = StringToEnumeration(xiiGetStaticRTTI<T>(), szValue, value);
    out_iValue        = static_cast<typename T::Enum>(value);
    return retval;
  }

  /// \brief Returns the default value (Enum::Default) for the given enumeration type.
  static xiiInt64 DefaultEnumerationValue(const xiiRTTI* pEnumerationRtti); // [tested]

  /// \brief Makes sure the given value is valid under the given enumeration type.
  ///
  /// Invalid bitflag bits are removed and an invalid enum value is replaced by the default value.
  static xiiInt64 MakeEnumerationValid(const xiiRTTI* pEnumerationRtti, xiiInt64 iValue); // [tested]

  /// \brief Templated convenience function that calls IsEqual and automatically deduces the type.
  template <typename T>
  static bool IsEqual(const T* pObject, const T* pObject2)
  {
    return IsEqual(pObject, pObject2, xiiGetStaticRTTI<T>());
  }

  /// \brief Compares pObject with pObject2 of type pType and returns whether they are equal.
  ///
  /// In case a class derived from xiiReflectedClass is passed in the correct derived type
  /// will automatically be determined so it is not necessary to put the exact type into pType,
  /// any derived class type will do. However, the function will return false  pObject and pObject2
  /// actually have a different type.
  static bool IsEqual(const void* pObject, const void* pObject2, const xiiRTTI* pType); // [tested]

  /// \brief Compares property pProp of pObject and pObject2 and returns whether it is equal in both.
  static bool IsEqual(const void* pObject, const void* pObject2, xiiAbstractProperty* pProp);

  /// \brief Deletes pObject using the allocator found in the owning property's type.
  static void DeleteObject(void* pObject, xiiAbstractProperty* pOwnerProperty);

  /// \brief Returns a global default initialization value for the given variant type.
  static xiiVariant GetDefaultVariantFromType(xiiVariant::Type::Enum type); // [tested]

  /// \brief Returns the default value for the specific type
  static xiiVariant GetDefaultVariantFromType(const xiiRTTI* pRtti);

  /// \brief Returns the default value for the specific type of the given property.
  static xiiVariant GetDefaultValue(const xiiAbstractProperty* pProperty, xiiVariant index = xiiVariant());


  /// \brief Sets all member properties in \a pObject of type \a pRtti to the value returned by xiiToolsReflectionUtils::GetDefaultValue()
  static void SetAllMemberPropertiesToDefault(const xiiRTTI* pRtti, void* pObject);

  /// \brief If pAttrib is valid and its min/max values are compatible, value will be clamped to them.
  /// Returns false if a clamp attribute exists but no clamp code was executed.
  static xiiResult ClampValue(xiiVariant& value, const xiiClampValueAttribute* pAttrib);
};
