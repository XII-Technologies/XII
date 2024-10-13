#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringConversion.h>

// **************** xiiStringWChar ****************

void xiiStringWChar::operator=(const xiiUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    xiiUnicodeUtils::SkipUtf16BomLE(pUtf16);
    XII_ASSERT_DEV(!xiiUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    xiiUnicodeUtils::UtfInserter<wchar_t, xiiHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (*pUtf16 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeUtf16ToUtf32(pUtf16);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringWChar::operator=(const xiiUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<wchar_t, xiiHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringWChar::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {

    while (*pWChar != '\0')
    {
      m_Data.PushBack(*pWChar);
      ++pWChar;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringWChar::operator=(xiiStringView sUtf8)
{
  m_Data.Clear();

  if (!sUtf8.IsEmpty())
  {
    const char* szUtf8 = sUtf8.GetStartPointer();

    XII_ASSERT_DEV(xiiUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

    // skip any Utf8 Byte Order Mark
    xiiUnicodeUtils::SkipUtf8Bom(szUtf8);

    xiiUnicodeUtils::UtfInserter<wchar_t, xiiHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (szUtf8 < sUtf8.GetEndPointer() && *szUtf8 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

// **************** xiiStringUtf8 ****************

void xiiStringUtf8::operator=(const char* szUtf8)
{
  XII_ASSERT_DEV(xiiUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    xiiUnicodeUtils::SkipUtf8Bom(szUtf8);

    while (*szUtf8 != '\0')
    {
      m_Data.PushBack(*szUtf8);
      ++szUtf8;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf8::operator=(const xiiUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    xiiUnicodeUtils::SkipUtf16BomLE(pUtf16);
    XII_ASSERT_DEV(!xiiUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    xiiUnicodeUtils::UtfInserter<char, xiiHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pUtf16 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeUtf16ToUtf32(pUtf16);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf8::operator=(const xiiUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<char, xiiHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringUtf8::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<char, xiiHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pWChar != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeWCharToUtf32(pWChar);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

// **************** xiiStringUtf16 ****************

void xiiStringUtf16::operator=(const char* szUtf8)
{
  XII_ASSERT_DEV(xiiUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    xiiUnicodeUtils::SkipUtf8Bom(szUtf8);

    xiiUnicodeUtils::UtfInserter<xiiUInt16, xiiHybridArray<xiiUInt16, BufferSize>> tempInserter(&m_Data);

    while (*szUtf8 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf16::operator=(const xiiUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    xiiUnicodeUtils::SkipUtf16BomLE(pUtf16);
    XII_ASSERT_DEV(!xiiUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*pUtf16 != '\0')
    {
      m_Data.PushBack(*pUtf16);
      ++pUtf16;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf16::operator=(const xiiUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<xiiUInt16, xiiHybridArray<xiiUInt16, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringUtf16::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<xiiUInt16, xiiHybridArray<xiiUInt16, BufferSize>> tempInserter(&m_Data);

    while (*pWChar != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeWCharToUtf32(pWChar);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}



// **************** xiiStringUtf32 ****************

void xiiStringUtf32::operator=(const char* szUtf8)
{
  XII_ASSERT_DEV(xiiUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    xiiUnicodeUtils::SkipUtf8Bom(szUtf8);

    while (*szUtf8 != '\0')
    {
      // decode utf8 to utf32
      m_Data.PushBack(xiiUnicodeUtils::DecodeUtf8ToUtf32(szUtf8));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf32::operator=(const xiiUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    xiiUnicodeUtils::SkipUtf16BomLE(pUtf16);
    XII_ASSERT_DEV(!xiiUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*pUtf16 != '\0')
    {
      // decode utf16 to utf32
      m_Data.PushBack(xiiUnicodeUtils::DecodeUtf16ToUtf32(pUtf16));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf32::operator=(const xiiUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    while (*pUtf32 != '\0')
    {
      m_Data.PushBack(*pUtf32);
      ++pUtf32;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringUtf32::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    while (*pWChar != '\0')
    {
      // decode wchar_t to utf32
      m_Data.PushBack(xiiUnicodeUtils::DecodeWCharToUtf32(pWChar));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringConversion);
