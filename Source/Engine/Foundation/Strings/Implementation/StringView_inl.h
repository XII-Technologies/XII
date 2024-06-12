#pragma once

XII_ALWAYS_INLINE constexpr xiiStringView::xiiStringView() = default;

XII_ALWAYS_INLINE xiiStringView::xiiStringView(char* pStart) :
  m_pStart(pStart), m_uiElementCount(xiiStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
constexpr XII_ALWAYS_INLINE xiiStringView::xiiStringView(T pStart, typename std::enable_if<std::is_same<T, const char*>::value, xiiInt32>::type*) :
  m_pStart(pStart), m_uiElementCount(xiiStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
constexpr XII_ALWAYS_INLINE xiiStringView::xiiStringView(const T&& str, typename std::enable_if<std::is_same<T, const char*>::value == false && std::is_convertible<T, const char*>::value, xiiInt32>::type*)
{
  m_pStart         = str;
  m_uiElementCount = xiiStringUtils::GetStringElementCount(m_pStart);
}

constexpr XII_ALWAYS_INLINE xiiStringView::xiiStringView(const char* pStart, const char* pEnd)
{
  XII_ASSERT_DEBUG(pStart <= pEnd, "Invalid pointers to construct a string view from.");

  m_pStart         = pStart;
  m_uiElementCount = static_cast<xiiUInt32>(pEnd - pStart);
}

constexpr XII_ALWAYS_INLINE xiiStringView::xiiStringView(const char* pStart, xiiUInt32 uiLength) :
  m_pStart(pStart), m_uiElementCount(uiLength)
{
}

template <size_t N>
constexpr XII_ALWAYS_INLINE xiiStringView::xiiStringView(const char (&str)[N]) :
  m_pStart(str), m_uiElementCount(N - 1)
{
  static_assert(N > 0, "Not a string literal");
}

template <size_t N>
constexpr XII_ALWAYS_INLINE xiiStringView::xiiStringView(char (&str)[N])
{
  m_pStart         = str;
  m_uiElementCount = xiiStringUtils::GetStringElementCount(str, str + N);
}

inline void xiiStringView::operator++()
{
  if (!IsValid())
    return;

  const char* pEnd = m_pStart + m_uiElementCount;
  xiiUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd).IgnoreResult(); // if it fails, the string is just empty
  m_uiElementCount = static_cast<xiiUInt32>(pEnd - m_pStart);
}

inline void xiiStringView::operator+=(xiiUInt32 d)
{
  const char* pEnd = m_pStart + m_uiElementCount;
  xiiUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd, d).IgnoreResult(); // if it fails, the string is just empty
  m_uiElementCount = static_cast<xiiUInt32>(pEnd - m_pStart);
}

XII_ALWAYS_INLINE bool xiiStringView::IsValid() const
{
  return (m_pStart != nullptr) && (m_uiElementCount > 0);
}

XII_ALWAYS_INLINE void xiiStringView::SetStartPosition(const char* szCurPos)
{
  XII_ASSERT_DEV((szCurPos >= m_pStart) && (szCurPos <= m_pStart + m_uiElementCount), "New start position must still be inside the view's range.");

  const char* pEnd = m_pStart + m_uiElementCount;
  m_pStart         = szCurPos;
  m_uiElementCount = static_cast<xiiUInt32>(pEnd - m_pStart);
}

XII_ALWAYS_INLINE bool xiiStringView::IsEmpty() const
{
  return m_uiElementCount == 0;
}

XII_ALWAYS_INLINE bool xiiStringView::IsEqual(xiiStringView sOther) const
{
  return xiiStringUtils::IsEqual(m_pStart, sOther.GetStartPointer(), m_pStart + m_uiElementCount, sOther.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::IsEqual_NoCase(xiiStringView sOther) const
{
  return xiiStringUtils::IsEqual_NoCase(m_pStart, sOther.GetStartPointer(), m_pStart + m_uiElementCount, sOther.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::StartsWith(xiiStringView sStartsWith) const
{
  return xiiStringUtils::StartsWith(m_pStart, sStartsWith.GetStartPointer(), m_pStart + m_uiElementCount, sStartsWith.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::StartsWith_NoCase(xiiStringView sStartsWith) const
{
  return xiiStringUtils::StartsWith_NoCase(m_pStart, sStartsWith.GetStartPointer(), m_pStart + m_uiElementCount, sStartsWith.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::EndsWith(xiiStringView sEndsWith) const
{
  return xiiStringUtils::EndsWith(m_pStart, sEndsWith.GetStartPointer(), m_pStart + m_uiElementCount, sEndsWith.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::EndsWith_NoCase(xiiStringView sEndsWith) const
{
  return xiiStringUtils::EndsWith_NoCase(m_pStart, sEndsWith.GetStartPointer(), m_pStart + m_uiElementCount, sEndsWith.GetEndPointer());
}

XII_ALWAYS_INLINE void xiiStringView::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

XII_ALWAYS_INLINE void xiiStringView::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  if (IsValid())
  {
    const char* pEnd = m_pStart + m_uiElementCount;
    xiiStringUtils::Trim(m_pStart, pEnd, szTrimCharsStart, szTrimCharsEnd);
    m_uiElementCount = static_cast<xiiUInt32>(pEnd - m_pStart);
  }
}

constexpr XII_ALWAYS_INLINE xiiStringView operator"" _xiisv(const char* pString, size_t uiLen)
{
  return xiiStringView(pString, static_cast<xiiUInt32>(uiLen));
}

template <typename Container>
void xiiStringView::Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  ref_output.Clear();

  if (IsEmpty())
    return;

  const xiiUInt32 uiParams = 6;

  const xiiStringView seps[uiParams] = {szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6};

  const char* szReadPos = GetStartPointer();

  while (true)
  {
    const char* szFoundPos       = xiiUnicodeUtils::GetMaxStringEnd<char>();
    xiiUInt32   uiFoundSeparator = 0;

    for (xiiUInt32 i = 0; i < uiParams; ++i)
    {
      const char* szFound = xiiStringUtils::FindSubString(szReadPos, seps[i].GetStartPointer(), GetEndPointer(), seps[i].GetEndPointer());

      if ((szFound != nullptr) && (szFound < szFoundPos))
      {
        szFoundPos       = szFound;
        uiFoundSeparator = i;
      }
    }

    // nothing found
    if (szFoundPos == xiiUnicodeUtils::GetMaxStringEnd<char>())
    {
      const xiiUInt32 uiLen = xiiStringUtils::GetStringElementCount(szReadPos, GetEndPointer());

      if (bReturnEmptyStrings || (uiLen > 0))
        ref_output.PushBack(xiiStringView(szReadPos, szReadPos + uiLen));

      return;
    }

    if (bReturnEmptyStrings || (szFoundPos > szReadPos))
      ref_output.PushBack(xiiStringView(szReadPos, szFoundPos));

    szReadPos = szFoundPos + seps[uiFoundSeparator].GetElementCount();
  }
}

XII_ALWAYS_INLINE bool operator==(xiiStringView lhs, xiiStringView rhs)
{
  return lhs.IsEqual(rhs);
}

XII_ALWAYS_INLINE std::strong_ordering operator<=>(xiiStringView lhs, xiiStringView rhs)
{
  return lhs.Compare(rhs) <=> 0;
}
