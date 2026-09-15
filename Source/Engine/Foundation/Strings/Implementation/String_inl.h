/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  Clear();
}

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(const xiiHybridStringBase& rhs, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(xiiHybridStringBase&& rhs, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  operator=(std::move(rhs));
}

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(const char* rhs, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(const wchar_t* rhs, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(const xiiStringView& rhs, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::~xiiHybridStringBase() = default;

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::Clear()
{
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

template <xiiUInt16 Size>
XII_ALWAYS_INLINE const char* xiiHybridStringBase<Size>::GetData() const
{
  XII_ASSERT_DEBUG(!m_Data.IsEmpty(), "xiiHybridString has been corrupted, the array can never be empty. This can happen when you access a string that was previously std::move'd into another string.");

  return &m_Data[0];
}

template <xiiUInt16 Size>
XII_ALWAYS_INLINE xiiUInt32 xiiHybridStringBase<Size>::GetElementCount() const
{
  return m_Data.GetCount() - 1;
}

template <xiiUInt16 Size>
XII_ALWAYS_INLINE xiiUInt32 xiiHybridStringBase<Size>::GetCharacterCount() const
{
  return xiiStringUtils::GetCharacterCount(GetData());
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(const char* szString)
{
  xiiUInt32 uiElementCount = xiiStringUtils::GetStringElementCount(szString);

  if (szString + uiElementCount < m_Data.GetData() || szString >= m_Data.GetData() + m_Data.GetCount())
  {
    // source string is outside our own memory, so no overlapped copy
  }
  else
  {
    // source string overlaps with our own memory -> we can't increase the size of our memory, as that might invalidate the source data
    XII_ASSERT_DEBUG(uiElementCount < m_Data.GetCount(), "Invalid copy of overlapping string data.");
  }

  m_Data.SetCountUninitialized(uiElementCount + 1);
  xiiStringUtils::Copy(&m_Data[0], uiElementCount + 1, szString);
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(const xiiHybridStringBase& rhs)
{
  if (this == &rhs)
    return;

  m_Data = rhs.m_Data;
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(xiiHybridStringBase&& rhs)
{
  if (this == &rhs)
    return;

  m_Data = std::move(rhs.m_Data);
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(const wchar_t* szString)
{
  xiiStringUtf8 sConversion(szString, m_Data.GetAllocator());
  *this = sConversion.GetData();
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(const xiiStringView& rhs)
{
  XII_ASSERT_DEBUG(rhs.GetStartPointer() < m_Data.GetData() || rhs.GetStartPointer() >= m_Data.GetData() + m_Data.GetCount(), "Can't assign string a value that points to ourself!");

  m_Data.SetCountUninitialized(rhs.GetElementCount() + 1);
  xiiStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.GetStartPointer(), rhs.GetEndPointer());
}

template <xiiUInt16 Size>
xiiStringView xiiHybridStringBase<Size>::GetSubString(xiiUInt32 uiFirstCharacter, xiiUInt32 uiNumCharacters) const
{
  const char* szStart = GetData();
  if (xiiUnicodeUtils::MoveToNextUtf8(szStart, uiFirstCharacter).Failed())
    return {}; // szStart was moved too far, the result is just an empty string

  const char* szEnd = szStart;
  xiiUnicodeUtils::MoveToNextUtf8(szEnd, uiNumCharacters).IgnoreResult(); // if it fails, szEnd just points to the end of this string

  return xiiStringView(szStart, szEnd);
}

template <xiiUInt16 Size>
xiiStringView xiiHybridStringBase<Size>::GetFirst(xiiUInt32 uiNumCharacters) const
{
  return GetSubString(0, uiNumCharacters);
}

template <xiiUInt16 Size>
xiiStringView xiiHybridStringBase<Size>::GetLast(xiiUInt32 uiNumCharacters) const
{
  const xiiUInt32 uiMaxCharacterCount = GetCharacterCount();
  XII_ASSERT_DEV(uiNumCharacters < uiMaxCharacterCount, "The string only contains {0} characters, cannot return the last {1} characters.", uiMaxCharacterCount, uiNumCharacters);
  return GetSubString(uiMaxCharacterCount - uiNumCharacters, uiNumCharacters);
}


template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString() :
  xiiHybridStringBase<Size>(A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(xiiAllocator* pAllocator) :
  xiiHybridStringBase<Size>(pAllocator)
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(const xiiHybridString<Size, A>& other) :
  xiiHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(const xiiHybridStringBase<Size>& other) :
  xiiHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(xiiHybridString<Size, A>&& other) :
  xiiHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(xiiHybridStringBase<Size>&& other) :
  xiiHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(const char* rhs) :
  xiiHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(const wchar_t* rhs) :
  xiiHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(const xiiStringView& rhs) :
  xiiHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(const xiiHybridString<Size, A>& rhs)
{
  xiiHybridStringBase<Size>::operator=(rhs);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(const xiiHybridStringBase<Size>& rhs)
{
  xiiHybridStringBase<Size>::operator=(rhs);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(xiiHybridString<Size, A>&& rhs)
{
  xiiHybridStringBase<Size>::operator=(std::move(rhs));
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(xiiHybridStringBase<Size>&& rhs)
{
  xiiHybridStringBase<Size>::operator=(std::move(rhs));
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(const char* rhs)
{
  xiiHybridStringBase<Size>::operator=(rhs);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(const wchar_t* rhs)
{
  xiiHybridStringBase<Size>::operator=(rhs);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(const xiiStringView& rhs)
{
  xiiHybridStringBase<Size>::operator=(rhs);
}

#if XII_ENABLED(XII_INTEROP_STL_STRINGS)

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(const std::string_view& rhs, xiiAllocator* pAllocator)
{
  *this = rhs;
}

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(const std::string& rhs, xiiAllocator* pAllocator)
{
  *this = rhs;
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(const std::string_view& rhs)
{
  if (rhs.empty())
  {
    Clear();
  }
  else
  {
    m_Data.SetCountUninitialized(((xiiUInt32)rhs.size() + 1));
    xiiStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.data(), rhs.data() + rhs.size());
  }
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(const std::string& rhs)
{
  *this = std::string_view(rhs);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(const std::string_view& rhs) :
  xiiHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(const std::string& rhs) :
  xiiHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(const std::string_view& rhs)
{
  xiiHybridStringBase<Size>::operator=(rhs);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(const std::string& rhs)
{
  xiiHybridStringBase<Size>::operator=(rhs);
}

#endif

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
