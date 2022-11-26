#pragma once

#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiRTTI;
class xiiPhantomRttiManager;
class xiiReflectedTypeStorageManager;

/// \brief Event message used by the xiiPhantomRttiManager.
struct XII_TOOLSFOUNDATION_DLL xiiPhantomTypeChange
{
  const xiiRTTI* m_pChangedType;
};

struct XII_TOOLSFOUNDATION_DLL xiiAttributeHolder
{
  xiiAttributeHolder();
  xiiAttributeHolder(const xiiAttributeHolder& rhs);
  virtual ~xiiAttributeHolder();

  xiiUInt32             GetCount() const;
  xiiPropertyAttribute* GetValue(xiiUInt32 uiIndex) const;
  void                  SetValue(xiiUInt32 uiIndex, xiiPropertyAttribute* value);
  void                  Insert(xiiUInt32 uiIndex, xiiPropertyAttribute* value);
  void                  Remove(xiiUInt32 uiIndex);

  void operator=(const xiiAttributeHolder& rhs);

  mutable xiiHybridArray<xiiPropertyAttribute*, 2> m_Attributes;
  xiiArrayPtr<xiiPropertyAttribute* const>         m_ReferenceAttributes;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiAttributeHolder);

/// \brief Stores the description of a reflected property in a serializable form, used by xiiReflectedTypeDescriptor.
struct XII_TOOLSFOUNDATION_DLL xiiReflectedPropertyDescriptor : public xiiAttributeHolder
{
  xiiReflectedPropertyDescriptor() {}
  xiiReflectedPropertyDescriptor(xiiPropertyCategory::Enum category, const char* szName, const char* szType, xiiBitflags<xiiPropertyFlags> flags);
  xiiReflectedPropertyDescriptor(xiiPropertyCategory::Enum category, const char* szName, const char* szType, xiiBitflags<xiiPropertyFlags> flags,
                                 const xiiArrayPtr<xiiPropertyAttribute* const> attributes); // [tested]
  /// \brief Initialize to a constant.
  xiiReflectedPropertyDescriptor(
    const char*                                    szName,
    const xiiVariant&                              constantValue,
    const xiiArrayPtr<xiiPropertyAttribute* const> attributes); // [tested]
  xiiReflectedPropertyDescriptor(const xiiReflectedPropertyDescriptor& rhs);
  ~xiiReflectedPropertyDescriptor();

  void operator=(const xiiReflectedPropertyDescriptor& rhs);

  xiiEnum<xiiPropertyCategory> m_Category;
  xiiString                    m_sName; ///< The name of this property. E.g. what xiiAbstractProperty::GetPropertyName() returns.
  xiiString                    m_sType; ///< The name of the type of the property. E.g. xiiAbstractProperty::GetSpecificType().GetTypeName()

  xiiBitflags<xiiPropertyFlags> m_Flags;
  xiiVariant                    m_ConstantValue;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiReflectedPropertyDescriptor);

struct XII_TOOLSFOUNDATION_DLL xiiFunctionArgumentDescriptor
{
  xiiFunctionArgumentDescriptor();
  xiiFunctionArgumentDescriptor(const char* szType, xiiBitflags<xiiPropertyFlags> flags);
  xiiString                     m_sType;
  xiiBitflags<xiiPropertyFlags> m_Flags;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiFunctionArgumentDescriptor);

/// \brief Stores the description of a reflected function in a serializable form, used by xiiReflectedTypeDescriptor.
struct XII_TOOLSFOUNDATION_DLL xiiReflectedFunctionDescriptor : public xiiAttributeHolder
{
  xiiReflectedFunctionDescriptor();
  xiiReflectedFunctionDescriptor(
    const char*                                    szName,
    xiiBitflags<xiiPropertyFlags>                  flags,
    xiiEnum<xiiFunctionType>                       type,
    const xiiArrayPtr<xiiPropertyAttribute* const> attributes);

  xiiReflectedFunctionDescriptor(const xiiReflectedFunctionDescriptor& rhs);
  ~xiiReflectedFunctionDescriptor();

  void operator=(const xiiReflectedFunctionDescriptor& rhs);

  xiiString                                      m_sName;
  xiiBitflags<xiiPropertyFlags>                  m_Flags;
  xiiEnum<xiiFunctionType>                       m_Type;
  xiiFunctionArgumentDescriptor                  m_ReturnValue;
  xiiDynamicArray<xiiFunctionArgumentDescriptor> m_Arguments;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiReflectedFunctionDescriptor);


/// \brief Stores the description of a reflected type in a serializable form. Used by xiiPhantomRttiManager to add new types.
struct XII_TOOLSFOUNDATION_DLL xiiReflectedTypeDescriptor : public xiiAttributeHolder
{
  ~xiiReflectedTypeDescriptor();

  xiiString m_sTypeName;
  xiiString m_sPluginName;
  xiiString m_sParentTypeName;

  xiiBitflags<xiiTypeFlags>                       m_Flags;
  xiiDynamicArray<xiiReflectedPropertyDescriptor> m_Properties;
  xiiDynamicArray<xiiReflectedFunctionDescriptor> m_Functions;
  xiiUInt32                                       m_uiTypeVersion = 1;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiReflectedTypeDescriptor);
