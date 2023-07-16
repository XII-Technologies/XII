#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Reflection/PhantomProperty.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

xiiPhantomConstantProperty::xiiPhantomConstantProperty(const xiiReflectedPropertyDescriptor* pDesc) :
  xiiAbstractConstantProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName       = m_sPropertyNameStorage.GetData();
  m_Value                = pDesc->m_ConstantValue;
  m_pPropertyType        = xiiRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(xiiPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

xiiPhantomConstantProperty::~xiiPhantomConstantProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const xiiRTTI* xiiPhantomConstantProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

void* xiiPhantomConstantProperty::GetPropertyPointer() const
{
  return nullptr;
}



xiiPhantomMemberProperty::xiiPhantomMemberProperty(const xiiReflectedPropertyDescriptor* pDesc) :
  xiiAbstractMemberProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName       = m_sPropertyNameStorage.GetData();
  m_pPropertyType        = xiiRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(xiiPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

xiiPhantomMemberProperty::~xiiPhantomMemberProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const xiiRTTI* xiiPhantomMemberProperty::GetSpecificType() const
{
  return m_pPropertyType;
}



xiiPhantomFunctionProperty::xiiPhantomFunctionProperty(xiiReflectedFunctionDescriptor* pDesc) :
  xiiAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName       = m_sPropertyNameStorage.GetData();
  m_FunctionType         = pDesc->m_Type;
  m_Flags                = pDesc->m_Flags;
  m_Flags.Add(xiiPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();

  m_ReturnValue = pDesc->m_ReturnValue;
  m_Arguments.Swap(pDesc->m_Arguments);
}



xiiPhantomFunctionProperty::~xiiPhantomFunctionProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

xiiFunctionType::Enum xiiPhantomFunctionProperty::GetFunctionType() const
{
  return m_FunctionType;
}

const xiiRTTI* xiiPhantomFunctionProperty::GetReturnType() const
{
  return xiiRTTI::FindTypeByName(m_ReturnValue.m_sType);
}

xiiBitflags<xiiPropertyFlags> xiiPhantomFunctionProperty::GetReturnFlags() const
{
  return m_ReturnValue.m_Flags;
}

xiiUInt32 xiiPhantomFunctionProperty::GetArgumentCount() const
{
  return m_Arguments.GetCount();
}

const xiiRTTI* xiiPhantomFunctionProperty::GetArgumentType(xiiUInt32 uiParamIndex) const
{
  return xiiRTTI::FindTypeByName(m_Arguments[uiParamIndex].m_sType);
}

xiiBitflags<xiiPropertyFlags> xiiPhantomFunctionProperty::GetArgumentFlags(xiiUInt32 uiParamIndex) const
{
  return m_Arguments[uiParamIndex].m_Flags;
}

void xiiPhantomFunctionProperty::Execute(void* pInstance, xiiArrayPtr<xiiVariant> values, xiiVariant& ref_returnValue) const
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

xiiPhantomArrayProperty::xiiPhantomArrayProperty(const xiiReflectedPropertyDescriptor* pDesc) :
  xiiAbstractArrayProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName       = m_sPropertyNameStorage.GetData();
  m_pPropertyType        = xiiRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(xiiPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

xiiPhantomArrayProperty::~xiiPhantomArrayProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const xiiRTTI* xiiPhantomArrayProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

xiiPhantomSetProperty::xiiPhantomSetProperty(const xiiReflectedPropertyDescriptor* pDesc) :
  xiiAbstractSetProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName       = m_sPropertyNameStorage.GetData();
  m_pPropertyType        = xiiRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(xiiPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

xiiPhantomSetProperty::~xiiPhantomSetProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const xiiRTTI* xiiPhantomSetProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

xiiPhantomMapProperty::xiiPhantomMapProperty(const xiiReflectedPropertyDescriptor* pDesc) :
  xiiAbstractMapProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName       = m_sPropertyNameStorage.GetData();
  m_pPropertyType        = xiiRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(xiiPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

xiiPhantomMapProperty::~xiiPhantomMapProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const xiiRTTI* xiiPhantomMapProperty::GetSpecificType() const
{
  return m_pPropertyType;
}
