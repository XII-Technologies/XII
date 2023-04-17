#pragma once

#include <Foundation/Strings/StringConversion.h>

XII_FORCE_INLINE xiiStringBuilder::xiiStringBuilder(xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();
}

XII_FORCE_INLINE xiiStringBuilder::xiiStringBuilder(const xiiStringBuilder& rhs) :
  m_Data(rhs.GetAllocator())
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

XII_FORCE_INLINE xiiStringBuilder::xiiStringBuilder(xiiStringBuilder&& rhs) noexcept
  :
  m_Data(rhs.GetAllocator())
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = std::move(rhs);
}

XII_FORCE_INLINE xiiStringBuilder::xiiStringBuilder(const char* szUTF8, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = szUTF8;
}

XII_FORCE_INLINE xiiStringBuilder::xiiStringBuilder(const wchar_t* szWChar, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = szWChar;
}

XII_FORCE_INLINE xiiStringBuilder::xiiStringBuilder(xiiStringView rhs, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

XII_ALWAYS_INLINE xiiAllocatorBase* xiiStringBuilder::GetAllocator() const
{
  return m_Data.GetAllocator();
}

XII_ALWAYS_INLINE void xiiStringBuilder::operator=(const char* szUTF8)
{
  Set(szUTF8);
}

XII_FORCE_INLINE void xiiStringBuilder::operator=(const wchar_t* szWChar)
{
  // fine to do this, szWChar can never come from the stringbuilder's own data array
  Clear();
  Append(szWChar);
}

XII_ALWAYS_INLINE void xiiStringBuilder::operator=(const xiiStringBuilder& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data             = rhs.m_Data;
}

XII_ALWAYS_INLINE void xiiStringBuilder::operator=(xiiStringBuilder&& rhs) noexcept
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data             = std::move(rhs.m_Data);
}

XII_ALWAYS_INLINE xiiUInt32 xiiStringBuilder::GetElementCount() const
{
  return m_Data.GetCount() - 1; // exclude the '\0' terminator
}

XII_ALWAYS_INLINE xiiUInt32 xiiStringBuilder::GetCharacterCount() const
{
  return m_uiCharacterCount;
}

XII_FORCE_INLINE void xiiStringBuilder::Clear()
{
  m_uiCharacterCount = 0;
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

XII_FORCE_INLINE void xiiStringBuilder::Append(xiiUInt32 uiChar)
{
  char  szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar     = &szChar[0];

  xiiUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  xiiUInt32 uiCharLen  = (xiiUInt32)(pChar - szChar);
  xiiUInt32 uiOldCount = m_Data.GetCount();
  m_Data.SetCountUninitialized(uiOldCount + uiCharLen);
  uiOldCount--;
  for (xiiUInt32 i = 0; i < uiCharLen; i++)
  {
    m_Data[uiOldCount + i] = szChar[i];
  }
  m_Data[uiOldCount + uiCharLen] = '\0';
  ++m_uiCharacterCount;
}

XII_FORCE_INLINE void xiiStringBuilder::Prepend(xiiUInt32 uiChar)
{
  char  szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar     = &szChar[0];

  xiiUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  Prepend(szChar);
}

XII_FORCE_INLINE void xiiStringBuilder::Append(
  const wchar_t* pData1,
  const wchar_t* pData2,
  const wchar_t* pData3,
  const wchar_t* pData4,
  const wchar_t* pData5,
  const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitly
  xiiStringUtf8 s1(pData1, m_Data.GetAllocator());
  xiiStringUtf8 s2(pData2, m_Data.GetAllocator());
  xiiStringUtf8 s3(pData3, m_Data.GetAllocator());
  xiiStringUtf8 s4(pData4, m_Data.GetAllocator());
  xiiStringUtf8 s5(pData5, m_Data.GetAllocator());
  xiiStringUtf8 s6(pData6, m_Data.GetAllocator());

  Append(s1.GetView(), s2.GetView(), s3.GetView(), s4.GetView(), s5.GetView(), s6.GetView());
}

XII_FORCE_INLINE void xiiStringBuilder::Prepend(
  const wchar_t* pData1,
  const wchar_t* pData2,
  const wchar_t* pData3,
  const wchar_t* pData4,
  const wchar_t* pData5,
  const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitly
  xiiStringUtf8 s1(pData1, m_Data.GetAllocator());
  xiiStringUtf8 s2(pData2, m_Data.GetAllocator());
  xiiStringUtf8 s3(pData3, m_Data.GetAllocator());
  xiiStringUtf8 s4(pData4, m_Data.GetAllocator());
  xiiStringUtf8 s5(pData5, m_Data.GetAllocator());
  xiiStringUtf8 s6(pData6, m_Data.GetAllocator());

  Prepend(s1.GetView(), s2.GetView(), s3.GetView(), s4.GetView(), s5.GetView(), s6.GetView());
}

XII_ALWAYS_INLINE const char* xiiStringBuilder::GetData() const
{
  XII_ASSERT_DEBUG(!m_Data.IsEmpty(), "xiiStringBuilder has been corrupted, the array can never be empty.");

  return &m_Data[0];
}

XII_FORCE_INLINE void xiiStringBuilder::AppendTerminator()
{
  // make sure the string terminates with a zero.
  if (m_Data.IsEmpty() || (m_Data.PeekBack() != '\0'))
    m_Data.PushBack('\0');
}

XII_FORCE_INLINE void xiiStringBuilder::ToUpper()
{
  const xiiUInt32 uiNewStringLength = xiiStringUtils::ToUpperString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

XII_FORCE_INLINE void xiiStringBuilder::ToLower()
{
  const xiiUInt32 uiNewStringLength = xiiStringUtils::ToLowerString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

XII_FORCE_INLINE void xiiStringBuilder::ChangeCharacter(iterator& it, xiiUInt32 uiCharacter)
{
  XII_ASSERT_DEV(it.IsValid(), "The given character iterator does not point to a valid character.");
  XII_ASSERT_DEV(it.GetData() >= GetData() && it.GetData() < GetData() + GetElementCount(),
                 "The given character iterator does not point into this string. It was either created from another string, or this string "
                 "has been reallocated in the mean time.");

  // this is only an optimization for pure ASCII strings
  // without it, the code below would still work
  if (xiiUnicodeUtils::IsASCII(*it) && xiiUnicodeUtils::IsASCII(uiCharacter))
  {
    char* pPos = const_cast<char*>(it.GetData()); // yes, I know...
    *pPos      = uiCharacter & 0xFF;
    return;
  }

  ChangeCharacterNonASCII(it, uiCharacter);
}

XII_ALWAYS_INLINE bool xiiStringBuilder::IsPureASCII() const
{
  return m_uiCharacterCount + 1 == m_Data.GetCount();
}

XII_ALWAYS_INLINE void xiiStringBuilder::Reserve(xiiUInt32 uiNumElements)
{
  m_Data.Reserve(uiNumElements);
}

XII_ALWAYS_INLINE void xiiStringBuilder::Insert(const char* szInsertAtPos, xiiStringView szTextToInsert)
{
  ReplaceSubString(szInsertAtPos, szInsertAtPos, szTextToInsert);
}

XII_ALWAYS_INLINE void xiiStringBuilder::Remove(const char* szRemoveFromPos, const char* szRemoveToPos)
{
  ReplaceSubString(szRemoveFromPos, szRemoveToPos, xiiStringView());
}

template <typename Container>
bool xiiUnicodeUtils::RepairNonUtf8Text(const char* pStartData, const char* pEndData, Container& out_Result)
{
  if (xiiUnicodeUtils::IsValidUtf8(pStartData, pEndData))
  {
    out_Result = xiiStringView(pStartData, pEndData);
    return false;
  }

  out_Result.Clear();

  xiiHybridArray<char, 1024>                              fixedText;
  xiiUnicodeUtils::UtfInserter<char, decltype(fixedText)> inserter(&fixedText);

  while (pStartData < pEndData)
  {
    const xiiUInt32 uiChar = xiiUnicodeUtils::DecodeUtf8ToUtf32(pStartData);
    xiiUnicodeUtils::EncodeUtf32ToUtf8(uiChar, inserter);
  }

  XII_ASSERT_DEV(xiiUnicodeUtils::IsValidUtf8(fixedText.GetData(), fixedText.GetData() + fixedText.GetCount()), "Repaired text is still not a valid Utf8 string.");

  out_Result = xiiStringView(fixedText.GetData(), fixedText.GetCount());
  return true;
}

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
