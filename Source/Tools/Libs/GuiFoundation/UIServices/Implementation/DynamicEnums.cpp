#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicEnums.h>

xiiMap<xiiString, xiiDynamicEnum> xiiDynamicEnum::s_DynamicEnums;

void xiiDynamicEnum::Clear()
{
  m_ValidValues.Clear();
}

void xiiDynamicEnum::SetValueAndName(xiiInt32 iValue, const char* szNewName)
{
  m_ValidValues[iValue] = szNewName;
}

void xiiDynamicEnum::RemoveValue(xiiInt32 iValue)
{
  m_ValidValues.Remove(iValue);
}

bool xiiDynamicEnum::IsValueValid(xiiInt32 iValue) const
{
  return m_ValidValues.Find(iValue).IsValid();
}

const char* xiiDynamicEnum::GetValueName(xiiInt32 iValue) const
{
  auto it = m_ValidValues.Find(iValue);

  if (!it.IsValid())
    return "<invalid value>";

  return it.Value();
}

xiiDynamicEnum& xiiDynamicEnum::GetDynamicEnum(const char* szEnumName)
{
  return s_DynamicEnums[szEnumName];
}
