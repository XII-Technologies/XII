#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicBitflags.h>

xiiMap<xiiString, xiiDynamicBitflags> xiiDynamicBitflags::s_DynamicBitflags;

void xiiDynamicBitflags::Clear()
{
  m_ValidValues.Clear();
}

void xiiDynamicBitflags::SetValueAndName(xiiUInt32 uiBitPos, xiiStringView sName)
{
  XII_ASSERT_DEV(uiBitPos < 64, "Only up to 64 bits is supported.");
  auto it    = m_ValidValues.FindOrAdd(XII_BIT(uiBitPos));
  it.Value() = sName;
}

void xiiDynamicBitflags::RemoveValue(xiiUInt32 uiBitPos)
{
  XII_ASSERT_DEV(uiBitPos < 64, "Only up to 64 bits is supported.");
  m_ValidValues.Remove(XII_BIT(uiBitPos));
}

bool xiiDynamicBitflags::IsValueValid(xiiUInt32 uiBitPos) const
{
  return m_ValidValues.Find(XII_BIT(uiBitPos)).IsValid();
}

bool xiiDynamicBitflags::TryGetValueName(xiiUInt32 uiBitPos, xiiStringView& out_sName) const
{
  auto it = m_ValidValues.Find(XII_BIT(uiBitPos));
  if (it.IsValid())
  {
    out_sName = it.Value();
    return true;
  }
  return false;
}

xiiDynamicBitflags& xiiDynamicBitflags::GetDynamicBitflags(xiiStringView sName)
{
  return s_DynamicBitflags[sName];
}
