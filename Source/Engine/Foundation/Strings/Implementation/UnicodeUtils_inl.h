#pragma once

/*
You can classify bytes in a UTF-8 stream as follows:
  With the high bit set to 0, it's a single byte value.
  With the two high bits set to 10, it's a continuation byte (the second, third or fourth byte in a UTF-8 multi-byte sequence).
  Otherwise, it's the first byte of a multi-byte sequence and the number of leading 1 bits indicates how many bytes there are in total for
this sequence (110... means two bytes, 1110... means three bytes, etc).
*/

XII_ALWAYS_INLINE bool xiiUnicodeUtils::IsUtf8StartByte(char uiByte)
{
  // valid utf8 start bytes are 0x0-------, 0x110-----, 0x1110----, 0x11110---, etc
  return ((uiByte & 0x80) == 0) || ((uiByte & 0xE0) == 0xC0) || ((uiByte & 0xF0) == 0xE0) || ((uiByte & 0xF8) == 0xF0) || ((uiByte & 0xFC) == 0xF8);
}

XII_ALWAYS_INLINE bool xiiUnicodeUtils::IsUtf8ContinuationByte(char uiByte)
{
  // check whether the two upper bits are set to '10'
  return (uiByte & 0xC0) == 0x80;
}

XII_ALWAYS_INLINE bool xiiUnicodeUtils::IsASCII(xiiUInt32 uiChar)
{
  return (uiChar <= 127);
}

inline xiiUInt32 xiiUnicodeUtils::GetUtf8SequenceLength(char uiFirstByte)
{
  const xiiUInt32 uiBit7 = uiFirstByte & XII_BIT(7);
  const xiiUInt32 uiBit6 = uiFirstByte & XII_BIT(6);
  const xiiUInt32 uiBit5 = uiFirstByte & XII_BIT(5);
  const xiiUInt32 uiBit4 = uiFirstByte & XII_BIT(4);

  if (uiBit7 == 0) // ASCII character '0xxxxxxx'
    return 1;

  XII_IGNORE_UNUSED(uiBit6);
  XII_ASSERT_DEV(uiBit6 != 0, "Invalid Leading UTF-8 Byte.");

  if (uiBit5 == 0) // '110xxxxx'
    return 2;
  if (uiBit4 == 0) // '1110xxxx'
    return 3;

  // '1111xxxx'
  return 4;
}

template <typename ByteIterator>
xiiUInt32 xiiUnicodeUtils::DecodeUtf8ToUtf32(ByteIterator& szUtf8Iterator)
{
  return utf8::unchecked::next(szUtf8Iterator);
}

template <typename UInt16Iterator>
bool xiiUnicodeUtils::IsUtf16Surrogate(UInt16Iterator& szUtf16Iterator)
{
  uint32_t cp = utf8::internal::mask16(*szUtf16Iterator);
  return utf8::internal::is_lead_surrogate(cp);
}

template <typename UInt16Iterator>
xiiUInt32 xiiUnicodeUtils::DecodeUtf16ToUtf32(UInt16Iterator& szUtf16Iterator)
{
  uint32_t cp = utf8::internal::mask16(*szUtf16Iterator++);
  if (utf8::internal::is_lead_surrogate(cp))
  {
    uint32_t trail_surrogate = utf8::internal::mask16(*szUtf16Iterator++);
    cp                       = (cp << 10) + trail_surrogate + utf8::internal::SURROGATE_OFFSET;
  }

  return cp;
}

template <typename WCharIterator>
xiiUInt32 xiiUnicodeUtils::DecodeWCharToUtf32(WCharIterator& szWCharIterator)
{
  if (sizeof(wchar_t) == 2)
  {
    return DecodeUtf16ToUtf32(szWCharIterator);
  }
  else // sizeof(wchar_t) == 4
  {
    const xiiUInt32 uiResult = *szWCharIterator;
    ++szWCharIterator;
    return uiResult;
  }
}

template <typename ByteIterator>
void xiiUnicodeUtils::EncodeUtf32ToUtf8(xiiUInt32 uiUtf32, ByteIterator& szUtf8Output)
{
  szUtf8Output = utf8::unchecked::utf32to8(&uiUtf32, &uiUtf32 + 1, szUtf8Output);
}

template <typename UInt16Iterator>
void xiiUnicodeUtils::EncodeUtf32ToUtf16(xiiUInt32 uiUtf32, UInt16Iterator& szUtf16Output)
{
  if (uiUtf32 > 0xffff)
  {
    // make a surrogate pair
    *szUtf16Output++ = static_cast<uint16_t>((uiUtf32 >> 10) + utf8::internal::LEAD_OFFSET);
    *szUtf16Output++ = static_cast<uint16_t>((uiUtf32 & 0x3ff) + utf8::internal::TRAIL_SURROGATE_MIN);
  }
  else
    *szUtf16Output++ = static_cast<uint16_t>(uiUtf32);
}

template <typename WCharIterator>
void xiiUnicodeUtils::EncodeUtf32ToWChar(xiiUInt32 uiUtf32, WCharIterator& szWCharOutput)
{
  if (sizeof(wchar_t) == 2)
  {
    EncodeUtf32ToUtf16(uiUtf32, szWCharOutput);
  }
  else
  {
    *szWCharOutput = static_cast<wchar_t>(uiUtf32);
    ++szWCharOutput;
  }
}

inline xiiUInt32 xiiUnicodeUtils::ConvertUtf8ToUtf32(const char* pFirstChar)
{
  return utf8::unchecked::peek_next(pFirstChar);
}

inline xiiUInt32 xiiUnicodeUtils::GetSizeForCharacterInUtf8(xiiUInt32 uiCharacter)
{
  // Basically implements this: http://en.wikipedia.org/wiki/Utf8#Description

  if (uiCharacter <= 0x0000007f)
    return 1;

  if (uiCharacter <= 0x000007ff)
    return 2;

  if (uiCharacter <= 0x0000ffff)
    return 3;

  // UTF-8 can use up to 6 bytes to encode a code point
  // however some committee agreed that never more than 4 bytes are used (no need for more than 21 Bits)
  // this implementation assumes in several places, that the UTF-8 encoding never uses more than 4 bytes

  XII_ASSERT_DEV(uiCharacter <= 0x0010ffff, "Invalid Unicode Codepoint");
  return 4;
}

inline bool xiiUnicodeUtils::IsValidUtf8(const char* szString, const char* szStringEnd)
{
  if (szStringEnd == GetMaxStringEnd<char>())
    szStringEnd = szString + strlen(szString);

  return utf8::is_valid(szString, szStringEnd);
}

inline bool xiiUnicodeUtils::SkipUtf8Bom(const char*& szUtf8)
{
  XII_ASSERT_DEBUG(szUtf8 != nullptr, "This function expects non nullptr pointers");

  if (utf8::starts_with_bom(szUtf8, szUtf8 + 4))
  {
    szUtf8 += 3;
    return true;
  }

  return false;
}

inline bool xiiUnicodeUtils::SkipUtf16BomLE(const xiiUInt16*& szUtf16)
{
  XII_ASSERT_DEBUG(szUtf16 != nullptr, "This function expects non nullptr pointers");

  if (*szUtf16 == xiiUnicodeUtils::Utf16BomLE)
  {
    ++szUtf16;
    return true;
  }

  return false;
}

inline bool xiiUnicodeUtils::SkipUtf16BomBE(const xiiUInt16*& szUtf16)
{
  XII_ASSERT_DEBUG(szUtf16 != nullptr, "This function expects non nullptr pointers");

  if (*szUtf16 == xiiUnicodeUtils::Utf16BomBE)
  {
    ++szUtf16;
    return true;
  }

  return false;
}

inline void xiiUnicodeUtils::MoveToNextUtf8(const char*& szUtf8, xiiUInt32 uiNumCharacters)
{
  XII_ASSERT_DEBUG(szUtf8 != nullptr, "Bad programmer!");

  while (uiNumCharacters > 0)
  {
    XII_ASSERT_DEV(*szUtf8 != '\0', "The given string must not point to the zero terminator.");

    do
    {
      ++szUtf8;
    } while (IsUtf8ContinuationByte(*szUtf8));

    --uiNumCharacters;
  }
}

inline void xiiUnicodeUtils::MoveToNextUtf8(const char*& szUtf8, const char* szUtf8End, xiiUInt32 uiNumCharacters)
{
  XII_ASSERT_DEBUG(szUtf8 != nullptr, "Bad programmer!");

  while (uiNumCharacters > 0 && szUtf8 < szUtf8End)
  {
    XII_ASSERT_DEV(*szUtf8 != '\0', "The given string must not point to the zero terminator.");

    do
    {
      ++szUtf8;
    } while ((szUtf8 < szUtf8End) && IsUtf8ContinuationByte(*szUtf8));

    --uiNumCharacters;
  }
}

inline void xiiUnicodeUtils::MoveToPriorUtf8(const char*& szUtf8, xiiUInt32 uiNumCharacters)
{
  XII_ASSERT_DEBUG(szUtf8 != nullptr, "Bad programmer!");

  while (uiNumCharacters > 0)
  {
    do
    {
      --szUtf8;
    } while (IsUtf8ContinuationByte(*szUtf8));

    --uiNumCharacters;
  }
}
template <typename T>
constexpr T* xiiUnicodeUtils::GetMaxStringEnd()
{
  return reinterpret_cast<T*>(-1);
}
