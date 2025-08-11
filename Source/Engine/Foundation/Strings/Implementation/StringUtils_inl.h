#pragma once

XII_ALWAYS_INLINE xiiInt32 xiiStringUtils::CompareChars(xiiUInt32 uiCharacter1, xiiUInt32 uiCharacter2)
{
  return (xiiInt32)uiCharacter1 - (xiiInt32)uiCharacter2;
}

inline xiiInt32 xiiStringUtils::CompareChars_NoCase(xiiUInt32 uiCharacter1, xiiUInt32 uiCharacter2)
{
  return (xiiInt32)ToUpperChar(uiCharacter1) - (xiiInt32)ToUpperChar(uiCharacter2);
}

inline xiiInt32 xiiStringUtils::CompareChars(const char* szUtf8Char1, const char* szUtf8Char2)
{
  return CompareChars(xiiUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char1), xiiUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char2));
}

inline xiiInt32 xiiStringUtils::CompareChars_NoCase(const char* szUtf8Char1, const char* szUtf8Char2)
{
  return CompareChars_NoCase(xiiUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char1), xiiUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char2));
}

template <typename T>
XII_ALWAYS_INLINE constexpr bool xiiStringUtils::IsNullOrEmpty(const T* pString)
{
  return (pString == nullptr) || (pString[0] == '\0');
}

template <typename T>
XII_ALWAYS_INLINE bool xiiStringUtils::IsNullOrEmpty(const T* pString, const T* pStringEnd)
{
  return (pString == nullptr) || pString == pStringEnd || (pString[0] == '\0');
}

template <typename T>
XII_ALWAYS_INLINE void xiiStringUtils::UpdateStringEnd(const T* pStringStart, const T*& ref_pStringEnd)
{
  if (ref_pStringEnd != xiiUnicodeUtils::GetMaxStringEnd<T>())
    return;

  ref_pStringEnd = pStringStart + GetStringElementCount(pStringStart, xiiUnicodeUtils::GetMaxStringEnd<T>());
}

template <typename T>
constexpr xiiUInt32 xiiStringUtils::GetStringElementCount(const T* pString)
{
  // can't use strlen here as long as it's not constexpr (C++ 23)

  if (pString == nullptr)
    return 0;

  xiiUInt32 uiCount = 0;
  while (*pString != '\0')
  {
    ++pString;
    ++uiCount;
  }

  return uiCount;
}

template <typename T>
xiiUInt32 xiiStringUtils::GetStringElementCount(const T* pString, const T* pStringEnd)
{
  if (IsNullOrEmpty(pString))
    return 0;

  if (pStringEnd != xiiUnicodeUtils::GetMaxStringEnd<T>())
    return (xiiUInt32)(pStringEnd - pString);

  xiiUInt32 uiCount = 0;
  while ((pString < pStringEnd) && (*pString != '\0'))
  {
    ++pString;
    ++uiCount;
  }

  return uiCount;
}

inline xiiUInt32 xiiStringUtils::GetCharacterCount(const char* szUtf8, const char* pStringEnd)
{
  if (IsNullOrEmpty(szUtf8))
    return 0;

  xiiUInt32 uiCharacters = 0;

  while ((szUtf8 < pStringEnd) && (*szUtf8 != '\0'))
  {
    // skip all the Utf8 continuation bytes
    if (!xiiUnicodeUtils::IsUtf8ContinuationByte(*szUtf8)) ++uiCharacters;

    ++szUtf8;
  }

  return uiCharacters;
}

inline void xiiStringUtils::GetCharacterAndElementCount(const char* szUtf8, xiiUInt32& ref_uiCharacterCount, xiiUInt32& ref_uiElementCount, const char* pStringEnd)
{
  ref_uiCharacterCount = 0;
  ref_uiElementCount   = 0;

  if (IsNullOrEmpty(szUtf8))
    return;

  while (szUtf8 < pStringEnd)
  {
    char uiByte = *szUtf8;
    if (uiByte == '\0')
    {
      break;
    }

    // skip all the Utf8 continuation bytes
    if (!xiiUnicodeUtils::IsUtf8ContinuationByte(uiByte))
      ++ref_uiCharacterCount;

    ++szUtf8;
    ++ref_uiElementCount;
  }
}

XII_ALWAYS_INLINE bool xiiStringUtils::IsEqual(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return xiiStringUtils::Compare(pString1, pString2, pString1End, pString2End) == 0;
}

XII_ALWAYS_INLINE bool xiiStringUtils::IsEqualN(const char* pString1, const char* pString2, xiiUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return xiiStringUtils::CompareN(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

XII_ALWAYS_INLINE bool xiiStringUtils::IsEqual_NoCase(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return xiiStringUtils::Compare_NoCase(pString1, pString2, pString1End, pString2End) == 0;
}

XII_ALWAYS_INLINE bool xiiStringUtils::IsEqualN_NoCase(const char* pString1, const char* pString2, xiiUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return xiiStringUtils::CompareN_NoCase(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

XII_ALWAYS_INLINE bool xiiStringUtils::IsDecimalDigit(xiiUInt32 uiChar)
{
  return (uiChar >= '0' && uiChar <= '9');
}

XII_ALWAYS_INLINE bool xiiStringUtils::IsHexDigit(xiiUInt32 uiChar)
{
  return IsDecimalDigit(uiChar) || (uiChar >= 'A' && uiChar <= 'F') || (uiChar >= 'a' && uiChar <= 'f');
}
