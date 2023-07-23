#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

XII_CREATE_SIMPLE_TEST(Strings, StringBase)
{
  // These tests need not be very through, as xiiStringBase only passes through to xiiStringUtil
  // which has been tested elsewhere already.
  // Here it is only assured that xiiStringBases passes its own pointers properly through,
  // such that the xiiStringUtil functions are called correctly.

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEmpty")
  {
    xiiStringView it(nullptr);
    XII_TEST_BOOL(it.IsEmpty());

    xiiStringView it2("");
    XII_TEST_BOOL(it2.IsEmpty());

    xiiStringView it3(nullptr, nullptr);
    XII_TEST_BOOL(it3.IsEmpty());

    const char* sz = "abcdef";

    xiiStringView it4(sz, sz);
    XII_TEST_BOOL(it4.IsEmpty());

    xiiStringView it5(sz, sz + 1);
    XII_TEST_BOOL(!it5.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StartsWith")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.StartsWith("abc"));
    XII_TEST_BOOL(it.StartsWith("abcdef"));
    XII_TEST_BOOL(it.StartsWith("")); // empty strings always return true

    xiiStringView it2(sz + 3);

    XII_TEST_BOOL(it2.StartsWith("def"));
    XII_TEST_BOOL(it2.StartsWith(""));

    xiiStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    XII_TEST_BOOL(it3.StartsWith("d"));
    XII_TEST_BOOL(!it3.StartsWith("de"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StartsWith_NoCase")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.StartsWith_NoCase("ABC"));
    XII_TEST_BOOL(it.StartsWith_NoCase("abcDEF"));
    XII_TEST_BOOL(it.StartsWith_NoCase("")); // empty strings always return true

    xiiStringView it2(sz + 3);

    XII_TEST_BOOL(it2.StartsWith_NoCase("DEF"));
    XII_TEST_BOOL(it2.StartsWith_NoCase(""));

    xiiStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    XII_TEST_BOOL(it3.StartsWith_NoCase("D"));
    XII_TEST_BOOL(!it3.StartsWith_NoCase("DE"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "EndsWith")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.EndsWith("def"));
    XII_TEST_BOOL(it.EndsWith("abcdef"));
    XII_TEST_BOOL(it.EndsWith("")); // empty strings always return true

    xiiStringView it2(sz + 3);

    XII_TEST_BOOL(it2.EndsWith("def"));
    XII_TEST_BOOL(it2.EndsWith(""));

    xiiStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    XII_TEST_BOOL(it3.EndsWith("d"));
    XII_TEST_BOOL(!it3.EndsWith("cd"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "EndsWith_NoCase")
  {
    const char*   sz = "ABCDEF";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.EndsWith_NoCase("def"));
    XII_TEST_BOOL(it.EndsWith_NoCase("abcdef"));
    XII_TEST_BOOL(it.EndsWith_NoCase("")); // empty strings always return true

    xiiStringView it2(sz + 3);

    XII_TEST_BOOL(it2.EndsWith_NoCase("def"));
    XII_TEST_BOOL(it2.EndsWith_NoCase(""));

    xiiStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    XII_TEST_BOOL(it3.EndsWith_NoCase("d"));
    XII_TEST_BOOL(!it3.EndsWith_NoCase("cd"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindSubString")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.FindSubString("abcdef") == sz);
    XII_TEST_BOOL(it.FindSubString("abc") == sz);
    XII_TEST_BOOL(it.FindSubString("def") == sz + 3);
    XII_TEST_BOOL(it.FindSubString("cd") == sz + 2);
    XII_TEST_BOOL(it.FindSubString("") == nullptr);
    XII_TEST_BOOL(it.FindSubString(nullptr) == nullptr);
    XII_TEST_BOOL(it.FindSubString("g") == nullptr);

    XII_TEST_BOOL(it.FindSubString("abcdef", sz) == sz);
    XII_TEST_BOOL(it.FindSubString("abcdef", sz + 1) == nullptr);
    XII_TEST_BOOL(it.FindSubString("def", sz + 2) == sz + 3);
    XII_TEST_BOOL(it.FindSubString("def", sz + 3) == sz + 3);
    XII_TEST_BOOL(it.FindSubString("def", sz + 4) == nullptr);
    XII_TEST_BOOL(it.FindSubString("", sz + 3) == nullptr);

    xiiStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    XII_TEST_BOOL(it2.FindSubString("abcdef") == nullptr);
    XII_TEST_BOOL(it2.FindSubString("abc") == nullptr);
    XII_TEST_BOOL(it2.FindSubString("de") == sz + 3);
    XII_TEST_BOOL(it2.FindSubString("cd") == sz + 2);
    XII_TEST_BOOL(it2.FindSubString("") == nullptr);
    XII_TEST_BOOL(it2.FindSubString(nullptr) == nullptr);
    XII_TEST_BOOL(it2.FindSubString("g") == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindSubString_NoCase")
  {
    const char*   sz = "ABCDEF";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.FindSubString_NoCase("abcdef") == sz);
    XII_TEST_BOOL(it.FindSubString_NoCase("abc") == sz);
    XII_TEST_BOOL(it.FindSubString_NoCase("def") == sz + 3);
    XII_TEST_BOOL(it.FindSubString_NoCase("cd") == sz + 2);
    XII_TEST_BOOL(it.FindSubString_NoCase("") == nullptr);
    XII_TEST_BOOL(it.FindSubString_NoCase(nullptr) == nullptr);
    XII_TEST_BOOL(it.FindSubString_NoCase("g") == nullptr);

    XII_TEST_BOOL(it.FindSubString_NoCase("abcdef", sz) == sz);
    XII_TEST_BOOL(it.FindSubString_NoCase("abcdef", sz + 1) == nullptr);
    XII_TEST_BOOL(it.FindSubString_NoCase("def", sz + 2) == sz + 3);
    XII_TEST_BOOL(it.FindSubString_NoCase("def", sz + 3) == sz + 3);
    XII_TEST_BOOL(it.FindSubString_NoCase("def", sz + 4) == nullptr);
    XII_TEST_BOOL(it.FindSubString_NoCase("", sz + 3) == nullptr);


    xiiStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    XII_TEST_BOOL(it2.FindSubString_NoCase("abcdef") == nullptr);
    XII_TEST_BOOL(it2.FindSubString_NoCase("abc") == nullptr);
    XII_TEST_BOOL(it2.FindSubString_NoCase("de") == sz + 3);
    XII_TEST_BOOL(it2.FindSubString_NoCase("cd") == sz + 2);
    XII_TEST_BOOL(it2.FindSubString_NoCase("") == nullptr);
    XII_TEST_BOOL(it2.FindSubString_NoCase(nullptr) == nullptr);
    XII_TEST_BOOL(it2.FindSubString_NoCase("g") == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindLastSubString")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.FindLastSubString("abcdef") == sz);
    XII_TEST_BOOL(it.FindLastSubString("abc") == sz);
    XII_TEST_BOOL(it.FindLastSubString("def") == sz + 3);
    XII_TEST_BOOL(it.FindLastSubString("cd") == sz + 2);
    XII_TEST_BOOL(it.FindLastSubString("") == nullptr);
    XII_TEST_BOOL(it.FindLastSubString(nullptr) == nullptr);
    XII_TEST_BOOL(it.FindLastSubString("g") == nullptr);

    xiiStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    XII_TEST_BOOL(it2.FindLastSubString("abcdef") == nullptr);
    XII_TEST_BOOL(it2.FindLastSubString("abc") == nullptr);
    XII_TEST_BOOL(it2.FindLastSubString("de") == sz + 3);
    XII_TEST_BOOL(it2.FindLastSubString("cd") == sz + 2);
    XII_TEST_BOOL(it2.FindLastSubString("") == nullptr);
    XII_TEST_BOOL(it2.FindLastSubString(nullptr) == nullptr);
    XII_TEST_BOOL(it2.FindLastSubString("g") == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindLastSubString_NoCase")
  {
    const char*   sz = "ABCDEF";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.FindLastSubString_NoCase("abcdef") == sz);
    XII_TEST_BOOL(it.FindLastSubString_NoCase("abc") == sz);
    XII_TEST_BOOL(it.FindLastSubString_NoCase("def") == sz + 3);
    XII_TEST_BOOL(it.FindLastSubString_NoCase("cd") == sz + 2);
    XII_TEST_BOOL(it.FindLastSubString_NoCase("") == nullptr);
    XII_TEST_BOOL(it.FindLastSubString_NoCase(nullptr) == nullptr);
    XII_TEST_BOOL(it.FindLastSubString_NoCase("g") == nullptr);

    xiiStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    XII_TEST_BOOL(it2.FindLastSubString_NoCase("abcdef") == nullptr);
    XII_TEST_BOOL(it2.FindLastSubString_NoCase("abc") == nullptr);
    XII_TEST_BOOL(it2.FindLastSubString_NoCase("de") == sz + 3);
    XII_TEST_BOOL(it2.FindLastSubString_NoCase("cd") == sz + 2);
    XII_TEST_BOOL(it2.FindLastSubString_NoCase("") == nullptr);
    XII_TEST_BOOL(it2.FindLastSubString_NoCase(nullptr) == nullptr);
    XII_TEST_BOOL(it2.FindLastSubString_NoCase("g") == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compare")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.Compare("abcdef") == 0);
    XII_TEST_BOOL(it.Compare("abcde") > 0);
    XII_TEST_BOOL(it.Compare("abcdefg") < 0);

    xiiStringView it2(sz + 2, sz + 5);
    it2.SetStartPosition(sz + 3);

    XII_TEST_BOOL(it2.Compare("de") == 0);
    XII_TEST_BOOL(it2.Compare("def") < 0);
    XII_TEST_BOOL(it2.Compare("d") > 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compare_NoCase")
  {
    const char*   sz = "ABCDEF";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.Compare_NoCase("abcdef") == 0);
    XII_TEST_BOOL(it.Compare_NoCase("abcde") > 0);
    XII_TEST_BOOL(it.Compare_NoCase("abcdefg") < 0);

    xiiStringView it2(sz + 2, sz + 5);
    it2.SetStartPosition(sz + 3);

    XII_TEST_BOOL(it2.Compare_NoCase("de") == 0);
    XII_TEST_BOOL(it2.Compare_NoCase("def") < 0);
    XII_TEST_BOOL(it2.Compare_NoCase("d") > 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompareN")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.CompareN("abc", 3) == 0);
    XII_TEST_BOOL(it.CompareN("abcde", 6) > 0);
    XII_TEST_BOOL(it.CompareN("abcg", 3) == 0);

    xiiStringView it2(sz + 2, sz + 5);

    XII_TEST_BOOL(it2.CompareN("cd", 2) == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompareN_NoCase")
  {
    const char*   sz = "ABCDEF";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.CompareN_NoCase("abc", 3) == 0);
    XII_TEST_BOOL(it.CompareN_NoCase("abcde", 6) > 0);
    XII_TEST_BOOL(it.CompareN_NoCase("abcg", 3) == 0);

    xiiStringView it2(sz + 2, sz + 5);

    XII_TEST_BOOL(it2.CompareN_NoCase("cd", 2) == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.IsEqual("abcdef"));
    XII_TEST_BOOL(!it.IsEqual("abcde"));
    XII_TEST_BOOL(!it.IsEqual("abcdefg"));

    xiiStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    XII_TEST_BOOL(it2.IsEqual("cde"));
    XII_TEST_BOOL(!it2.IsEqual("bcde"));
    XII_TEST_BOOL(!it2.IsEqual("cdef"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual_NoCase")
  {
    const char*   sz = "ABCDEF";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.IsEqual_NoCase("abcdef"));
    XII_TEST_BOOL(!it.IsEqual_NoCase("abcde"));
    XII_TEST_BOOL(!it.IsEqual_NoCase("abcdefg"));

    xiiStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    XII_TEST_BOOL(it2.IsEqual_NoCase("cde"));
    XII_TEST_BOOL(!it2.IsEqual_NoCase("bcde"));
    XII_TEST_BOOL(!it2.IsEqual_NoCase("cdef"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqualN")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.IsEqualN("abcGHI", 3));
    XII_TEST_BOOL(!it.IsEqualN("abcGHI", 4));

    xiiStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    XII_TEST_BOOL(it2.IsEqualN("cdeZX", 3));
    XII_TEST_BOOL(!it2.IsEqualN("cdeZX", 4));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqualN_NoCase")
  {
    const char*   sz = "ABCDEF";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.IsEqualN_NoCase("abcGHI", 3));
    XII_TEST_BOOL(!it.IsEqualN_NoCase("abcGHI", 4));

    xiiStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    XII_TEST_BOOL(it2.IsEqualN_NoCase("cdeZX", 3));
    XII_TEST_BOOL(!it2.IsEqualN_NoCase("cdeZX", 4));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator==/!=")
  {
    const char*   sz  = "abcdef";
    const char*   sz2 = "blabla";
    xiiStringView it(sz);
    xiiStringView it2(sz);
    xiiStringView it3(sz2);

    XII_TEST_BOOL(it == sz);
    XII_TEST_BOOL(sz == it);
    XII_TEST_BOOL(it == "abcdef");
    XII_TEST_BOOL("abcdef" == it);
    XII_TEST_BOOL(it == it);
    XII_TEST_BOOL(it == it2);

    XII_TEST_BOOL(it != sz2);
    XII_TEST_BOOL(sz2 != it);
    XII_TEST_BOOL(it != "blabla");
    XII_TEST_BOOL("blabla" != it);
    XII_TEST_BOOL(it != it3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "substring operator ==/!=/</>/<=/>=")
  {
    const char* sz1 = "aaabbbcccddd";
    const char* sz2 = "aaabbbdddeee";

    xiiStringView it1(sz1 + 3, sz1 + 6);
    xiiStringView it2(sz2 + 3, sz2 + 6);

    XII_TEST_BOOL(it1 == it1);
    XII_TEST_BOOL(it2 == it2);

    XII_TEST_BOOL(it1 == it2);
    XII_TEST_BOOL(!(it1 != it2));
    XII_TEST_BOOL(!(it1 < it2));
    XII_TEST_BOOL(!(it1 > it2));
    XII_TEST_BOOL(it1 <= it2);
    XII_TEST_BOOL(it1 >= it2);

    it1 = xiiStringView(sz1 + 3, sz1 + 7);
    it2 = xiiStringView(sz2 + 3, sz2 + 7);

    XII_TEST_BOOL(it1 == it1);
    XII_TEST_BOOL(it2 == it2);

    XII_TEST_BOOL(it1 != it2);
    XII_TEST_BOOL(!(it1 == it2));

    XII_TEST_BOOL(it1 < it2);
    XII_TEST_BOOL(!(it1 > it2));
    XII_TEST_BOOL(it1 <= it2);
    XII_TEST_BOOL(!(it1 >= it2));

    XII_TEST_BOOL(it2 > it1);
    XII_TEST_BOOL(!(it2 < it1));
    XII_TEST_BOOL(it2 >= it1);
    XII_TEST_BOOL(!(it2 <= it1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator</>")
  {
    const char*   sz  = "abcdef";
    const char*   sz2 = "abcdefg";
    xiiStringView it(sz);
    xiiStringView it2(sz2);

    XII_TEST_BOOL(it < sz2);
    XII_TEST_BOOL(sz < it2);
    XII_TEST_BOOL(it < it2);

    XII_TEST_BOOL(sz2 > it);
    XII_TEST_BOOL(it2 > sz);
    XII_TEST_BOOL(it2 > it);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator<=/>=")
  {
    {
      const char*   sz  = "abcdef";
      const char*   sz2 = "abcdefg";
      xiiStringView it(sz);
      xiiStringView it2(sz2);

      XII_TEST_BOOL(it <= sz2);
      XII_TEST_BOOL(sz <= it2);
      XII_TEST_BOOL(it <= it2);

      XII_TEST_BOOL(sz2 >= it);
      XII_TEST_BOOL(it2 >= sz);
      XII_TEST_BOOL(it2 >= it);
    }

    {
      const char*   sz  = "abcdef";
      const char*   sz2 = "abcdef";
      xiiStringView it(sz);
      xiiStringView it2(sz2);

      XII_TEST_BOOL(it <= sz2);
      XII_TEST_BOOL(sz <= it2);
      XII_TEST_BOOL(it <= it2);

      XII_TEST_BOOL(sz2 >= it);
      XII_TEST_BOOL(it2 >= sz);
      XII_TEST_BOOL(it2 >= it);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindWholeWord")
  {
    xiiStringUtf8 s(L"abc def mompfhüßß ßßß öäü abcdef abc def abc def");
    xiiStringView it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8);
    xiiStringView it2(s.GetData() + 8, s.GetData() + s.GetElementCount());

    XII_TEST_BOOL(it.FindWholeWord("abc", xiiStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[34]);
    XII_TEST_BOOL(it.FindWholeWord("def", xiiStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[38]);
    XII_TEST_BOOL(it.FindWholeWord("mompfh", xiiStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[0]); // ü is not English (thus a delimiter)

    XII_TEST_BOOL(it.FindWholeWord("abc", xiiStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    XII_TEST_BOOL(it.FindWholeWord("abc", xiiStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == nullptr);

    XII_TEST_BOOL(it2.FindWholeWord("abc", xiiStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    XII_TEST_BOOL(it2.FindWholeWord("abc", xiiStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == &it.GetStartPointer()[42]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindWholeWord_NoCase")
  {
    xiiStringUtf8 s(L"abc def mompfhüßß ßßß öäü abcdef abc def abc def");
    xiiStringView it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8);
    xiiStringView it2(s.GetData() + 8, s.GetData() + s.GetElementCount());

    XII_TEST_BOOL(it.FindWholeWord_NoCase("ABC", xiiStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[34]);
    XII_TEST_BOOL(it.FindWholeWord_NoCase("DEF", xiiStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[38]);
    XII_TEST_BOOL(it.FindWholeWord_NoCase("momPFH", xiiStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[0]);

    XII_TEST_BOOL(it.FindWholeWord_NoCase("ABc", xiiStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    XII_TEST_BOOL(it.FindWholeWord_NoCase("ABc", xiiStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == nullptr);

    XII_TEST_BOOL(it2.FindWholeWord_NoCase("ABc", xiiStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    XII_TEST_BOOL(it2.FindWholeWord_NoCase("ABc", xiiStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == &it.GetStartPointer()[42]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ComputeCharacterPosition")
  {
    const wchar_t*   sz = L"mompfhüßß ßßß öäü abcdef abc def abc def";
    xiiStringBuilder s(sz);

    XII_TEST_STRING(s.ComputeCharacterPosition(14), xiiStringUtf8(L"öäü abcdef abc def abc def").GetData());
  }
}
