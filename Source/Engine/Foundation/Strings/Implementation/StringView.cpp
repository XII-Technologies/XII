#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

xiiUInt32 xiiStringView::GetCharacter() const
{
  if (!IsValid())
    return 0;

  return xiiUnicodeUtils::ConvertUtf8ToUtf32(m_pStart);
}

const char* xiiStringView::GetData(xiiStringBuilder& tempStorage) const
{
  tempStorage = *this;
  return tempStorage.GetData();
}

bool xiiStringView::IsEqualN(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const
{
  return xiiStringUtils::IsEqualN(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

bool xiiStringView::IsEqualN_NoCase(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const
{
  return xiiStringUtils::IsEqualN_NoCase(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

xiiInt32 xiiStringView::Compare(xiiStringView sOther) const
{
  return xiiStringUtils::Compare(GetStartPointer(), sOther.GetStartPointer(), GetEndPointer(), sOther.GetEndPointer());
}

xiiInt32 xiiStringView::CompareN(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const
{
  return xiiStringUtils::CompareN(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

xiiInt32 xiiStringView::Compare_NoCase(xiiStringView sOther) const
{
  return xiiStringUtils::Compare_NoCase(GetStartPointer(), sOther.GetStartPointer(), GetEndPointer(), sOther.GetEndPointer());
}

xiiInt32 xiiStringView::CompareN_NoCase(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const
{
  return xiiStringUtils::CompareN_NoCase(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

const char* xiiStringView::ComputeCharacterPosition(xiiUInt32 uiCharacterIndex) const
{
  const char* pos = GetStartPointer();
  xiiUnicodeUtils::MoveToNextUtf8(pos, GetEndPointer(), uiCharacterIndex);
  return pos;
}

const char* xiiStringView::FindSubString(xiiStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  XII_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindSubString(szStartSearchAt, sStringToFind.GetStartPointer(), GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* xiiStringView::FindSubString_NoCase(xiiStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  XII_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindSubString_NoCase(szStartSearchAt, sStringToFind.GetStartPointer(), GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* xiiStringView::FindLastSubString(xiiStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetEndPointer();

  XII_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindLastSubString(GetStartPointer(), sStringToFind.GetStartPointer(), szStartSearchAt, GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* xiiStringView::FindLastSubString_NoCase(xiiStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetEndPointer();

  XII_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindLastSubString_NoCase(GetStartPointer(), sStringToFind.GetStartPointer(), szStartSearchAt, GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* xiiStringView::FindWholeWord(const char* szSearchFor, xiiStringUtils::XII_CHARACTER_FILTER IsDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  XII_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, IsDelimiterCB, GetEndPointer());
}

const char* xiiStringView::FindWholeWord_NoCase(const char* szSearchFor, xiiStringUtils::XII_CHARACTER_FILTER IsDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  XII_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, IsDelimiterCB, GetEndPointer());
}

void xiiStringView::Shrink(xiiUInt32 uiShrinkCharsFront, xiiUInt32 uiShrinkCharsBack)
{
  while (IsValid() && (uiShrinkCharsFront > 0))
  {
    xiiUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd, 1);
    --uiShrinkCharsFront;
  }

  while (IsValid() && (uiShrinkCharsBack > 0))
  {
    xiiUnicodeUtils::MoveToPriorUtf8(m_pEnd, 1);
    --uiShrinkCharsBack;
  }
}

xiiStringView xiiStringView::GetShrunk(xiiUInt32 uiShrinkCharsFront, xiiUInt32 uiShrinkCharsBack) const
{
  xiiStringView tmp = *this;
  tmp.Shrink(uiShrinkCharsFront, uiShrinkCharsBack);
  return tmp;
}

bool xiiStringView::TrimWordStart(xiiStringView sWord1, xiiStringView sWord2, xiiStringView sWord3, xiiStringView sWord4, xiiStringView sWord5)
{
  /// \test TrimWordStart
  bool trimmed = false;

  while (true)
  {
    if (!sWord1.IsEmpty() && StartsWith_NoCase(sWord1))
    {
      Shrink(xiiStringUtils::GetCharacterCount(sWord1.GetStartPointer(), sWord1.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    if (!sWord2.IsEmpty() && StartsWith_NoCase(sWord2))
    {
      Shrink(xiiStringUtils::GetCharacterCount(sWord2.GetStartPointer(), sWord2.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    if (!sWord3.IsEmpty() && StartsWith_NoCase(sWord3))
    {
      Shrink(xiiStringUtils::GetCharacterCount(sWord3.GetStartPointer(), sWord3.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    if (!sWord4.IsEmpty() && StartsWith_NoCase(sWord4))
    {
      Shrink(xiiStringUtils::GetCharacterCount(sWord4.GetStartPointer(), sWord4.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    if (!sWord5.IsEmpty() && StartsWith_NoCase(sWord5))
    {
      Shrink(xiiStringUtils::GetCharacterCount(sWord5.GetStartPointer(), sWord5.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    return trimmed;
  }
}

bool xiiStringView::TrimWordEnd(xiiStringView sWord1, xiiStringView sWord2, xiiStringView sWord3, xiiStringView sWord4, xiiStringView sWord5)
{
  /// \test TrimWordEnd

  bool trimmed = false;

  while (true)
  {
    if (!sWord1.IsEmpty() && EndsWith_NoCase(sWord1))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(sWord1.GetStartPointer(), sWord1.GetEndPointer()));
      trimmed = true;
      continue;
    }

    if (!sWord2.IsEmpty() && EndsWith_NoCase(sWord2))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(sWord2.GetStartPointer(), sWord2.GetEndPointer()));
      trimmed = true;
      continue;
    }

    if (!sWord3.IsEmpty() && EndsWith_NoCase(sWord3))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(sWord3.GetStartPointer(), sWord3.GetEndPointer()));
      trimmed = true;
      continue;
    }

    if (!sWord4.IsEmpty() && EndsWith_NoCase(sWord4))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(sWord4.GetStartPointer(), sWord4.GetEndPointer()));
      trimmed = true;
      continue;
    }

    if (!sWord5.IsEmpty() && EndsWith_NoCase(sWord5))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(sWord5.GetStartPointer(), sWord5.GetEndPointer()));
      trimmed = true;
      continue;
    }

    return trimmed;
  }
}

xiiStringView::iterator xiiStringView::GetIteratorFront() const
{
  return begin(*this);
}

xiiStringView::reverse_iterator xiiStringView::GetIteratorBack() const
{
  return rbegin(*this);
}

bool xiiStringView::HasAnyExtension() const
{
  return xiiPathUtils::HasAnyExtension(*this);
}

bool xiiStringView::HasExtension(xiiStringView sExtension) const
{
  return xiiPathUtils::HasExtension(*this, sExtension);
}

xiiStringView xiiStringView::GetFileExtension() const
{
  return xiiPathUtils::GetFileExtension(*this);
}

xiiStringView xiiStringView::GetFileName() const
{
  return xiiPathUtils::GetFileName(*this);
}

xiiStringView xiiStringView::GetFileNameAndExtension() const
{
  return xiiPathUtils::GetFileNameAndExtension(*this);
}

xiiStringView xiiStringView::GetFileDirectory() const
{
  return xiiPathUtils::GetFileDirectory(*this);
}

bool xiiStringView::IsAbsolutePath() const
{
  return xiiPathUtils::IsAbsolutePath(*this);
}

bool xiiStringView::IsRelativePath() const
{
  return xiiPathUtils::IsRelativePath(*this);
}

bool xiiStringView::IsRootedPath() const
{
  return xiiPathUtils::IsRootedPath(*this);
}

xiiStringView xiiStringView::GetRootedPathRootName() const
{
  return xiiPathUtils::GetRootedPathRootName(*this);
}

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringView);
