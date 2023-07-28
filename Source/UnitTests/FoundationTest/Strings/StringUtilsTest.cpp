#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/String.h>

XII_CREATE_SIMPLE_TEST_GROUP(Strings);

XII_CREATE_SIMPLE_TEST(Strings, StringUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNullOrEmpty")
  {
    XII_TEST_BOOL(xiiStringUtils::IsNullOrEmpty((char*)nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::IsNullOrEmpty("") == true);

    // all other characters are not empty
    for (xiiUInt8 c = 1; c < 255; c++)
      XII_TEST_BOOL(xiiStringUtils::IsNullOrEmpty(&c) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetStringElementCount")
  {
    XII_TEST_INT(xiiStringUtils::GetStringElementCount((char*)nullptr), 0);

    // Counts the Bytes
    XII_TEST_INT(xiiStringUtils::GetStringElementCount(""), 0);
    XII_TEST_INT(xiiStringUtils::GetStringElementCount("a"), 1);
    XII_TEST_INT(xiiStringUtils::GetStringElementCount("ab"), 2);
    XII_TEST_INT(xiiStringUtils::GetStringElementCount("abc"), 3);

    // Counts the number of wchar_t's
    XII_TEST_INT(xiiStringUtils::GetStringElementCount(L""), 0);
    XII_TEST_INT(xiiStringUtils::GetStringElementCount(L"a"), 1);
    XII_TEST_INT(xiiStringUtils::GetStringElementCount(L"ab"), 2);
    XII_TEST_INT(xiiStringUtils::GetStringElementCount(L"abc"), 3);

    // test with a sub-string
    const char* sz = "abc def ghi";
    XII_TEST_INT(xiiStringUtils::GetStringElementCount(sz, sz + 0), 0);
    XII_TEST_INT(xiiStringUtils::GetStringElementCount(sz, sz + 3), 3);
    XII_TEST_INT(xiiStringUtils::GetStringElementCount(sz, sz + 6), 6);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "UpdateStringEnd")
  {
    const char* sz    = "Test test";
    const char* szEnd = xiiUnicodeUtils::GetMaxStringEnd<char>();

    xiiStringUtils::UpdateStringEnd(sz, szEnd);
    XII_TEST_BOOL(szEnd == sz + xiiStringUtils::GetStringElementCount(sz));

    xiiStringUtils::UpdateStringEnd(sz, szEnd);
    XII_TEST_BOOL(szEnd == sz + xiiStringUtils::GetStringElementCount(sz));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCharacterCount")
  {
    XII_TEST_INT(xiiStringUtils::GetCharacterCount(nullptr), 0);
    XII_TEST_INT(xiiStringUtils::GetCharacterCount(""), 0);
    XII_TEST_INT(xiiStringUtils::GetCharacterCount("a"), 1);
    XII_TEST_INT(xiiStringUtils::GetCharacterCount("abc"), 3);

    xiiStringUtf8 s(L"äöü"); // 6 Bytes

    XII_TEST_INT(xiiStringUtils::GetStringElementCount(s.GetData()), 6);
    XII_TEST_INT(xiiStringUtils::GetCharacterCount(s.GetData()), 3);

    // test with a sub-string
    const char* sz = "abc def ghi";
    XII_TEST_INT(xiiStringUtils::GetCharacterCount(sz, sz + 0), 0);
    XII_TEST_INT(xiiStringUtils::GetCharacterCount(sz, sz + 3), 3);
    XII_TEST_INT(xiiStringUtils::GetCharacterCount(sz, sz + 6), 6);

    XII_TEST_INT(xiiStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 0), 0);
    XII_TEST_INT(xiiStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 2), 1);
    XII_TEST_INT(xiiStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 4), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCharacterAndElementCount")
  {
    xiiUInt32 uiCC, uiEC;

    xiiStringUtils::GetCharacterAndElementCount(nullptr, uiCC, uiEC);
    XII_TEST_INT(uiCC, 0);
    XII_TEST_INT(uiEC, 0);

    xiiStringUtils::GetCharacterAndElementCount("", uiCC, uiEC);
    XII_TEST_INT(uiCC, 0);
    XII_TEST_INT(uiEC, 0);

    xiiStringUtils::GetCharacterAndElementCount("a", uiCC, uiEC);
    XII_TEST_INT(uiCC, 1);
    XII_TEST_INT(uiEC, 1);

    xiiStringUtils::GetCharacterAndElementCount("abc", uiCC, uiEC);
    XII_TEST_INT(uiCC, 3);
    XII_TEST_INT(uiEC, 3);

    xiiStringUtf8 s(L"äöü"); // 6 Bytes

    xiiStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC);
    XII_TEST_INT(uiCC, 3);
    XII_TEST_INT(uiEC, 6);

    xiiStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC, s.GetData() + 0);
    XII_TEST_INT(uiCC, 0);
    XII_TEST_INT(uiEC, 0);

    xiiStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC, s.GetData() + 4);
    XII_TEST_INT(uiCC, 2);
    XII_TEST_INT(uiEC, 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy (full)")
  {
    char szDest[256] = "";

    // large enough
    XII_TEST_INT(xiiStringUtils::Copy(szDest, 256, "Test ABC"), 8);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(szDest, "Test ABC"));

    // exactly fitting
    XII_TEST_INT(xiiStringUtils::Copy(szDest, 13, "Humpf, humpf"), 12);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(szDest, "Humpf, humpf"));

    // too small
    XII_TEST_INT(xiiStringUtils::Copy(szDest, 8, "Test ABC"), 7);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(szDest, "Test AB"));

    const char* szUTF8 = "ABC \xe6\x97\xa5\xd1\x88"; // contains 'ABC ' + two UTF-8 chars (first is three bytes, second is two bytes)

    // large enough
    XII_TEST_INT(xiiStringUtils::Copy(szDest, 256, szUTF8), 9);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(szDest, szUTF8));

    // exactly fitting
    XII_TEST_INT(xiiStringUtils::Copy(szDest, 10, szUTF8), 9);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(szDest, szUTF8));

    // These tests are disabled as previously valid behavior was now turned into an assert.
    // Comment them in to test the assert.
    // too small 1
    /*XII_TEST_INT(xiiStringUtils::Copy(szDest, 9, szUTF8), 7);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(szDest, szUTF8, 5)); // one character less

    // too small 2
    XII_TEST_INT(xiiStringUtils::Copy(szDest, 7, szUTF8), 4);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(szDest, szUTF8, 4)); // two characters less*/


    // copy only from a subset
    XII_TEST_INT(xiiStringUtils::Copy(szDest, 256, szUTF8, szUTF8 + 7), 7);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(szDest, szUTF8, 5)); // two characters less
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CopyN")
  {
    char szDest[256] = "";

    XII_TEST_INT(xiiStringUtils::CopyN(szDest, 256, "Test ABC", 4), 4);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(szDest, "Test"));

    const char* szUTF8 = "ABC \xe6\x97\xa5\xd1\x88"; // contains 'ABC ' + two UTF-8 chars (first is three bytes, second is two bytes)

    XII_TEST_INT(xiiStringUtils::CopyN(szDest, 256, szUTF8, 6), 9);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(szDest, szUTF8, 6));

    XII_TEST_INT(xiiStringUtils::CopyN(szDest, 256, szUTF8, 5), 7);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(szDest, szUTF8, 5));

    XII_TEST_INT(xiiStringUtils::CopyN(szDest, 256, szUTF8, 4), 4);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(szDest, szUTF8, 4));

    XII_TEST_INT(xiiStringUtils::CopyN(szDest, 256, szUTF8, 1), 1);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(szDest, szUTF8, 1));

    XII_TEST_INT(xiiStringUtils::CopyN(szDest, 256, szUTF8, 0), 0);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(szDest, ""));

    // copy only from a subset
    XII_TEST_INT(xiiStringUtils::CopyN(szDest, 256, szUTF8, 6, szUTF8 + 7), 7);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(szDest, szUTF8, 5));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ToUpperChar")
  {
    // this only tests the ASCII range
    for (xiiInt32 i = 0; i < 128; ++i)
      XII_TEST_INT(xiiStringUtils::ToUpperChar(i), toupper(i));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ToLowerChar")
  {
    // this only tests the ASCII range
    for (xiiInt32 i = 0; i < 128; ++i)
      XII_TEST_INT(xiiStringUtils::ToLowerChar(i), tolower(i));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ToUpperString")
  {
    xiiStringUtf8 sL(L"abc öäü ß €");
    xiiStringUtf8 sU(L"ABC ÖÄÜ ß €");

    char szCopy[256];
    xiiStringUtils::Copy(szCopy, 256, sL.GetData());

    xiiStringUtils::ToUpperString(szCopy);

    XII_TEST_BOOL(xiiStringUtils::IsEqual(szCopy, sU.GetData()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ToLowerString")
  {
    xiiStringUtf8 sL(L"abc öäü ß €");
    xiiStringUtf8 sU(L"ABC ÖÄÜ ß €");

    char szCopy[256];
    xiiStringUtils::Copy(szCopy, 256, sU.GetData());

    xiiStringUtils::ToLowerString(szCopy);

    XII_TEST_BOOL(xiiStringUtils::IsEqual(szCopy, sL.GetData()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompareChars")
  {
    XII_TEST_BOOL(xiiStringUtils::CompareChars('a', 'a') == 0); // make sure the order is right
    XII_TEST_BOOL(xiiStringUtils::CompareChars('a', 'b') < 0);  // a smaller than b -> negative
    XII_TEST_BOOL(xiiStringUtils::CompareChars('b', 'a') > 0);  // b bigger than a  -> positive
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompareChars(utf8)")
  {
    XII_TEST_BOOL(xiiStringUtils::CompareChars("a", "a") == 0); // make sure the order is right
    XII_TEST_BOOL(xiiStringUtils::CompareChars("a", "b") < 0);  // a smaller than b -> negative
    XII_TEST_BOOL(xiiStringUtils::CompareChars("b", "a") > 0);  // b bigger than a  -> positive
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompareChars_NoCase")
  {
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase('a', 'A') == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase('a', 'B') < 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase('B', 'a') > 0);

    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase('A', 'a') == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase('A', 'b') < 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase('b', 'A') > 0);

    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase(L'ä', L'Ä') == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase(L'ä', L'Ö') < 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase(L'ö', L'Ä') > 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompareChars_NoCase(utf8)")
  {
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase("a", "A") == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase("a", "B") < 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase("B", "a") > 0);

    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase("A", "a") == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase("A", "b") < 0);
    XII_TEST_BOOL(xiiStringUtils::CompareChars_NoCase("b", "A") > 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    XII_TEST_BOOL(xiiStringUtils::IsEqual(nullptr, nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(nullptr, "") == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqual("", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqual("", "") == true);

    XII_TEST_BOOL(xiiStringUtils::IsEqual("abc", "abc") == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqual("abc", "abcd") == false);
    XII_TEST_BOOL(xiiStringUtils::IsEqual("abcd", "abc") == false);

    XII_TEST_BOOL(xiiStringUtils::IsEqual("a", nullptr) == false);
    XII_TEST_BOOL(xiiStringUtils::IsEqual(nullptr, "a") == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqualN")
  {
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(nullptr, nullptr, 1) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(nullptr, "", 1) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("", nullptr, 1) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("", "", 1) == true);

    // as long as we compare 'nothing' the strings must be equal
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abc", nullptr, 0) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abc", "", 0) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN(nullptr, "abc", 0) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("", "abc", 0) == true);

    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abc", "abcdef", 1) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abc", "abcdef", 2) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abc", "abcdef", 3) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abc", "abcdef", 4) == false);

    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abcdef", "abc", 1) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abcdef", "abc", 2) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abcdef", "abc", 3) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN("abcdef", "abc", 4) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual_NoCase")
  {
    XII_TEST_BOOL(xiiStringUtils::IsEqual_NoCase(nullptr, nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqual_NoCase(nullptr, "") == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqual_NoCase("", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqual_NoCase("", "") == true);


    xiiStringUtf8 sL(L"abc öäü ß €");
    xiiStringUtf8 sU(L"ABC ÖÄÜ ß €");
    xiiStringUtf8 sU2(L"ABC ÖÄÜ ß € ");

    XII_TEST_BOOL(xiiStringUtils::IsEqual_NoCase(sL.GetData(), sU.GetData()) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqual_NoCase(sL.GetData(), sU2.GetData()) == false);
    XII_TEST_BOOL(xiiStringUtils::IsEqual_NoCase(sU2.GetData(), sL.GetData()) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqualN_NoCase")
  {
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase(nullptr, nullptr, 1) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase(nullptr, "", 1) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase("", nullptr, 1) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase("", "", 1) == true);

    // as long as we compare 'nothing' the strings must be equal
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase("abc", nullptr, 0) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase("abc", "", 0) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase(nullptr, "abc", 0) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase("", "abc", 0) == true);

    xiiStringUtf8 sL(L"abc öäü ß €");
    xiiStringUtf8 sU(L"ABC ÖÄÜ ß € moep");

    for (xiiInt32 i = 0; i < 12; ++i)
      XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase(sL.GetData(), sU.GetData(), i) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase(sL.GetData(), sU.GetData(), 12) == false);

    for (xiiInt32 i = 0; i < 12; ++i)
      XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase(sU.GetData(), sL.GetData(), i) == true);
    XII_TEST_BOOL(xiiStringUtils::IsEqualN_NoCase(sU.GetData(), sL.GetData(), 12) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compare")
  {
    XII_TEST_BOOL(xiiStringUtils::Compare(nullptr, nullptr) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare(nullptr, "") == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare("", nullptr) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare("", "") == 0);

    XII_TEST_BOOL(xiiStringUtils::Compare("abc", "abc") == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare("abc", "abcd") < 0);
    XII_TEST_BOOL(xiiStringUtils::Compare("abcd", "abc") > 0);

    XII_TEST_BOOL(xiiStringUtils::Compare("a", nullptr) > 0);
    XII_TEST_BOOL(xiiStringUtils::Compare(nullptr, "a") < 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    XII_TEST_BOOL(xiiStringUtils::Compare(sz, "abc", sz + 3) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare(sz, "abc def", sz + 7) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare(sz, sz, sz + 7, sz + 7) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare(sz, sz, sz + 7, sz + 6) > 0);
    XII_TEST_BOOL(xiiStringUtils::Compare(sz, sz, sz + 7, sz + 8) < 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompareN")
  {
    XII_TEST_BOOL(xiiStringUtils::CompareN(nullptr, nullptr, 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN(nullptr, "", 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("", nullptr, 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("", "", 1) == 0);

    // as long as we compare 'nothing' the strings must be equal
    XII_TEST_BOOL(xiiStringUtils::CompareN("abc", nullptr, 0) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("abc", "", 0) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN(nullptr, "abc", 0) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("", "abc", 0) == 0);

    XII_TEST_BOOL(xiiStringUtils::CompareN("abc", "abcdef", 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("abc", "abcdef", 2) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("abc", "abcdef", 3) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("abc", "abcdef", 4) < 0);

    XII_TEST_BOOL(xiiStringUtils::CompareN("abcdef", "abc", 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("abcdef", "abc", 2) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("abcdef", "abc", 3) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN("abcdef", "abc", 4) > 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    XII_TEST_BOOL(xiiStringUtils::CompareN(sz, "abc", 10, sz + 3) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN(sz, "abc def", 10, sz + 7) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 7) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 6) > 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 8) < 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compare_NoCase")
  {
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase(nullptr, nullptr) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase(nullptr, "") == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase("", nullptr) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase("", "") == 0);

    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase("abc", "aBc") == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase("ABC", "abcd") < 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase("abcd", "ABC") > 0);

    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase("a", nullptr) > 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase(nullptr, "a") < 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase(sz, "ABC", sz + 3) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase(sz, "ABC def", sz + 7) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 7) == 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 6) > 0);
    XII_TEST_BOOL(xiiStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 8) < 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompareN_NoCase")
  {
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase(nullptr, nullptr, 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase(nullptr, "", 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("", nullptr, 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("", "", 1) == 0);

    // as long as we compare 'nothing' the strings must be equal
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("abc", nullptr, 0) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("abc", "", 0) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase(nullptr, "abc", 0) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("", "abc", 0) == 0);

    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("aBc", "abcdef", 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("aBc", "abcdef", 2) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("aBc", "abcdef", 3) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("aBc", "abcdef", 4) < 0);

    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("abcdef", "Abc", 1) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("abcdef", "Abc", 2) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("abcdef", "Abc", 3) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase("abcdef", "Abc", 4) > 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase(sz, "ABC", 10, sz + 3) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase(sz, "ABC def", 10, sz + 7) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 7) == 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 6) > 0);
    XII_TEST_BOOL(xiiStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 8) < 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "snprintf")
  {
    // This function has been tested to death during its implementation.
    // That test-code would require several pages, if one would try to test it properly.
    // I am not going to do that here, I am quite confident the function works as expected with pure ASCII strings.
    // So I'm only testing a bit of Utf8 stuff.

    xiiStringUtf8 s(L"Abc %s äöü ß %i %s %.4f");
    xiiStringUtf8 s2(L"ÄÖÜ");

    char sz[256];
    xiiStringUtils::snprintf(sz, 256, s.GetData(), "ASCII", 42, s2.GetData(), 23.31415);

    xiiStringUtf8 sC(L"Abc ASCII äöü ß 42 ÄÖÜ 23.3142"); // notice the correct float rounding ;-)

    XII_TEST_STRING(sz, sC.GetData());


    // NaN and Infinity
    xiiStringUtils::snprintf(sz, 256, "NaN Value: %.2f", xiiMath::NaN<float>());
    XII_TEST_STRING(sz, "NaN Value: NaN");

    xiiStringUtils::snprintf(sz, 256, "Inf Value: %.2f", +xiiMath::Infinity<float>());
    XII_TEST_STRING(sz, "Inf Value: Infinity");

    xiiStringUtils::snprintf(sz, 256, "Inf Value: %.2f", -xiiMath::Infinity<float>());
    XII_TEST_STRING(sz, "Inf Value: -Infinity");

    xiiStringUtils::snprintf(sz, 256, "NaN Value: %.2e", xiiMath::NaN<float>());
    XII_TEST_STRING(sz, "NaN Value: NaN");

    xiiStringUtils::snprintf(sz, 256, "Inf Value: %.2e", +xiiMath::Infinity<float>());
    XII_TEST_STRING(sz, "Inf Value: Infinity");

    xiiStringUtils::snprintf(sz, 256, "Inf Value: %.2e", -xiiMath::Infinity<float>());
    XII_TEST_STRING(sz, "Inf Value: -Infinity");

    xiiStringUtils::snprintf(sz, 256, "NaN Value: %+10.2f", xiiMath::NaN<float>());
    XII_TEST_STRING(sz, "NaN Value:       +NaN");

    xiiStringUtils::snprintf(sz, 256, "Inf Value: %+10.2f", +xiiMath::Infinity<float>());
    XII_TEST_STRING(sz, "Inf Value:  +Infinity");

    xiiStringUtils::snprintf(sz, 256, "Inf Value: %+10.2f", -xiiMath::Infinity<float>());
    XII_TEST_STRING(sz, "Inf Value:  -Infinity");

    // extended stuff
    xiiStringUtils::snprintf(sz, 256, "size: %zu", (size_t)12345678);
    XII_TEST_STRING(sz, "size: 12345678");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StartsWith")
  {
    XII_TEST_BOOL(xiiStringUtils::StartsWith(nullptr, nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith(nullptr, "") == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith("", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith("", "") == true);

    XII_TEST_BOOL(xiiStringUtils::StartsWith("abc", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith("abc", "") == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith(nullptr, "abc") == false);
    XII_TEST_BOOL(xiiStringUtils::StartsWith("", "abc") == false);

    XII_TEST_BOOL(xiiStringUtils::StartsWith("abc", "abc") == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith("abcdef", "abc") == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith("abcdef", "Abc") == false);

    // substring test
    const char*     sz          = reinterpret_cast<const char*>(u8"äbc def ghi");
    const xiiUInt32 uiByteCount = xiiStringUtils::GetStringElementCount(u8"äbc");

    XII_TEST_BOOL(xiiStringUtils::StartsWith(sz, reinterpret_cast<const char*>(u8"äbc"), sz + uiByteCount) == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith(sz, reinterpret_cast<const char*>(u8"äbc"), sz + uiByteCount - 1) == false);
    XII_TEST_BOOL(xiiStringUtils::StartsWith(sz, reinterpret_cast<const char*>(u8"äbc"), sz + 0) == false);

    const char* sz2 = reinterpret_cast<const char*>(u8"äbc def");
    XII_TEST_BOOL(xiiStringUtils::StartsWith(sz, sz2, sz + uiByteCount, sz2 + uiByteCount) == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StartsWith_NoCase")
  {
    xiiStringUtf8 sL(L"äöü");
    xiiStringUtf8 sU(L"ÄÖÜ");

    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase(nullptr, nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase(nullptr, "") == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase("", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase("", "") == true);

    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase("abc", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase("abc", "") == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase(nullptr, "abc") == false);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase("", "abc") == false);

    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase("abc", "ABC") == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase("aBCdef", "abc") == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase("aBCdef", "bc") == false);

    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase(sL.GetData(), sU.GetData()) == true);

    // substring test
    const char*     sz          = reinterpret_cast<const char*>(u8"äbc def ghi");
    const xiiUInt32 uiByteCount = xiiStringUtils::GetStringElementCount(reinterpret_cast<const char*>(u8"äbc"));
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase(sz, reinterpret_cast<const char*>(u8"ÄBC"), sz + uiByteCount - 1) == false);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase(sz, reinterpret_cast<const char*>(u8"ÄBC"), sz + uiByteCount) == true);
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase(sz, reinterpret_cast<const char*>(u8"ÄBC"), sz + 0) == false);

    const char* sz2 = reinterpret_cast<const char*>(u8"Äbc def");
    XII_TEST_BOOL(xiiStringUtils::StartsWith_NoCase(sz, sz2, sz + uiByteCount, sz2 + uiByteCount) == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "EndsWith")
  {
    XII_TEST_BOOL(xiiStringUtils::EndsWith(nullptr, nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith(nullptr, "") == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith("", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith("", "") == true);

    XII_TEST_BOOL(xiiStringUtils::EndsWith("abc", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith("abc", "") == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith(nullptr, "abc") == false);
    XII_TEST_BOOL(xiiStringUtils::EndsWith("", "abc") == false);

    XII_TEST_BOOL(xiiStringUtils::EndsWith("abc", "abc") == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith("abcdef", "def") == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith("abcdef", "Def") == false);
    XII_TEST_BOOL(xiiStringUtils::EndsWith("def", "abcdef") == false);

    // substring test
    const char* sz = "abc def ghi";
    XII_TEST_BOOL(xiiStringUtils::EndsWith(sz, "abc", sz + 3) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith(sz, "def", sz + 7) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith(sz, "def", sz + 8) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "EndsWith_NoCase")
  {
    xiiStringUtf8 sL(L"äöü");
    xiiStringUtf8 sU(L"ÄÖÜ");

    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase(nullptr, nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase(nullptr, "") == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("", "") == true);

    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("abc", nullptr) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("abc", "") == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase(nullptr, "abc") == false);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("", "abc") == false);

    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("abc", "abc") == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("abcdef", "def") == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("abcdef", "Def") == true);

    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase("def", "abcdef") == false);

    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase(sL.GetData(), sU.GetData()) == true);

    // substring test
    const char* sz = "abc def ghi";
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase(sz, "ABC", sz + 3) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase(sz, "DEF", sz + 7) == true);
    XII_TEST_BOOL(xiiStringUtils::EndsWith_NoCase(sz, "DEF", sz + 8) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindSubString")
  {
    xiiStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    xiiStringUtf8 s2(L"äöü");
    xiiStringUtf8 s3(L"äöü2");

    const char* szABC = "abc";

    XII_TEST_BOOL(xiiStringUtils::FindSubString(szABC, szABC) == szABC);
    XII_TEST_BOOL(xiiStringUtils::FindSubString("abc", "") == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindSubString("abc", nullptr) == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindSubString(nullptr, "abc") == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindSubString("", "abc") == nullptr);

    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), "abc") == s.GetData());
    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), "def") == &s.GetData()[4]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), "ghi") == &s.GetData()[8]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), s2.GetData()) == &s.GetData()[12]);

    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), "abc2") == &s.GetData()[30]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), "def2") == &s.GetData()[35]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), "ghi2") == &s.GetData()[40]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), s3.GetData()) == &s.GetData()[45]);

    // substring test
    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), "abc2", s.GetData() + 34) == &s.GetData()[30]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString(s.GetData(), "abc2", s.GetData() + 33) == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindSubString_NoCase")
  {
    xiiStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    xiiStringUtf8 s2(L"äÖü");
    xiiStringUtf8 s3(L"ÄöÜ2");

    const char* szABC = "abc";

    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(szABC, "aBc") == szABC);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase("abc", "") == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase("abc", nullptr) == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(nullptr, "abc") == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase("", "abc") == nullptr);

    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), "Abc") == s.GetData());
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), "dEf") == &s.GetData()[4]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), "ghI") == &s.GetData()[8]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), s2.GetData()) == &s.GetData()[12]);

    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), "abC2") == &s.GetData()[30]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), "dEf2") == &s.GetData()[35]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), "Ghi2") == &s.GetData()[40]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), s3.GetData()) == &s.GetData()[45]);

    // substring test
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), "aBc2", s.GetData() + 34) == &s.GetData()[30]);
    XII_TEST_BOOL(xiiStringUtils::FindSubString_NoCase(s.GetData(), "abC2", s.GetData() + 33) == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindLastSubString")
  {
    xiiStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    xiiStringUtf8 s2(L"äöü");
    xiiStringUtf8 s3(L"äöü2");

    const char* szABC = "abc";

    XII_TEST_BOOL(xiiStringUtils::FindLastSubString(szABC, szABC) == szABC);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString("abc", "") == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString("abc", nullptr) == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString(nullptr, "abc") == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString("", "abc") == nullptr);

    XII_TEST_BOOL(xiiStringUtils::FindLastSubString(s.GetData(), "abc") == &s.GetData()[30]);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString(s.GetData(), "def") == &s.GetData()[35]);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString(s.GetData(), "ghi") == &s.GetData()[40]);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString(s.GetData(), s2.GetData()) == &s.GetData()[45]);

    // substring test
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString(s.GetData(), "abc", nullptr, s.GetData() + 33) == &s.GetData()[30]);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString(s.GetData(), "abc", nullptr, s.GetData() + 32) == &s.GetData()[0]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindLastSubString_NoCase")
  {
    xiiStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    xiiStringUtf8 s2(L"äÖü");
    xiiStringUtf8 s3(L"ÄöÜ2");

    const char* szABC = "abc";

    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase(szABC, "aBC") == szABC);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase("abc", "") == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase("abc", nullptr) == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase(nullptr, "abc") == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase("", "abc") == nullptr);

    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase(s.GetData(), "Abc") == &s.GetData()[30]);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase(s.GetData(), "dEf") == &s.GetData()[35]);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase(s.GetData(), "ghI") == &s.GetData()[40]);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase(s.GetData(), s2.GetData()) == &s.GetData()[45]);

    // substring test
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase(s.GetData(), "ABC", nullptr, s.GetData() + 33) == &s.GetData()[30]);
    XII_TEST_BOOL(xiiStringUtils::FindLastSubString_NoCase(s.GetData(), "ABC", nullptr, s.GetData() + 32) == &s.GetData()[0]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindWholeWord")
  {
    xiiStringUtf8 s(L"mompfhüßß ßßß öäü abcdef abc def");

    XII_TEST_BOOL(xiiStringUtils::FindWholeWord(s.GetData(), "abc", xiiStringUtils::IsWordDelimiter_English) == &s.GetData()[34]);
    XII_TEST_BOOL(xiiStringUtils::FindWholeWord(s.GetData(), "def", xiiStringUtils::IsWordDelimiter_English) == &s.GetData()[38]);
    XII_TEST_BOOL(xiiStringUtils::FindWholeWord(s.GetData(), "mompfh", xiiStringUtils::IsWordDelimiter_English) == &s.GetData()[0]); // ü is not english

    // substring test
    XII_TEST_BOOL(xiiStringUtils::FindWholeWord(s.GetData(), "abc", xiiStringUtils::IsWordDelimiter_English, s.GetData() + 37) == &s.GetData()[34]);
    XII_TEST_BOOL(xiiStringUtils::FindWholeWord(s.GetData(), "abc", xiiStringUtils::IsWordDelimiter_English, s.GetData() + 36) == nullptr);
    XII_TEST_BOOL(xiiStringUtils::FindWholeWord(s.GetData(), "abc", xiiStringUtils::IsWordDelimiter_English, s.GetData() + 30) == s.GetData() + 27);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindWholeWord_NoCase")
  {
    xiiStringUtf8 s(L"mompfhüßß ßßß öäü abcdef abc def");

    XII_TEST_BOOL(xiiStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", xiiStringUtils::IsWordDelimiter_English) == &s.GetData()[34]);
    XII_TEST_BOOL(xiiStringUtils::FindWholeWord_NoCase(s.GetData(), "DEF", xiiStringUtils::IsWordDelimiter_English) == &s.GetData()[38]);
    XII_TEST_BOOL(xiiStringUtils::FindWholeWord_NoCase(s.GetData(), "momPFH", xiiStringUtils::IsWordDelimiter_English) == &s.GetData()[0]);

    // substring test
    XII_TEST_BOOL(
      xiiStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", xiiStringUtils::IsWordDelimiter_English, s.GetData() + 37) == &s.GetData()[34]);
    XII_TEST_BOOL(xiiStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", xiiStringUtils::IsWordDelimiter_English, s.GetData() + 36) == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindUIntAtTheEnd")
  {
    xiiUInt32 uiTestValue           = 0;
    xiiUInt32 uiCharactersFromStart = 0;

    XII_TEST_BOOL(xiiStringUtils::FindUIntAtTheEnd(nullptr, uiTestValue, &uiCharactersFromStart).Failed());

    xiiStringUtf8 noNumberAtTheEnd(L"ThisStringContainsNoNumberAtTheEnd");
    XII_TEST_BOOL(xiiStringUtils::FindUIntAtTheEnd(noNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Failed());

    xiiStringUtf8 noNumberAtTheEnd2(L"ThisStringContainsNoNumberAtTheEndBut42InBetween");
    XII_TEST_BOOL(xiiStringUtils::FindUIntAtTheEnd(noNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Failed());

    xiiStringUtf8 aNumberAtTheEnd(L"ThisStringContainsANumberAtTheEnd1");
    XII_TEST_BOOL(xiiStringUtils::FindUIntAtTheEnd(aNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    XII_TEST_INT(uiTestValue, 1);
    XII_TEST_INT(uiCharactersFromStart, aNumberAtTheEnd.GetElementCount() - 1);

    xiiStringUtf8 aZeroLeadingNumberAtTheEnd(L"ThisStringContainsANumberAtTheEnd011129");
    XII_TEST_BOOL(xiiStringUtils::FindUIntAtTheEnd(aZeroLeadingNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    XII_TEST_INT(uiTestValue, 11129);
    XII_TEST_INT(uiCharactersFromStart, aZeroLeadingNumberAtTheEnd.GetElementCount() - 6);

    XII_TEST_BOOL(xiiStringUtils::FindUIntAtTheEnd(aNumberAtTheEnd.GetData(), uiTestValue, nullptr).Succeeded());
    XII_TEST_INT(uiTestValue, 1);

    xiiStringUtf8 twoNumbersInOneString(L"FirstANumber23AndThen42");
    XII_TEST_BOOL(xiiStringUtils::FindUIntAtTheEnd(twoNumbersInOneString.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    XII_TEST_INT(uiTestValue, 42);

    xiiStringUtf8 onlyANumber(L"55566553");
    XII_TEST_BOOL(xiiStringUtils::FindUIntAtTheEnd(onlyANumber.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    XII_TEST_INT(uiTestValue, 55566553);
    XII_TEST_INT(uiCharactersFromStart, 0);
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SkipCharacters")
  {
    xiiStringUtf8 s(L"mompf   hüßß ßßß öäü abcdef abc def");
    const char*   szEmpty = "";

    XII_TEST_BOOL(xiiStringUtils::SkipCharacters(s.GetData(), xiiStringUtils::IsWhiteSpace, false) == &s.GetData()[0]);
    XII_TEST_BOOL(xiiStringUtils::SkipCharacters(s.GetData(), xiiStringUtils::IsWhiteSpace, true) == &s.GetData()[1]);
    XII_TEST_BOOL(xiiStringUtils::SkipCharacters(&s.GetData()[5], xiiStringUtils::IsWhiteSpace, false) == &s.GetData()[8]);
    XII_TEST_BOOL(xiiStringUtils::SkipCharacters(&s.GetData()[5], xiiStringUtils::IsWhiteSpace, true) == &s.GetData()[8]);
    XII_TEST_BOOL(xiiStringUtils::SkipCharacters(szEmpty, xiiStringUtils::IsWhiteSpace, false) == szEmpty);
    XII_TEST_BOOL(xiiStringUtils::SkipCharacters(szEmpty, xiiStringUtils::IsWhiteSpace, true) == szEmpty);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindWordEnd")
  {
    xiiStringUtf8 s(L"mompf   hüßß ßßß öäü abcdef abc def");
    const char*   szEmpty = "";

    XII_TEST_BOOL(xiiStringUtils::FindWordEnd(s.GetData(), xiiStringUtils::IsWhiteSpace, true) == &s.GetData()[5]);
    XII_TEST_BOOL(xiiStringUtils::FindWordEnd(s.GetData(), xiiStringUtils::IsWhiteSpace, false) == &s.GetData()[5]);
    XII_TEST_BOOL(xiiStringUtils::FindWordEnd(&s.GetData()[5], xiiStringUtils::IsWhiteSpace, true) == &s.GetData()[6]);
    XII_TEST_BOOL(xiiStringUtils::FindWordEnd(&s.GetData()[5], xiiStringUtils::IsWhiteSpace, false) == &s.GetData()[5]);
    XII_TEST_BOOL(xiiStringUtils::FindWordEnd(szEmpty, xiiStringUtils::IsWhiteSpace, true) == szEmpty);
    XII_TEST_BOOL(xiiStringUtils::FindWordEnd(szEmpty, xiiStringUtils::IsWhiteSpace, false) == szEmpty);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsWhitespace")
  {
    XII_TEST_BOOL(xiiStringUtils::IsWhiteSpace(' '));
    XII_TEST_BOOL(xiiStringUtils::IsWhiteSpace('\t'));
    XII_TEST_BOOL(xiiStringUtils::IsWhiteSpace('\n'));
    XII_TEST_BOOL(xiiStringUtils::IsWhiteSpace('\r'));
    XII_TEST_BOOL(xiiStringUtils::IsWhiteSpace('\v'));

    XII_TEST_BOOL(xiiStringUtils::IsWhiteSpace('\0') == false);

    for (xiiUInt32 i = 33; i < 256; ++i)
    {
      XII_TEST_BOOL(xiiStringUtils::IsWhiteSpace(i) == false);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsDecimalDigit / IsHexDigit")
  {
    XII_TEST_BOOL(xiiStringUtils::IsDecimalDigit('0'));
    XII_TEST_BOOL(xiiStringUtils::IsDecimalDigit('4'));
    XII_TEST_BOOL(xiiStringUtils::IsDecimalDigit('9'));
    XII_TEST_BOOL(!xiiStringUtils::IsDecimalDigit('/'));
    XII_TEST_BOOL(!xiiStringUtils::IsDecimalDigit('A'));

    XII_TEST_BOOL(xiiStringUtils::IsHexDigit('0'));
    XII_TEST_BOOL(xiiStringUtils::IsHexDigit('4'));
    XII_TEST_BOOL(xiiStringUtils::IsHexDigit('9'));
    XII_TEST_BOOL(xiiStringUtils::IsHexDigit('A'));
    XII_TEST_BOOL(xiiStringUtils::IsHexDigit('E'));
    XII_TEST_BOOL(xiiStringUtils::IsHexDigit('a'));
    XII_TEST_BOOL(xiiStringUtils::IsHexDigit('f'));
    XII_TEST_BOOL(!xiiStringUtils::IsHexDigit('g'));
    XII_TEST_BOOL(!xiiStringUtils::IsHexDigit('/'));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsWordDelimiter_English / IsIdentifierDelimiter_C_Code")
  {
    for (xiiUInt32 i = 0; i < 256; ++i)
    {
      const bool alpha      = (i >= 'a' && i <= 'z');
      const bool alpha2     = (i >= 'A' && i <= 'Z');
      const bool num        = (i >= '0' && i <= '9');
      const bool dash       = i == '-';
      const bool underscore = i == '_';

      const bool bCode = alpha || alpha2 || num || underscore;
      const bool bWord = bCode || dash;


      XII_TEST_BOOL(xiiStringUtils::IsWordDelimiter_English(i) == !bWord);
      XII_TEST_BOOL(xiiStringUtils::IsIdentifierDelimiter_C_Code(i) == !bCode);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValidIdentifierName")
  {
    XII_TEST_BOOL(!xiiStringUtils::IsValidIdentifierName(""));
    XII_TEST_BOOL(!xiiStringUtils::IsValidIdentifierName("1asdf"));
    XII_TEST_BOOL(!xiiStringUtils::IsValidIdentifierName("as df"));
    XII_TEST_BOOL(!xiiStringUtils::IsValidIdentifierName("asdf!"));

    XII_TEST_BOOL(xiiStringUtils::IsValidIdentifierName("asdf1"));
    XII_TEST_BOOL(xiiStringUtils::IsValidIdentifierName("_asdf"));
  }
}
