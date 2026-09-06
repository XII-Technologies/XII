/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Strings/StringConversion.h>

inline xiiStringBuilder::xiiStringBuilder(xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  AppendTerminator();
}

inline xiiStringBuilder::xiiStringBuilder(const xiiStringBuilder& rhs) :
  m_Data(rhs.GetAllocator())
{
  AppendTerminator();

  *this = rhs;
}

inline xiiStringBuilder::xiiStringBuilder(xiiStringBuilder&& rhs) noexcept :
  m_Data(rhs.GetAllocator())
{
  AppendTerminator();

  *this = std::move(rhs);
}

inline xiiStringBuilder::xiiStringBuilder(const char* szUTF8, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  AppendTerminator();

  *this = szUTF8;
}

inline xiiStringBuilder::xiiStringBuilder(const wchar_t* pWChar, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  AppendTerminator();

  *this = pWChar;
}

inline xiiStringBuilder::xiiStringBuilder(xiiStringView rhs, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  AppendTerminator();

  *this = rhs;
}

XII_ALWAYS_INLINE xiiAllocator* xiiStringBuilder::GetAllocator() const
{
  return m_Data.GetAllocator();
}

XII_ALWAYS_INLINE void xiiStringBuilder::operator=(const char* szUTF8)
{
  Set(szUTF8);
}

XII_FORCE_INLINE void xiiStringBuilder::operator=(const wchar_t* pWChar)
{
  // fine to do this, szWChar can never come from the stringbuilder's own data array
  Clear();
  Append(pWChar);
}

XII_ALWAYS_INLINE void xiiStringBuilder::operator=(const xiiStringBuilder& rhs)
{
  m_Data = rhs.m_Data;
}

XII_ALWAYS_INLINE void xiiStringBuilder::operator=(xiiStringBuilder&& rhs) noexcept
{
  m_Data = std::move(rhs.m_Data);
}

XII_ALWAYS_INLINE xiiUInt32 xiiStringBuilder::GetElementCount() const
{
  return m_Data.GetCount() - 1; // exclude the '\0' terminator
}

XII_ALWAYS_INLINE xiiUInt32 xiiStringBuilder::GetCharacterCount() const
{
  return xiiStringUtils::GetCharacterCount(m_Data.GetData());
}

XII_FORCE_INLINE void xiiStringBuilder::Clear()
{
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

inline void xiiStringBuilder::Append(xiiUInt32 uiChar)
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
}

inline void xiiStringBuilder::Prepend(xiiUInt32 uiChar)
{
  char  szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar     = &szChar[0];

  xiiUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  Prepend(szChar);
}

inline void xiiStringBuilder::Append(const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4, const wchar_t* pData5, const wchar_t* pData6)
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

inline void xiiStringBuilder::Prepend(const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4, const wchar_t* pData5, const wchar_t* pData6)
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

inline void xiiStringBuilder::AppendTerminator()
{
  // make sure the string terminates with a zero.
  if (m_Data.IsEmpty() || (m_Data.PeekBack() != '\0'))
    m_Data.PushBack('\0');
}

inline void xiiStringBuilder::ToUpper()
{
  const xiiUInt32 uiNewStringLength = xiiStringUtils::ToUpperString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

inline void xiiStringBuilder::ToLower()
{
  const xiiUInt32 uiNewStringLength = xiiStringUtils::ToLowerString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

inline void xiiStringBuilder::ChangeCharacter(iterator& ref_it, xiiUInt32 uiCharacter)
{
  XII_ASSERT_DEV(ref_it.IsValid(), "The given character iterator does not point to a valid character.");
  XII_ASSERT_DEV(ref_it.GetData() >= GetData() && ref_it.GetData() < GetData() + GetElementCount(), "The given character iterator does not point into this string. It was either created from another string, or this string has been reallocated in the mean time.");

  // This is only an optimization for pure ASCII strings. Without it, the code below would still work.
  if (xiiUnicodeUtils::IsASCII(*ref_it) && xiiUnicodeUtils::IsASCII(uiCharacter))
  {
    char* pPos = const_cast<char*>(ref_it.GetData()); // yes, I know...
    *pPos      = uiCharacter & 0xFF;
    return;
  }

  ChangeCharacterNonASCII(ref_it, uiCharacter);
}

XII_ALWAYS_INLINE void xiiStringBuilder::Reserve(xiiUInt32 uiNumElements)
{
  m_Data.Reserve(uiNumElements);
}

XII_ALWAYS_INLINE void xiiStringBuilder::Insert(const char* szInsertAtPos, xiiStringView sTextToInsert)
{
  ReplaceSubString(szInsertAtPos, szInsertAtPos, sTextToInsert);
}

XII_ALWAYS_INLINE void xiiStringBuilder::Remove(const char* szRemoveFromPos, const char* szRemoveToPos)
{
  ReplaceSubString(szRemoveFromPos, szRemoveToPos, xiiStringView());
}

template <typename Container>
bool xiiUnicodeUtils::RepairNonUtf8Text(const char* pStartData, const char* pEndData, Container& out_result)
{
  if (xiiUnicodeUtils::IsValidUtf8(pStartData, pEndData))
  {
    out_result = xiiStringView(pStartData, pEndData);
    return false;
  }

  out_result.Clear();

  xiiHybridArray<char, 1024>                              fixedText;
  xiiUnicodeUtils::UtfInserter<char, decltype(fixedText)> inserter(&fixedText);

  while (pStartData < pEndData)
  {
    const xiiUInt32 uiChar = xiiUnicodeUtils::DecodeUtf8ToUtf32(pStartData);
    xiiUnicodeUtils::EncodeUtf32ToUtf8(uiChar, inserter);
  }

  XII_ASSERT_DEV(xiiUnicodeUtils::IsValidUtf8(fixedText.GetData(), fixedText.GetData() + fixedText.GetCount()), "Repaired text is still not a valid Utf8 string.");

  out_result = xiiStringView(fixedText.GetData(), fixedText.GetCount());
  return true;
}

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
