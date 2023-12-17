#pragma once

XII_ALWAYS_INLINE constexpr xiiStringView::xiiStringView() = default;

XII_ALWAYS_INLINE xiiStringView::xiiStringView(char* pStart) :
  m_pStart(pStart), m_pEnd(pStart + xiiStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
constexpr XII_ALWAYS_INLINE xiiStringView::xiiStringView(T pStart, typename std::enable_if<std::is_same<T, const char*>::value, xiiInt32>::type*) :
  m_pStart(pStart), m_pEnd(pStart + xiiStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
XII_ALWAYS_INLINE xiiStringView::xiiStringView(const T&& str, typename std::enable_if<std::is_same<T, const char*>::value == false && std::is_convertible<T, const char*>::value, xiiInt32>::type*)
{
  m_pStart = str;
  m_pEnd   = m_pStart + xiiStringUtils::GetStringElementCount(m_pStart);
}

XII_ALWAYS_INLINE xiiStringView::xiiStringView(const char* pStart, const char* pEnd)
{
  XII_ASSERT_DEV(pStart <= pEnd, "It should start BEFORE it ends.");

  m_pStart = pStart;
  m_pEnd   = pEnd;
}

constexpr XII_ALWAYS_INLINE xiiStringView::xiiStringView(const char* pStart, xiiUInt32 uiLength) :
  m_pStart(pStart), m_pEnd(pStart + uiLength)
{
}

template <size_t N>
XII_ALWAYS_INLINE xiiStringView::xiiStringView(const char (&str)[N]) :
  m_pStart(str), m_pEnd(str + N - 1)
{
  static_assert(N > 0, "Not a string literal");
  XII_ASSERT_DEBUG(str[N - 1] == '\0', "Not a string literal. Manually cast to 'const char*' if you are trying to pass a const char fixed size array.");
}

template <size_t N>
XII_ALWAYS_INLINE xiiStringView::xiiStringView(char (&str)[N])
{
  m_pStart = str;
  m_pEnd   = m_pStart + xiiStringUtils::GetStringElementCount(str, str + N);
}

inline void xiiStringView::operator++()
{
  if (!IsValid())
    return;

  xiiUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd);
}

inline void xiiStringView::operator+=(xiiUInt32 d)
{
  xiiUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd, d);
}
XII_ALWAYS_INLINE bool xiiStringView::IsValid() const
{
  return (m_pStart != nullptr) && (m_pStart < m_pEnd);
}

XII_ALWAYS_INLINE void xiiStringView::SetStartPosition(const char* szCurPos)
{
  XII_ASSERT_DEV((szCurPos >= m_pStart) && (szCurPos <= m_pEnd), "New start position must still be inside the view's range.");

  m_pStart = szCurPos;
}

XII_ALWAYS_INLINE bool xiiStringView::IsEmpty() const
{
  return m_pStart == m_pEnd || xiiStringUtils::IsNullOrEmpty(m_pStart);
}

XII_ALWAYS_INLINE bool xiiStringView::IsEqual(xiiStringView sOther) const
{
  return xiiStringUtils::IsEqual(m_pStart, sOther.GetStartPointer(), m_pEnd, sOther.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::IsEqual_NoCase(xiiStringView sOther) const
{
  return xiiStringUtils::IsEqual_NoCase(m_pStart, sOther.GetStartPointer(), m_pEnd, sOther.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::StartsWith(xiiStringView sStartsWith) const
{
  return xiiStringUtils::StartsWith(m_pStart, sStartsWith.GetStartPointer(), m_pEnd, sStartsWith.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::StartsWith_NoCase(xiiStringView sStartsWith) const
{
  return xiiStringUtils::StartsWith_NoCase(m_pStart, sStartsWith.GetStartPointer(), m_pEnd, sStartsWith.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::EndsWith(xiiStringView sEndsWith) const
{
  return xiiStringUtils::EndsWith(m_pStart, sEndsWith.GetStartPointer(), m_pEnd, sEndsWith.GetEndPointer());
}

XII_ALWAYS_INLINE bool xiiStringView::EndsWith_NoCase(xiiStringView sEndsWith) const
{
  return xiiStringUtils::EndsWith_NoCase(m_pStart, sEndsWith.GetStartPointer(), m_pEnd, sEndsWith.GetEndPointer());
}

XII_ALWAYS_INLINE void xiiStringView::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

XII_ALWAYS_INLINE void xiiStringView::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  if (IsValid())
  {
    xiiStringUtils::Trim(m_pStart, m_pEnd, szTrimCharsStart, szTrimCharsEnd);
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

XII_ALWAYS_INLINE bool operator<(xiiStringView lhs, xiiStringView rhs)
{
  return lhs.Compare(rhs) < 0;
}

XII_ALWAYS_INLINE bool operator<=(xiiStringView lhs, xiiStringView rhs)
{
  return lhs.Compare(rhs) <= 0;
}

XII_ALWAYS_INLINE bool operator>(xiiStringView lhs, xiiStringView rhs)
{
  return lhs.Compare(rhs) > 0;
}

XII_ALWAYS_INLINE bool operator>=(xiiStringView lhs, xiiStringView rhs)
{
  return lhs.Compare(rhs) >= 0;
}
