#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringConversion.h>

// **************** xiiStringWChar ****************

void xiiStringWChar::operator=(const xiiUInt16* szUtf16)
{
  m_Data.Clear();

  if (szUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    xiiUnicodeUtils::SkipUtf16BomLE(szUtf16);
    XII_ASSERT_DEV(!xiiUnicodeUtils::SkipUtf16BomBE(szUtf16), "Utf-16 Big Endian is currently not supported.");

    xiiUnicodeUtils::UtfInserter<wchar_t, xiiHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (*szUtf16 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeUtf16ToUtf32(szUtf16);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringWChar::operator=(const xiiUInt32* szUtf32)
{
  m_Data.Clear();

  if (szUtf32 != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<wchar_t, xiiHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (*szUtf32 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = *szUtf32;
      ++szUtf32;

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringWChar::operator=(const wchar_t* szWChar)
{
  m_Data.Clear();

  if (szWChar != nullptr)
  {

    while (*szWChar != '\0')
    {
      m_Data.PushBack(*szWChar);
      ++szWChar;
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
  XII_ASSERT_DEV(
    xiiUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

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


void xiiStringUtf8::operator=(const xiiUInt16* szUtf16)
{
  m_Data.Clear();

  if (szUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    xiiUnicodeUtils::SkipUtf16BomLE(szUtf16);
    XII_ASSERT_DEV(!xiiUnicodeUtils::SkipUtf16BomBE(szUtf16), "Utf-16 Big Endian is currently not supported.");

    xiiUnicodeUtils::UtfInserter<char, xiiHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*szUtf16 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeUtf16ToUtf32(szUtf16);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf8::operator=(const xiiUInt32* szUtf32)
{
  m_Data.Clear();

  if (szUtf32 != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<char, xiiHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*szUtf32 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = *szUtf32;
      ++szUtf32;

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringUtf8::operator=(const wchar_t* szWChar)
{
  m_Data.Clear();

  if (szWChar != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<char, xiiHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*szWChar != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeWCharToUtf32(szWChar);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

void xiiStringUtf8::operator=(const Microsoft::WRL::Wrappers::HString& hstring)
{
  xiiUInt32      len = 0;
  const wchar_t* raw = hstring.GetRawBuffer(&len);

  // delegate to wchar_t operator
  *this = raw;
}

void xiiStringUtf8::operator=(const HSTRING& hstring)
{
  Microsoft::WRL::Wrappers::HString tmp;
  tmp.Attach(hstring);

  xiiUInt32      len = 0;
  const wchar_t* raw = tmp.GetRawBuffer(&len);

  // delegate to wchar_t operator
  *this = raw;
}

#endif


// **************** xiiStringUtf16 ****************

void xiiStringUtf16::operator=(const char* szUtf8)
{
  XII_ASSERT_DEV(
    xiiUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

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


void xiiStringUtf16::operator=(const xiiUInt16* szUtf16)
{
  m_Data.Clear();

  if (szUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    xiiUnicodeUtils::SkipUtf16BomLE(szUtf16);
    XII_ASSERT_DEV(!xiiUnicodeUtils::SkipUtf16BomBE(szUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*szUtf16 != '\0')
    {
      m_Data.PushBack(*szUtf16);
      ++szUtf16;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf16::operator=(const xiiUInt32* szUtf32)
{
  m_Data.Clear();

  if (szUtf32 != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<xiiUInt16, xiiHybridArray<xiiUInt16, BufferSize>> tempInserter(&m_Data);

    while (*szUtf32 != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = *szUtf32;
      ++szUtf32;

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringUtf16::operator=(const wchar_t* szWChar)
{
  m_Data.Clear();

  if (szWChar != nullptr)
  {
    xiiUnicodeUtils::UtfInserter<xiiUInt16, xiiHybridArray<xiiUInt16, BufferSize>> tempInserter(&m_Data);

    while (*szWChar != '\0')
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeWCharToUtf32(szWChar);

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
  XII_ASSERT_DEV(
    xiiUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

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


void xiiStringUtf32::operator=(const xiiUInt16* szUtf16)
{
  m_Data.Clear();

  if (szUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    xiiUnicodeUtils::SkipUtf16BomLE(szUtf16);
    XII_ASSERT_DEV(!xiiUnicodeUtils::SkipUtf16BomBE(szUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*szUtf16 != '\0')
    {
      // decode utf16 to utf32
      m_Data.PushBack(xiiUnicodeUtils::DecodeUtf16ToUtf32(szUtf16));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void xiiStringUtf32::operator=(const xiiUInt32* szUtf32)
{
  m_Data.Clear();

  if (szUtf32 != nullptr)
  {
    while (*szUtf32 != '\0')
    {
      m_Data.PushBack(*szUtf32);
      ++szUtf32;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void xiiStringUtf32::operator=(const wchar_t* szWChar)
{
  m_Data.Clear();

  if (szWChar != nullptr)
  {
    while (*szWChar != '\0')
    {
      // decode wchar_t to utf32
      m_Data.PushBack(xiiUnicodeUtils::DecodeWCharToUtf32(szWChar));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

xiiStringHString::xiiStringHString()
{
}

xiiStringHString::xiiStringHString(const char* szUtf8)
{
  *this = szUtf8;
}

xiiStringHString::xiiStringHString(const xiiUInt16* szUtf16)
{
  *this = szUtf16;
}

xiiStringHString::xiiStringHString(const xiiUInt32* szUtf32)
{
  *this = szUtf32;
}

xiiStringHString::xiiStringHString(const wchar_t* szWChar)
{
  *this = szWChar;
}

void xiiStringHString::operator=(const char* szUtf8)
{
  m_Data.Set(xiiStringWChar(szUtf8).GetData());
}

void xiiStringHString::operator=(const xiiUInt16* szUtf16)
{
  m_Data.Set(xiiStringWChar(szUtf16).GetData());
}

void xiiStringHString::operator=(const xiiUInt32* szUtf32)
{
  m_Data.Set(xiiStringWChar(szUtf32).GetData());
}

void xiiStringHString::operator=(const wchar_t* szWChar)
{
  m_Data.Set(xiiStringWChar(szWChar).GetData());
}

#endif


XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringConversion);
