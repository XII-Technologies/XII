/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

const char* xiiStringView::GetData(xiiStringBuilder& ref_sTempStorage) const
{
  ref_sTempStorage = *this;
  return ref_sTempStorage.GetData();
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
  if (xiiUnicodeUtils::MoveToNextUtf8(pos, GetEndPointer(), uiCharacterIndex).Failed())
    return nullptr;

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

const char* xiiStringView::FindWholeWord(const char* szSearchFor, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  XII_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, isDelimiterCB, GetEndPointer());
}

const char* xiiStringView::FindWholeWord_NoCase(const char* szSearchFor, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  XII_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return xiiStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, isDelimiterCB, GetEndPointer());
}

void xiiStringView::Shrink(xiiUInt32 uiShrinkCharsFront, xiiUInt32 uiShrinkCharsBack)
{
  const char* pEnd = m_pStart + m_uiElementCount;

  while (IsValid() && (uiShrinkCharsFront > 0))
  {
    if (xiiUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd, 1).Failed())
    {
      *this = {};
      return;
    }

    --uiShrinkCharsFront;
  }

  while (IsValid() && (uiShrinkCharsBack > 0))
  {
    if (xiiUnicodeUtils::MoveToPriorUtf8(pEnd, m_pStart, 1).Failed())
    {
      *this = {};
      return;
    }

    --uiShrinkCharsBack;
  }

  m_uiElementCount = static_cast<xiiUInt32>(pEnd - m_pStart);
}

xiiStringView xiiStringView::GetShrunk(xiiUInt32 uiShrinkCharsFront, xiiUInt32 uiShrinkCharsBack) const
{
  xiiStringView tmp = *this;
  tmp.Shrink(uiShrinkCharsFront, uiShrinkCharsBack);
  return tmp;
}

xiiStringView xiiStringView::GetSubString(xiiUInt32 uiFirstCharacter, xiiUInt32 uiNumCharacters) const
{
  if (!IsValid())
  {
    return {};
  }

  const char* pEnd = m_pStart + m_uiElementCount;

  const char* pSubStart = m_pStart;
  if (xiiUnicodeUtils::MoveToNextUtf8(pSubStart, pEnd, uiFirstCharacter).Failed() || pSubStart == pEnd)
  {
    return {};
  }

  const char* pSubEnd = pSubStart;
  xiiUnicodeUtils::MoveToNextUtf8(pSubEnd, pEnd, uiNumCharacters).IgnoreResult(); // if it fails, it just points to the end

  return xiiStringView(pSubStart, pSubEnd);
}

void xiiStringView::ChopAwayFirstCharacterUtf8()
{
  if (IsValid())
  {
    const char* pEnd = m_pStart + m_uiElementCount;
    xiiUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd, 1).AssertSuccess();
    m_uiElementCount = static_cast<xiiUInt32>(pEnd - m_pStart);
  }
}

void xiiStringView::ChopAwayFirstCharacterAscii()
{
  if (IsValid())
  {
    XII_ASSERT_DEBUG(xiiUnicodeUtils::IsASCII(*m_pStart), "ChopAwayFirstCharacterAscii() was called on a non-ASCII character.");

    m_pStart += 1;
    m_uiElementCount--;
  }
}

bool xiiStringView::TrimWordStart(xiiStringView sWord)
{
  const bool bTrimAll = false;

  bool trimmed = false;

  do
  {
    if (!sWord.IsEmpty() && StartsWith_NoCase(sWord))
    {
      Shrink(xiiStringUtils::GetCharacterCount(sWord.GetStartPointer(), sWord.GetEndPointer()), 0);
      trimmed = true;
    }

  } while (bTrimAll);

  return trimmed;
}

bool xiiStringView::TrimWordEnd(xiiStringView sWord)
{
  const bool bTrimAll = false;

  bool trimmed = false;

  do
  {
    if (!sWord.IsEmpty() && EndsWith_NoCase(sWord))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(sWord.GetStartPointer(), sWord.GetEndPointer()));
      trimmed = true;
    }

  } while (bTrimAll);

  return trimmed;
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

xiiStringView xiiStringView::GetFileExtension(bool bFullExtension /*= false*/) const
{
  return xiiPathUtils::GetFileExtension(*this, bFullExtension);
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

#if XII_ENABLED(XII_INTEROP_STL_STRINGS)
xiiStringView::xiiStringView(const std::string_view& rhs)
{
  if (!rhs.empty())
  {
    m_pStart         = rhs.data();
    m_uiElementCount = static_cast<xiiUInt32>(rhs.size());
  }
}

xiiStringView::xiiStringView(const std::string& rhs)
{
  if (!rhs.empty())
  {
    m_pStart         = rhs.data();
    m_uiElementCount = static_cast<xiiUInt32>(rhs.size());
  }
}

std::string_view xiiStringView::GetAsStdView() const
{
  return std::string_view(m_pStart, static_cast<size_t>(m_uiElementCount));
}

xiiStringView::operator std::string_view() const
{
  return GetAsStdView();
}
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringView);
