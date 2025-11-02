#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

xiiMap<xiiString, xiiDynamicStringEnum>                             xiiDynamicStringEnum::s_DynamicEnums;
xiiDelegate<void(xiiStringView sEnumName, xiiDynamicStringEnum& e)> xiiDynamicStringEnum::s_RequestUnknownCallback;

// static
xiiDynamicStringEnum& xiiDynamicStringEnum::GetDynamicEnum(xiiStringView sEnumName)
{
  bool bExisted = false;
  auto it       = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  if (!bExisted && s_RequestUnknownCallback.IsValid())
  {
    s_RequestUnknownCallback(sEnumName, it.Value());
  }

  return it.Value();
}

// static
xiiDynamicStringEnum& xiiDynamicStringEnum::CreateDynamicEnum(xiiStringView sEnumName)
{
  bool bExisted = false;
  auto it       = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  xiiDynamicStringEnum& e = it.Value();
  e.Clear();
  e.SetStorageFile(nullptr);

  return e;
}

// static
void xiiDynamicStringEnum::RemoveEnum(xiiStringView sEnumName)
{
  s_DynamicEnums.Remove(sEnumName);
}

void xiiDynamicStringEnum::Clear()
{
  m_ValidValues.Clear();
}

void xiiDynamicStringEnum::AddValidValue(xiiStringView sValue, bool bSortValues /*= false*/)
{
  xiiString sNewValue = sValue;

  if (!m_ValidValues.Contains(sNewValue))
    m_ValidValues.PushBack(sNewValue);

  if (bSortValues)
    SortValues();
}

void xiiDynamicStringEnum::RemoveValue(xiiStringView sValue)
{
  m_ValidValues.RemoveAndCopy(sValue);
}

bool xiiDynamicStringEnum::IsValueValid(xiiStringView sValue) const
{
  return m_ValidValues.Contains(sValue);
}

void xiiDynamicStringEnum::SortValues()
{
  xiiCompareString_NoCase comp;
  m_ValidValues.Sort(comp);
}

void xiiDynamicStringEnum::SetEditCommand(xiiStringView sCmd, const xiiVariant& value)
{
  m_sEditCommand     = sCmd;
  m_EditCommandValue = value;
}

void xiiDynamicStringEnum::ReadFromStorage()
{
  Clear();

  xiiStringBuilder sFile, tmp;

  xiiFileReader file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  sFile.ReadAll(file);

  xiiHybridArray<xiiStringView, 32> values;

  sFile.Split(false, values, "\n", "\r");

  for (auto val : values)
  {
    AddValidValue(val.GetData(tmp));
  }
}

void xiiDynamicStringEnum::SaveToStorage()
{
  if (m_sStorageFile.IsEmpty())
    return;

  xiiFileWriter file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  xiiStringBuilder tmp;

  for (const auto& val : m_ValidValues)
  {
    tmp.Set(val, "\n");
    file.WriteBytes(tmp.GetData(), tmp.GetElementCount()).IgnoreResult();
  }
}
