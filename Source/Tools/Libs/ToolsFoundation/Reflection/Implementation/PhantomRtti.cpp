#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomProperty.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>

xiiPhantomRTTI::xiiPhantomRTTI(xiiStringView sName, const xiiRTTI* pParentType, xiiUInt32 uiTypeSize, xiiUInt32 uiTypeVersion, xiiUInt8 uiVariantType, xiiBitflags<xiiTypeFlags> flags, xiiStringView sPluginName) :
  xiiRTTI(nullptr, pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags | xiiTypeFlags::Phantom, nullptr, xiiArrayPtr<const xiiAbstractProperty*>(), xiiArrayPtr<const xiiAbstractFunctionProperty*>(), xiiArrayPtr<const xiiPropertyAttribute*>(), xiiArrayPtr<xiiAbstractMessageHandler*>(), xiiArrayPtr<xiiMessageSenderInfo>(), nullptr)
{
  m_sTypeNameStorage   = sName;
  m_sPluginNameStorage = sPluginName;

  m_sTypeName   = m_sTypeNameStorage;
  m_sPluginName = m_sPluginNameStorage;

  RegisterType();
}

xiiPhantomRTTI::~xiiPhantomRTTI()
{
  UnregisterType();
  m_sTypeName = {};

  for (auto pProp : m_PropertiesStorage)
  {
    XII_DEFAULT_DELETE(pProp);
  }
  m_PropertiesStorage.Clear();
  m_Properties.Clear();

  for (auto pFunc : m_FunctionsStorage)
  {
    XII_DEFAULT_DELETE(pFunc);
  }
  m_FunctionsStorage.Clear();
  m_Functions.Clear();

  for (auto pAttrib : m_AttributesStorage)
  {
    auto pAttribNonConst = const_cast<xiiPropertyAttribute*>(pAttrib);
    XII_DEFAULT_DELETE(pAttribNonConst);
  }
  m_AttributesStorage.Clear();
  m_Attributes.Clear();
}

void xiiPhantomRTTI::SetProperties(xiiDynamicArray<xiiReflectedPropertyDescriptor>& properties)
{
  for (auto pProp : m_PropertiesStorage)
  {
    XII_DEFAULT_DELETE(pProp);
  }
  m_PropertiesStorage.Clear();

  const xiiUInt32 iCount = properties.GetCount();
  m_PropertiesStorage.Reserve(iCount);

  for (xiiUInt32 i = 0; i < iCount; i++)
  {
    switch (properties[i].m_Category)
    {
      case xiiPropertyCategory::Constant:
      {
        m_PropertiesStorage.PushBack(XII_DEFAULT_NEW(xiiPhantomConstantProperty, &properties[i]));
      }
      break;
      case xiiPropertyCategory::Member:
      {
        m_PropertiesStorage.PushBack(XII_DEFAULT_NEW(xiiPhantomMemberProperty, &properties[i]));
      }
      break;
      case xiiPropertyCategory::Array:
      {
        m_PropertiesStorage.PushBack(XII_DEFAULT_NEW(xiiPhantomArrayProperty, &properties[i]));
      }
      break;
      case xiiPropertyCategory::Set:
      {
        m_PropertiesStorage.PushBack(XII_DEFAULT_NEW(xiiPhantomSetProperty, &properties[i]));
      }
      break;
      case xiiPropertyCategory::Map:
      {
        m_PropertiesStorage.PushBack(XII_DEFAULT_NEW(xiiPhantomMapProperty, &properties[i]));
      }
      break;
      case xiiPropertyCategory::Function:
        break; // Handled in SetFunctions
    }
  }

  m_Properties = m_PropertiesStorage.GetArrayPtr();
}


void xiiPhantomRTTI::SetFunctions(xiiDynamicArray<xiiReflectedFunctionDescriptor>& functions)
{
  for (auto pProp : m_FunctionsStorage)
  {
    XII_DEFAULT_DELETE(pProp);
  }
  m_FunctionsStorage.Clear();

  const xiiUInt32 iCount = functions.GetCount();
  m_FunctionsStorage.Reserve(iCount);

  for (xiiUInt32 i = 0; i < iCount; i++)
  {
    m_FunctionsStorage.PushBack(XII_DEFAULT_NEW(xiiPhantomFunctionProperty, &functions[i]));
  }

  m_Functions = m_FunctionsStorage.GetArrayPtr();
}

void xiiPhantomRTTI::SetAttributes(xiiDynamicArray<const xiiPropertyAttribute*>& attributes)
{
  for (auto pAttrib : m_AttributesStorage)
  {
    auto pAttribNonConst = const_cast<xiiPropertyAttribute*>(pAttrib);
    XII_DEFAULT_DELETE(pAttribNonConst);
  }
  m_AttributesStorage.Clear();
  m_AttributesStorage = attributes;
  m_Attributes        = m_AttributesStorage;
  attributes.Clear();
}

void xiiPhantomRTTI::UpdateType(xiiReflectedTypeDescriptor& desc)
{
  xiiRTTI::UpdateType(xiiRTTI::FindTypeByName(desc.m_sParentTypeName), 0, desc.m_uiTypeVersion, xiiVariantType::Invalid, desc.m_Flags);

  m_sPluginNameStorage = desc.m_sPluginName;
  m_sPluginName        = m_sPluginNameStorage;

  SetProperties(desc.m_Properties);
  SetFunctions(desc.m_Functions);
  SetAttributes(desc.m_Attributes);
  SetupParentHierarchy();
}

bool xiiPhantomRTTI::IsEqualToDescriptor(const xiiReflectedTypeDescriptor& desc)
{
  if ((desc.m_Flags.GetValue() & ~xiiTypeFlags::Phantom) != (GetTypeFlags().GetValue() & ~xiiTypeFlags::Phantom))
    return false;

  if (desc.m_sParentTypeName.IsEmpty() && GetParentType() != nullptr)
    return false;

  if (GetParentType() != nullptr && desc.m_sParentTypeName != GetParentType()->GetTypeName())
    return false;

  if (desc.m_sPluginName != GetPluginName())
    return false;

  if (desc.m_sTypeName != GetTypeName())
    return false;

  if (desc.m_Properties.GetCount() != GetProperties().GetCount())
    return false;

  for (xiiUInt32 i = 0; i < GetProperties().GetCount(); i++)
  {
    if (desc.m_Properties[i].m_Category != GetProperties()[i]->GetCategory())
      return false;

    if (desc.m_Properties[i].m_sName != GetProperties()[i]->GetPropertyName())
      return false;

    if ((desc.m_Properties[i].m_Flags.GetValue() & ~xiiPropertyFlags::Phantom) !=
        (GetProperties()[i]->GetFlags().GetValue() & ~xiiPropertyFlags::Phantom))
      return false;

    switch (desc.m_Properties[i].m_Category)
    {
      case xiiPropertyCategory::Constant:
      {
        auto pProp = (xiiPhantomConstantProperty*)GetProperties()[i];

        if (pProp->GetSpecificType() != xiiRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;

        if (pProp->GetConstant() != desc.m_Properties[i].m_ConstantValue)
          return false;
      }
      break;
      case xiiPropertyCategory::Member:
      {
        if (GetProperties()[i]->GetSpecificType() != xiiRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case xiiPropertyCategory::Array:
      {
        if (GetProperties()[i]->GetSpecificType() != xiiRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case xiiPropertyCategory::Set:
      {
        if (GetProperties()[i]->GetSpecificType() != xiiRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case xiiPropertyCategory::Map:
      {
        if (GetProperties()[i]->GetSpecificType() != xiiRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case xiiPropertyCategory::Function:
        break; // Functions handled below
    }

    if (desc.m_Functions.GetCount() != GetFunctions().GetCount())
      return false;

    for (xiiUInt32 j = 0; j < GetFunctions().GetCount(); j++)
    {
      const xiiAbstractFunctionProperty* pProp = GetFunctions()[j];
      if (desc.m_Functions[j].m_sName != pProp->GetPropertyName())
        return false;
      if ((desc.m_Functions[j].m_Flags.GetValue() & ~xiiPropertyFlags::Phantom) != (pProp->GetFlags().GetValue() & ~xiiPropertyFlags::Phantom))
        return false;
      if (desc.m_Functions[j].m_Type != pProp->GetFunctionType())
        return false;

      if (pProp->GetReturnType() != xiiRTTI::FindTypeByName(desc.m_Functions[j].m_ReturnValue.m_sType))
        return false;
      if (pProp->GetReturnFlags() != desc.m_Functions[j].m_ReturnValue.m_Flags)
        return false;
      if (desc.m_Functions[j].m_Arguments.GetCount() != pProp->GetArgumentCount())
        return false;
      for (xiiUInt32 a = 0; a < pProp->GetArgumentCount(); a++)
      {
        if (pProp->GetArgumentType(a) != xiiRTTI::FindTypeByName(desc.m_Functions[j].m_Arguments[a].m_sType))
          return false;
        if (pProp->GetArgumentFlags(a) != desc.m_Functions[j].m_Arguments[a].m_Flags)
          return false;
      }
    }

    if (desc.m_Properties[i].m_Attributes.GetCount() != GetProperties()[i]->GetAttributes().GetCount())
      return false;

    for (xiiUInt32 i2 = 0; i2 < desc.m_Properties[i].m_Attributes.GetCount(); i2++)
    {
      if (!xiiReflectionUtils::IsEqual(desc.m_Properties[i].m_Attributes[i2], GetProperties()[i]->GetAttributes()[i2]))
        return false;
    }
  }

  if (desc.m_Attributes.GetCount() != GetAttributes().GetCount())
    return false;

  // TODO: compare attribute values?
  for (xiiUInt32 i = 0; i < GetAttributes().GetCount(); i++)
  {
    if (desc.m_Attributes[i]->GetDynamicRTTI() != GetAttributes()[i]->GetDynamicRTTI())
      return false;
  }
  return true;
}
