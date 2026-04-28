/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

XII_CREATE_SIMPLE_TEST(Strings, UnicodeUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsASCII")
  {
    // test all ASCII Characters
    for (xiiUInt32 i = 0; i < 128; ++i)
      XII_TEST_BOOL(xiiUnicodeUtils::IsASCII(i));

    for (xiiUInt32 i = 128; i < 0xFFFFF; ++i)
      XII_TEST_BOOL(!xiiUnicodeUtils::IsASCII(i));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsUtf8StartByte")
  {
    xiiStringUtf8 s(L"äöü€");
    // ä
    XII_TEST_BOOL(xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[0]));
    XII_TEST_BOOL(!xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[1]));

    // ö
    XII_TEST_BOOL(xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[2]));
    XII_TEST_BOOL(!xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[3]));

    // ü
    XII_TEST_BOOL(xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[4]));
    XII_TEST_BOOL(!xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[5]));

    // €
    XII_TEST_BOOL(xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[6]));
    XII_TEST_BOOL(!xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[7]));
    XII_TEST_BOOL(!xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[8]));

    // \0
    XII_TEST_BOOL(xiiUnicodeUtils::IsUtf8StartByte(s.GetData()[9]));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsUtf8ContinuationByte")
  {
    // all ASCII Characters are not continuation bytes
    for (char i = 0; i < 127; ++i)
    {
      XII_TEST_BOOL(!xiiUnicodeUtils::IsUtf8ContinuationByte(i));
    }

    for (xiiUInt32 i = 0; i < 255u; ++i)
    {
      const char uiContByte    = static_cast<char>(0x80 | (i & 0x3F));
      const char uiNoContByte1 = static_cast<char>(i | 0x40);
      const char uiNoContByte2 = static_cast<char>(i | 0xC0);

      XII_TEST_BOOL(xiiUnicodeUtils::IsUtf8ContinuationByte(uiContByte));
      XII_TEST_BOOL(!xiiUnicodeUtils::IsUtf8ContinuationByte(uiNoContByte1));
      XII_TEST_BOOL(!xiiUnicodeUtils::IsUtf8ContinuationByte(uiNoContByte2));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetUtf8SequenceLength")
  {
    // All ASCII characters are 1 byte in length
    for (char i = 0; i < 127; ++i)
    {
      XII_TEST_INT(xiiUnicodeUtils::GetUtf8SequenceLength(i), 1);
    }

    {
      xiiStringUtf8 s(L"ä");
      XII_TEST_INT(xiiUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      xiiStringUtf8 s(L"ß");
      XII_TEST_INT(xiiUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      xiiStringUtf8 s(L"€");
      XII_TEST_INT(xiiUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 3);
    }

    {
      xiiStringUtf8 s(L"з");
      XII_TEST_INT(xiiUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      xiiStringUtf8 s(L"г");
      XII_TEST_INT(xiiUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      xiiStringUtf8 s(L"ы");
      XII_TEST_INT(xiiUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      xiiUInt32     u[2] = {L'\u0B87', 0};
      xiiStringUtf8 s(u);
      XII_TEST_INT(xiiUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 3);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ConvertUtf8ToUtf32")
  {
    // Just wraps around 'utf8::peek_next'
    // I think we can assume that that works.
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetSizeForCharacterInUtf8")
  {
    // All ASCII characters are 1 byte in length
    for (xiiUInt32 i = 0; i < 128; ++i)
      XII_TEST_INT(xiiUnicodeUtils::GetSizeForCharacterInUtf8(i), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Decode")
  {
    char      utf8[]  = {(char)0xC3, (char)0xB6, 0};
    xiiUInt16 utf16[] = {0xF6, 0};
    wchar_t   wchar[] = {L'ö', 0};

    char*      szUtf8  = &utf8[0];
    xiiUInt16* szUtf16 = &utf16[0];
    wchar_t*   szWChar = &wchar[0];

    xiiUInt32 uiUtf321 = xiiUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);
    xiiUInt32 uiUtf322 = xiiUnicodeUtils::DecodeUtf16ToUtf32(szUtf16);
    xiiUInt32 uiUtf323 = xiiUnicodeUtils::DecodeWCharToUtf32(szWChar);

    XII_TEST_INT(uiUtf321, uiUtf322);
    XII_TEST_INT(uiUtf321, uiUtf323);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Encode")
  {
    char      utf8[4]  = {0};
    xiiUInt16 utf16[4] = {0};
    wchar_t   wchar[4] = {0};

    char*      szUtf8  = &utf8[0];
    xiiUInt16* szUtf16 = &utf16[0];
    wchar_t*   szWChar = &wchar[0];

    xiiUnicodeUtils::EncodeUtf32ToUtf8(0xF6, szUtf8);
    xiiUnicodeUtils::EncodeUtf32ToUtf16(0xF6, szUtf16);
    xiiUnicodeUtils::EncodeUtf32ToWChar(0xF6, szWChar);

    XII_TEST_BOOL(utf8[0] == (char)0xC3 && utf8[1] == (char)0xB6);
    XII_TEST_BOOL(utf16[0] == 0xF6);
    XII_TEST_BOOL(wchar[0] == L'ö');
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MoveToNextUtf8")
  {
    xiiStringUtf8 s(L"aböäß€de");

    XII_TEST_INT(s.GetElementCount(), 13);

    const char* sz = s.GetData();

    // test how far it skips ahead

    xiiUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[1]);

    xiiUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[2]);

    xiiUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[4]);

    xiiUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[6]);

    xiiUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[8]);

    xiiUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[11]);

    xiiUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[12]);

    sz                = s.GetData();
    const char* szEnd = s.GetView().GetEndPointer();


    xiiUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[1]);

    xiiUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[2]);

    xiiUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[4]);

    xiiUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[6]);

    xiiUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[8]);

    xiiUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[11]);

    xiiUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[12]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MoveToPriorUtf8")
  {
    xiiStringUtf8 s(L"aböäß€de");

    const char* sz = &s.GetData()[13];

    XII_TEST_INT(s.GetElementCount(), 13);

    // test how far it skips ahead

    xiiUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[12]);

    xiiUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[11]);

    xiiUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[8]);

    xiiUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[6]);

    xiiUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[4]);

    xiiUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[2]);

    xiiUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[1]);

    xiiUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    XII_TEST_BOOL(sz == &s.GetData()[0]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SkipUtf8Bom")
  {
    // C++ is really stupid, chars are signed, but Utf8 only works with unsigned values ... argh!

    char        szWithBom[] = {(char)0XEF, (char)0xBB, (char)0xBF, 'a'};
    char        szNoBom[]   = {'a'};
    const char* pString     = szWithBom;

    XII_TEST_BOOL(xiiUnicodeUtils::SkipUtf8Bom(pString) == true);
    XII_TEST_BOOL(pString == &szWithBom[3]);

    pString = szNoBom;

    XII_TEST_BOOL(xiiUnicodeUtils::SkipUtf8Bom(pString) == false);
    XII_TEST_BOOL(pString == szNoBom);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SkipUtf16BomLE")
  {
    xiiUInt16 szWithBom[] = {0XFEFF, 'a'};
    xiiUInt16 szNoBom[]   = {'a'};

    const xiiUInt16* pString = szWithBom;

    XII_TEST_BOOL(xiiUnicodeUtils::SkipUtf16BomLE(pString) == true);
    XII_TEST_BOOL(pString == &szWithBom[1]);

    pString = szNoBom;

    XII_TEST_BOOL(xiiUnicodeUtils::SkipUtf16BomLE(pString) == false);
    XII_TEST_BOOL(pString == szNoBom);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SkipUtf16BomBE")
  {
    xiiUInt16 szWithBom[] = {0XFFFE, 'a'};
    xiiUInt16 szNoBom[]   = {'a'};

    const xiiUInt16* pString = szWithBom;

    XII_TEST_BOOL(xiiUnicodeUtils::SkipUtf16BomBE(pString) == true);
    XII_TEST_BOOL(pString == &szWithBom[1]);

    pString = szNoBom;

    XII_TEST_BOOL(xiiUnicodeUtils::SkipUtf16BomBE(pString) == false);
    XII_TEST_BOOL(pString == szNoBom);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsUtf16Surrogate")
  {
    xiiUInt16 szNoSurrogate[] = {0x2AD7};
    xiiUInt16 szSurrogate[]   = {0xD83E};

    XII_TEST_BOOL(xiiUnicodeUtils::IsUtf16Surrogate(szNoSurrogate) == false);
    XII_TEST_BOOL(xiiUnicodeUtils::IsUtf16Surrogate(szSurrogate) == true);
  }
}
