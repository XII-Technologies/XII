#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

xiiMap<xiiString, xiiDynamicStringEnum>                            xiiDynamicStringEnum::s_DynamicEnums;
xiiDelegate<void(const char* szEnumName, xiiDynamicStringEnum& e)> xiiDynamicStringEnum::s_RequestUnknownCallback;

void xiiDynamicStringEnum::RemoveEnum(const char* szEnumName)
{
  s_DynamicEnums.Remove(szEnumName);
}

void xiiDynamicStringEnum::Clear()
{
  m_ValidValues.Clear();
}

void xiiDynamicStringEnum::AddValidValue(const char* szNewName, bool bSortValues /*= false*/)
{
  xiiString sName = szNewName;

  if (!m_ValidValues.Contains(sName))
    m_ValidValues.PushBack(sName);

  if (bSortValues)
    SortValues();
}

void xiiDynamicStringEnum::RemoveValue(const char* szValue)
{
  m_ValidValues.RemoveAndCopy(szValue);
}

bool xiiDynamicStringEnum::IsValueValid(const char* szValue) const
{
  return m_ValidValues.Contains(szValue);
}

void xiiDynamicStringEnum::SortValues()
{
  m_ValidValues.Sort();
}

xiiDynamicStringEnum& xiiDynamicStringEnum::GetDynamicEnum(const char* szEnumName)
{
  bool bExisted = false;
  auto it       = s_DynamicEnums.FindOrAdd(szEnumName, &bExisted);

  if (!bExisted && s_RequestUnknownCallback.IsValid())
  {
    s_RequestUnknownCallback(szEnumName, it.Value());
  }

  return it.Value();
}

xiiDynamicStringEnum& xiiDynamicStringEnum::CreateDynamicEnum(const char* szEnumName)
{
  bool bExisted = false;
  auto it       = s_DynamicEnums.FindOrAdd(szEnumName, &bExisted);

  xiiDynamicStringEnum& e = it.Value();
  e.Clear();
  e.SetStorageFile(nullptr);

  return e;
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
