#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

// This file takes ages to compile in a Release build
// since we don't care for runtime performance, just disable all optimizations.
#pragma optimize("", off)

XII_CREATE_SIMPLE_TEST(Strings, StringBuilder)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(empty)")
  {
    xiiStringBuilder s;

    XII_TEST_BOOL(s.IsEmpty());
    XII_TEST_INT(s.GetCharacterCount(), 0);
    XII_TEST_INT(s.GetElementCount(), 0);
    XII_TEST_BOOL(s.IsPureASCII());
    XII_TEST_BOOL(s == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(Utf8)")
  {
    xiiStringUtf8    sUtf8(L"abc äöü € def");
    xiiStringBuilder s(sUtf8.GetData());

    XII_TEST_BOOL(s.GetData() != sUtf8.GetData());
    XII_TEST_BOOL(s == sUtf8.GetData());
    XII_TEST_INT(s.GetElementCount(), 18);
    XII_TEST_INT(s.GetCharacterCount(), 13);
    XII_TEST_BOOL(!s.IsPureASCII());

    xiiStringBuilder s2("test test");

    XII_TEST_BOOL(s2 == "test test");
    XII_TEST_INT(s2.GetElementCount(), 9);
    XII_TEST_INT(s2.GetCharacterCount(), 9);
    XII_TEST_BOOL(s2.IsPureASCII());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(wchar_t)")
  {
    xiiStringUtf8    sUtf8(L"abc äöü € def");
    xiiStringBuilder s(L"abc äöü € def");

    XII_TEST_BOOL(s == sUtf8.GetData());
    XII_TEST_INT(s.GetElementCount(), 18);
    XII_TEST_INT(s.GetCharacterCount(), 13);
    XII_TEST_BOOL(!s.IsPureASCII());

    xiiStringBuilder s2(L"test test");

    XII_TEST_BOOL(s2 == "test test");
    XII_TEST_INT(s2.GetElementCount(), 9);
    XII_TEST_INT(s2.GetCharacterCount(), 9);
    XII_TEST_BOOL(s2.IsPureASCII());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(copy)")
  {
    xiiStringUtf8    sUtf8(L"abc äöü € def");
    xiiStringBuilder s(L"abc äöü € def");
    xiiStringBuilder s2(s);

    XII_TEST_BOOL(s2 == sUtf8.GetData());
    XII_TEST_INT(s2.GetElementCount(), 18);
    XII_TEST_INT(s2.GetCharacterCount(), 13);
    XII_TEST_BOOL(!s2.IsPureASCII());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(StringView)")
  {
    xiiStringUtf8 sUtf8(L"abc äöü € def");

    xiiStringView it(sUtf8.GetData() + 2, sUtf8.GetData() + 8);

    xiiStringBuilder s(it);

    XII_TEST_INT(s.GetElementCount(), 6);
    XII_TEST_INT(s.GetCharacterCount(), 4);
    XII_TEST_BOOL(!s.IsPureASCII());
    XII_TEST_BOOL(s == xiiStringUtf8(L"c äö").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor(multiple)")
  {
    xiiStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    xiiStringUtf8 sUtf2(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");

    xiiStringBuilder sb(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData());

    XII_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=(Utf8)")
  {
    xiiStringUtf8    sUtf8(L"abc äöü € def");
    xiiStringBuilder s("bla");
    s = sUtf8.GetData();

    XII_TEST_BOOL(s.GetData() != sUtf8.GetData());
    XII_TEST_BOOL(s == sUtf8.GetData());
    XII_TEST_INT(s.GetElementCount(), 18);
    XII_TEST_INT(s.GetCharacterCount(), 13);
    XII_TEST_BOOL(!s.IsPureASCII());

    xiiStringBuilder s2("bla");
    s2 = "test test";

    XII_TEST_BOOL(s2 == "test test");
    XII_TEST_INT(s2.GetElementCount(), 9);
    XII_TEST_INT(s2.GetCharacterCount(), 9);
    XII_TEST_BOOL(s2.IsPureASCII());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=(wchar_t)")
  {
    xiiStringUtf8    sUtf8(L"abc äöü € def");
    xiiStringBuilder s("bla");
    s = L"abc äöü € def";

    XII_TEST_BOOL(s == sUtf8.GetData());
    XII_TEST_INT(s.GetElementCount(), 18);
    XII_TEST_INT(s.GetCharacterCount(), 13);
    XII_TEST_BOOL(!s.IsPureASCII());

    xiiStringBuilder s2("bla");
    s2 = L"test test";

    XII_TEST_BOOL(s2 == "test test");
    XII_TEST_INT(s2.GetElementCount(), 9);
    XII_TEST_INT(s2.GetCharacterCount(), 9);
    XII_TEST_BOOL(s2.IsPureASCII());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=(copy)")
  {
    xiiStringUtf8    sUtf8(L"abc äöü € def");
    xiiStringBuilder s(L"abc äöü € def");
    xiiStringBuilder s2;
    s2 = s;

    XII_TEST_BOOL(s2 == sUtf8.GetData());
    XII_TEST_INT(s2.GetElementCount(), 18);
    XII_TEST_INT(s2.GetCharacterCount(), 13);
    XII_TEST_BOOL(!s2.IsPureASCII());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=(StringView)")
  {
    xiiStringBuilder s("abcdefghi");
    xiiStringView    it(s.GetData() + 2, s.GetData() + 8);
    it.SetStartPosition(s.GetData() + 3);

    s = it;

    XII_TEST_BOOL(s == "defgh");
    XII_TEST_INT(s.GetElementCount(), 5);
    XII_TEST_INT(s.GetCharacterCount(), 5);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "convert to xiiStringView")
  {
    xiiStringBuilder s(L"aölsdföasld");
    xiiStringBuilder tmp;

    xiiStringView sv = s;

    XII_TEST_STRING(sv.GetData(tmp), xiiStringUtf8(L"aölsdföasld").GetData());
    XII_TEST_BOOL(sv == xiiStringUtf8(L"aölsdföasld").GetData());

    s = "abcdef";

    XII_TEST_STRING(sv.GetStartPointer(), "abcdef");
    XII_TEST_BOOL(sv == "abcdef");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    xiiStringBuilder s(L"abc äöü € def");

    XII_TEST_BOOL(!s.IsEmpty());
    XII_TEST_BOOL(!s.IsPureASCII());

    s.Clear();
    XII_TEST_BOOL(s.IsEmpty());
    XII_TEST_INT(s.GetElementCount(), 0);
    XII_TEST_INT(s.GetCharacterCount(), 0);
    XII_TEST_BOOL(s.IsPureASCII());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetElementCount / GetCharacterCount / IsPureASCII")
  {
    xiiStringBuilder s(L"abc äöü € def");

    XII_TEST_BOOL(!s.IsPureASCII());
    XII_TEST_INT(s.GetElementCount(), 18);
    XII_TEST_INT(s.GetCharacterCount(), 13);

    s = "abc";

    XII_TEST_BOOL(s.IsPureASCII());
    XII_TEST_INT(s.GetElementCount(), 3);
    XII_TEST_INT(s.GetCharacterCount(), 3);

    s = L"Hällo! I love €";

    XII_TEST_BOOL(!s.IsPureASCII());
    XII_TEST_INT(s.GetElementCount(), 18);
    XII_TEST_INT(s.GetCharacterCount(), 15);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Append(single unicode char)")
  {
    xiiStringUtf32 u32(L"äöüß");

    xiiStringBuilder s("abc");
    XII_TEST_INT(s.GetCharacterCount(), 3);
    s.Append(u32.GetData()[0]);
    XII_TEST_INT(s.GetCharacterCount(), 4);

    XII_TEST_BOOL(s == xiiStringUtf8(L"abcä").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Prepend(single unicode char)")
  {
    xiiStringUtf32 u32(L"äöüß");

    xiiStringBuilder s("abc");
    XII_TEST_INT(s.GetCharacterCount(), 3);
    s.Prepend(u32.GetData()[0]);
    XII_TEST_INT(s.GetCharacterCount(), 4);

    XII_TEST_BOOL(s == xiiStringUtf8(L"äabc").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Append(char)")
  {
    xiiStringBuilder s("abc");
    XII_TEST_INT(s.GetCharacterCount(), 3);
    s.Append("de", "fg", "hi", xiiStringUtf8(L"öä").GetData(), "jk", xiiStringUtf8(L"ü€").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 15);

    XII_TEST_BOOL(s == xiiStringUtf8(L"abcdefghiöäjkü€").GetData());

    s = "pups";
    s.Append(nullptr, "b", nullptr, "d", nullptr, xiiStringUtf8(L"ü€").GetData());
    XII_TEST_BOOL(s == xiiStringUtf8(L"pupsbdü€").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Append(wchar_t)")
  {
    xiiStringBuilder s("abc");
    XII_TEST_INT(s.GetCharacterCount(), 3);
    s.Append(L"de", L"fg", L"hi", L"öä", L"jk", L"ü€");
    XII_TEST_INT(s.GetCharacterCount(), 15);

    XII_TEST_BOOL(s == xiiStringUtf8(L"abcdefghiöäjkü€").GetData());

    s = "pups";
    s.Append(nullptr, L"b", nullptr, L"d", nullptr, L"ü€");
    XII_TEST_BOOL(s == xiiStringUtf8(L"pupsbdü€").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Append(multiple)")
  {
    xiiStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    xiiStringUtf8 sUtf2(L"Test⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺Test2");

    xiiStringBuilder sb("Test");
    sb.Append(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), "Test2");

    XII_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Set(multiple)")
  {
    xiiStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    xiiStringUtf8 sUtf2(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺Test2");

    xiiStringBuilder sb("Test");
    sb.Set(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), "Test2");

    XII_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "AppendFormat")
  {
    xiiStringBuilder s("abc");
    s.AppendFormat("Test{0}{1}{2}", 42, "foo", xiiStringUtf8(L"bär").GetData());

    XII_TEST_BOOL(s == xiiStringUtf8(L"abcTest42foobär").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Prepend(char)")
  {
    xiiStringBuilder s("abc");
    s.Prepend("de", "fg", "hi", xiiStringUtf8(L"öä").GetData(), "jk", xiiStringUtf8(L"ü€").GetData());

    XII_TEST_BOOL(s == xiiStringUtf8(L"defghiöäjkü€abc").GetData());

    s = "pups";
    s.Prepend(nullptr, "b", nullptr, "d", nullptr, xiiStringUtf8(L"ü€").GetData());
    XII_TEST_BOOL(s == xiiStringUtf8(L"bdü€pups").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Prepend(wchar_t)")
  {
    xiiStringBuilder s("abc");
    s.Prepend(L"de", L"fg", L"hi", L"öä", L"jk", L"ü€");

    XII_TEST_BOOL(s == xiiStringUtf8(L"defghiöäjkü€abc").GetData());

    s = "pups";
    s.Prepend(nullptr, L"b", nullptr, L"d", nullptr, L"ü€");
    XII_TEST_BOOL(s == xiiStringUtf8(L"bdü€pups").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PrependFormat")
  {
    xiiStringBuilder s("abc");
    s.PrependFormat("Test{0}{1}{2}", 42, "foo", xiiStringUtf8(L"bär").GetData());

    XII_TEST_BOOL(s == xiiStringUtf8(L"Test42foobärabc").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Printf")
  {
    xiiStringBuilder s("abc");
    s.Printf("Test%i%s%s", 42, "foo", xiiStringUtf8(L"bär").GetData());

    XII_TEST_BOOL(s == xiiStringUtf8(L"Test42foobär").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Format")
  {
    xiiStringBuilder s("abc");
    s.Format("Test{0}{1}{2}", 42, "foo", xiiStringUtf8(L"bär").GetData());

    XII_TEST_BOOL(s == xiiStringUtf8(L"Test42foobär").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ToUpper")
  {
    xiiStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    s.ToUpper();
    XII_TEST_BOOL(s == xiiStringUtf8(L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ€ß").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ToLower")
  {
    xiiStringBuilder s(L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ€ß");
    s.ToLower();
    XII_TEST_BOOL(s == xiiStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shrink")
  {
    xiiStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    s.Shrink(5, 3);

    XII_TEST_BOOL(s == xiiStringUtf8(L"fghijklmnopqrstuvwxyzäö").GetData());

    s.Shrink(9, 7);
    XII_TEST_BOOL(s == xiiStringUtf8(L"opqrstu").GetData());

    s.Shrink(3, 2);
    XII_TEST_BOOL(s == xiiStringUtf8(L"rs").GetData());

    s.Shrink(1, 0);
    XII_TEST_BOOL(s == xiiStringUtf8(L"s").GetData());

    s.Shrink(0, 0);
    XII_TEST_BOOL(s == xiiStringUtf8(L"s").GetData());

    s.Shrink(0, 1);
    XII_TEST_BOOL(s == xiiStringUtf8(L"").GetData());

    s.Shrink(10, 0);
    XII_TEST_BOOL(s == xiiStringUtf8(L"").GetData());

    s.Shrink(0, 10);
    XII_TEST_BOOL(s == xiiStringUtf8(L"").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reserve")
  {
    xiiHeapAllocator allocator("reserve test allocator");
    xiiStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß", &allocator);
    xiiUInt32        characterCountBefore = s.GetCharacterCount();

    s.Reserve(2048);

    XII_TEST_BOOL(s.GetCharacterCount() == characterCountBefore);

    xiiUInt64 iNumAllocs = allocator.GetStats().m_uiNumAllocations;
    s.Append("blablablablablablablablablablablablablablablablablablablablablablablablablablablablablabla");
    XII_TEST_BOOL(iNumAllocs == allocator.GetStats().m_uiNumAllocations);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Convert to StringView")
  {
    xiiStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    xiiStringView    it = s;

    XII_TEST_BOOL(it.StartsWith(xiiStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData()));
    XII_TEST_BOOL(it.EndsWith(xiiStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ChangeCharacter")
  {
    xiiStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");

    xiiStringUtf8 upr(L"ÄÖÜ€ßABCDEFGHIJKLMNOPQRSTUVWXYZ");
    xiiStringView view(upr.GetData());

    for (auto it = begin(s); it.IsValid(); ++it, view.Shrink(1, 0))
    {
      s.ChangeCharacter(it, view.GetCharacter());

      XII_TEST_BOOL(it.GetCharacter() == view.GetCharacter()); // iterator reflects the changes
    }

    XII_TEST_BOOL(s == upr.GetData());
    XII_TEST_INT(s.GetCharacterCount(), 31);
    XII_TEST_INT(s.GetElementCount(), 37);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceSubString")
  {
    xiiStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");

    s.ReplaceSubString(s.GetData() + 3, s.GetData() + 7, "DEFG"); // equal length, equal num characters
    XII_TEST_BOOL(s == xiiStringUtf8(L"abcDEFGhijklmnopqrstuvwxyzäöü€ß").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 31);
    XII_TEST_INT(s.GetElementCount(), 37);

    s.ReplaceSubString(s.GetData() + 7, s.GetData() + 15, ""); // remove
    XII_TEST_BOOL(s == xiiStringUtf8(L"abcDEFGpqrstuvwxyzäöü€ß").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 23);
    XII_TEST_INT(s.GetElementCount(), 29);

    s.ReplaceSubString(s.GetData() + 17, s.GetData() + 22, "blablub"); // make longer
    XII_TEST_BOOL(s == xiiStringUtf8(L"abcDEFGpqrstuvwxyblablubü€ß").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 27);
    XII_TEST_INT(s.GetElementCount(), 31);

    s.ReplaceSubString(s.GetData() + 22, s.GetData() + 22, xiiStringUtf8(L"määh!").GetData()); // insert
    XII_TEST_BOOL(s == xiiStringUtf8(L"abcDEFGpqrstuvwxyblablmääh!ubü€ß").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 32);
    XII_TEST_INT(s.GetElementCount(), 38);

    s.ReplaceSubString(s.GetData(), s.GetData() + 10, nullptr); // remove at front
    XII_TEST_BOOL(s == xiiStringUtf8(L"stuvwxyblablmääh!ubü€ß").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 22);
    XII_TEST_INT(s.GetElementCount(), 28);

    s.ReplaceSubString(s.GetData() + 18, s.GetData() + 28, nullptr); // remove at back
    XII_TEST_BOOL(s == xiiStringUtf8(L"stuvwxyblablmääh").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 16);
    XII_TEST_INT(s.GetElementCount(), 18);

    s.ReplaceSubString(s.GetData(), s.GetData() + 18, nullptr); // clear
    XII_TEST_BOOL(s == xiiStringUtf8(L"").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 0);
    XII_TEST_INT(s.GetElementCount(), 0);

    const char* szInsert = "abc def ghi";

    s.ReplaceSubString(s.GetData(), s.GetData(), xiiStringView(szInsert, szInsert + 7)); // partial insert into empty
    XII_TEST_BOOL(s == xiiStringUtf8(L"abc def").GetData());
    XII_TEST_INT(s.GetCharacterCount(), 7);
    XII_TEST_INT(s.GetElementCount(), 7);

    // insert very large block
    s = xiiStringBuilder("a"); // hard reset to keep buffer small
    xiiString insertString("omfg this string is so long it possibly won't never ever ever ever fit into the current buffer - this will "
                           "hopefully lead to a buffer resize :)"
                           "................................................................................................................"
                           "........................................................"
                           "................................................................................................................"
                           "........................................................"
                           "................................................................................................................"
                           "........................................................"
                           "................................................................................................................"
                           "........................................................"
                           "................................................................................................................"
                           "........................................................"
                           "................................................................................................................"
                           "........................................................");
    s.ReplaceSubString(s.GetData(), s.GetData() + s.GetElementCount(), insertString.GetData());
    XII_TEST_BOOL(s == insertString.GetData());
    XII_TEST_INT(s.GetCharacterCount(), insertString.GetCharacterCount());
    XII_TEST_INT(s.GetElementCount(), insertString.GetElementCount());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert")
  {
    xiiStringBuilder s;

    s.Insert(s.GetData(), "test");
    XII_TEST_BOOL(s == "test");

    s.Insert(s.GetData() + 2, "TUT");
    XII_TEST_BOOL(s == "teTUTst");

    s.Insert(s.GetData(), "MOEP");
    XII_TEST_BOOL(s == "MOEPteTUTst");

    s.Insert(s.GetData() + s.GetElementCount(), "hompf");
    XII_TEST_BOOL(s == "MOEPteTUTsthompf");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove")
  {
    xiiStringBuilder s("MOEPteTUTsthompf");

    s.Remove(s.GetData() + 11, s.GetData() + s.GetElementCount());
    XII_TEST_BOOL(s == "MOEPteTUTst");

    s.Remove(s.GetData(), s.GetData() + 4);
    XII_TEST_BOOL(s == "teTUTst");

    s.Remove(s.GetData() + 2, s.GetData() + 5);
    XII_TEST_BOOL(s == "test");

    s.Remove(s.GetData(), s.GetData() + s.GetElementCount());
    XII_TEST_BOOL(s == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceFirst")
  {
    xiiStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceFirst("def", "BLOED");
    XII_TEST_BOOL(s == "abc BLOED abc def ghi abc ghi");

    s.ReplaceFirst("abc", "BLOED");
    XII_TEST_BOOL(s == "BLOED BLOED abc def ghi abc ghi");

    s.ReplaceFirst("abc", "BLOED", s.GetData() + 15);
    XII_TEST_BOOL(s == "BLOED BLOED abc def ghi BLOED ghi");

    s.ReplaceFirst("ghi", "LAANGWEILIG");
    XII_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED ghi");

    s.ReplaceFirst("ghi", "LAANGWEILIG");
    XII_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("def", "OEDE");
    XII_TEST_BOOL(s == "BLOED BLOED abc OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("abc", "BLOEDE");
    XII_TEST_BOOL(s == "BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG", "weg");
    XII_TEST_BOOL(s == "weg");

    s.ReplaceFirst("weg", nullptr);
    XII_TEST_BOOL(s == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceLast")
  {
    xiiStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceLast("abc", "ABC");
    XII_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast("abc", "ABC");
    XII_TEST_BOOL(s == "abc def ABC def ghi ABC ghi");

    s.ReplaceLast("abc", "ABC");
    XII_TEST_BOOL(s == "ABC def ABC def ghi ABC ghi");

    s.ReplaceLast("ghi", "GHI", s.GetData() + 24);
    XII_TEST_BOOL(s == "ABC def ABC def GHI ABC ghi");

    s.ReplaceLast("i", "I");
    XII_TEST_BOOL(s == "ABC def ABC def GHI ABC ghI");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceAll")
  {
    xiiStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceAll("abc", "TEST");
    XII_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll("def", "def");
    XII_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll("def", "defdef");
    XII_TEST_BOOL(s == "TEST defdef TEST defdef ghi TEST ghi");

    s.ReplaceAll("def", "defdef");
    XII_TEST_BOOL(s == "TEST defdefdefdef TEST defdefdefdef ghi TEST ghi");

    s.ReplaceAll("def", " ");
    XII_TEST_BOOL(s == "TEST      TEST      ghi TEST ghi");

    s.ReplaceAll(" ", "");
    XII_TEST_BOOL(s == "TESTTESTghiTESTghi");

    s.ReplaceAll("TEST", "a");
    XII_TEST_BOOL(s == "aaghiaghi");

    s.ReplaceAll("hi", "hihi");
    XII_TEST_BOOL(s == "aaghihiaghihi");

    s.ReplaceAll("ag", " ");
    XII_TEST_BOOL(s == "a hihi hihi");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceFirst_NoCase")
  {
    xiiStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceFirst_NoCase("DEF", "BLOED");
    XII_TEST_BOOL(s == "abc BLOED abc def ghi abc ghi");

    s.ReplaceFirst_NoCase("ABC", "BLOED");
    XII_TEST_BOOL(s == "BLOED BLOED abc def ghi abc ghi");

    s.ReplaceFirst_NoCase("ABC", "BLOED", s.GetData() + 15);
    XII_TEST_BOOL(s == "BLOED BLOED abc def ghi BLOED ghi");

    s.ReplaceFirst_NoCase("GHI", "LAANGWEILIG");
    XII_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED ghi");

    s.ReplaceFirst_NoCase("GHI", "LAANGWEILIG");
    XII_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("DEF", "OEDE");
    XII_TEST_BOOL(s == "BLOED BLOED abc OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("ABC", "BLOEDE");
    XII_TEST_BOOL(s == "BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG", "weg");
    XII_TEST_BOOL(s == "weg");

    s.ReplaceFirst_NoCase("WEG", nullptr);
    XII_TEST_BOOL(s == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceLast_NoCase")
  {
    xiiStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceLast_NoCase("abc", "ABC");
    XII_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("aBc", "ABC");
    XII_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("ABC", "ABC");
    XII_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("GHI", "GHI", s.GetData() + 24);
    XII_TEST_BOOL(s == "abc def abc def GHI ABC ghi");

    s.ReplaceLast_NoCase("I", "I");
    XII_TEST_BOOL(s == "abc def abc def GHI ABC ghI");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceAll_NoCase")
  {
    xiiStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceAll_NoCase("ABC", "TEST");
    XII_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "def");
    XII_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "defdef");
    XII_TEST_BOOL(s == "TEST defdef TEST defdef ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "defdef");
    XII_TEST_BOOL(s == "TEST defdefdefdef TEST defdefdefdef ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", " ");
    XII_TEST_BOOL(s == "TEST      TEST      ghi TEST ghi");

    s.ReplaceAll_NoCase(" ", "");
    XII_TEST_BOOL(s == "TESTTESTghiTESTghi");

    s.ReplaceAll_NoCase("teST", "a");
    XII_TEST_BOOL(s == "aaghiaghi");

    s.ReplaceAll_NoCase("hI", "hihi");
    XII_TEST_BOOL(s == "aaghihiaghihi");

    s.ReplaceAll_NoCase("Ag", " ");
    XII_TEST_BOOL(s == "a hihi hihi");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceWholeWord")
  {
    xiiStringBuilder s = "abcd abc abcd abc dabc abc";

    XII_TEST_BOOL(s.ReplaceWholeWord("abc", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "abcd def abcd abc dabc abc");

    XII_TEST_BOOL(s.ReplaceWholeWord("abc", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "abcd def abcd def dabc abc");

    XII_TEST_BOOL(s.ReplaceWholeWord("abc", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "abcd def abcd def dabc def");

    XII_TEST_BOOL(s.ReplaceWholeWord("abc", "def", xiiStringUtils::IsWordDelimiter_English) == nullptr);
    XII_TEST_BOOL(s == "abcd def abcd def dabc def");

    XII_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "def def abcd def dabc def");

    XII_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "def def def def dabc def");

    XII_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", xiiStringUtils::IsWordDelimiter_English) == nullptr);
    XII_TEST_BOOL(s == "def def def def dabc def");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceWholeWord_NoCase")
  {
    xiiStringBuilder s = "abcd abc abcd abc dabc abc";

    XII_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "abcd def abcd abc dabc abc");

    XII_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "abcd def abcd def dabc abc");

    XII_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "abcd def abcd def dabc def");

    XII_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", xiiStringUtils::IsWordDelimiter_English) == nullptr);
    XII_TEST_BOOL(s == "abcd def abcd def dabc def");

    XII_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABCd", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "def def abcd def dabc def");

    XII_TEST_BOOL(s.ReplaceWholeWord_NoCase("aBCD", "def", xiiStringUtils::IsWordDelimiter_English) != nullptr);
    XII_TEST_BOOL(s == "def def def def dabc def");

    XII_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABcd", "def", xiiStringUtils::IsWordDelimiter_English) == nullptr);
    XII_TEST_BOOL(s == "def def def def dabc def");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceWholeWordAll")
  {
    xiiStringBuilder s = "abcd abc abcd abc dabc abc";

    XII_TEST_INT(s.ReplaceWholeWordAll("abc", "def", xiiStringUtils::IsWordDelimiter_English), 3);
    XII_TEST_BOOL(s == "abcd def abcd def dabc def");

    XII_TEST_INT(s.ReplaceWholeWordAll("abc", "def", xiiStringUtils::IsWordDelimiter_English), 0);
    XII_TEST_BOOL(s == "abcd def abcd def dabc def");

    XII_TEST_INT(s.ReplaceWholeWordAll("abcd", "def", xiiStringUtils::IsWordDelimiter_English), 2);
    XII_TEST_BOOL(s == "def def def def dabc def");

    XII_TEST_INT(s.ReplaceWholeWordAll("abcd", "def", xiiStringUtils::IsWordDelimiter_English), 0);
    XII_TEST_BOOL(s == "def def def def dabc def");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReplaceWholeWordAll_NoCase")
  {
    xiiStringBuilder s = "abcd abc abcd abc dabc abc";

    XII_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABC", "def", xiiStringUtils::IsWordDelimiter_English), 3);
    XII_TEST_BOOL(s == "abcd def abcd def dabc def");

    XII_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABC", "def", xiiStringUtils::IsWordDelimiter_English), 0);
    XII_TEST_BOOL(s == "abcd def abcd def dabc def");

    XII_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABCd", "def", xiiStringUtils::IsWordDelimiter_English), 2);
    XII_TEST_BOOL(s == "def def def def dabc def");

    XII_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABCd", "def", xiiStringUtils::IsWordDelimiter_English), 0);
    XII_TEST_BOOL(s == "def def def def dabc def");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "teset")
  {
    const char*   sz = "abc def";
    xiiStringView it(sz);

    xiiStringBuilder s = it;
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Split")
  {
    xiiStringBuilder s = "|abc,def<>ghi|,<>jkl|mno,pqr|stu";

    xiiHybridArray<xiiStringView, 32> SubStrings;

    s.Split(false, SubStrings, ",", "|", "<>");

    XII_TEST_INT(SubStrings.GetCount(), 7);
    XII_TEST_BOOL(SubStrings[0] == "abc");
    XII_TEST_BOOL(SubStrings[1] == "def");
    XII_TEST_BOOL(SubStrings[2] == "ghi");
    XII_TEST_BOOL(SubStrings[3] == "jkl");
    XII_TEST_BOOL(SubStrings[4] == "mno");
    XII_TEST_BOOL(SubStrings[5] == "pqr");
    XII_TEST_BOOL(SubStrings[6] == "stu");

    s.Split(true, SubStrings, ",", "|", "<>");

    XII_TEST_INT(SubStrings.GetCount(), 10);
    XII_TEST_BOOL(SubStrings[0] == "");
    XII_TEST_BOOL(SubStrings[1] == "abc");
    XII_TEST_BOOL(SubStrings[2] == "def");
    XII_TEST_BOOL(SubStrings[3] == "ghi");
    XII_TEST_BOOL(SubStrings[4] == "");
    XII_TEST_BOOL(SubStrings[5] == "");
    XII_TEST_BOOL(SubStrings[6] == "jkl");
    XII_TEST_BOOL(SubStrings[7] == "mno");
    XII_TEST_BOOL(SubStrings[8] == "pqr");
    XII_TEST_BOOL(SubStrings[9] == "stu");
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeCleanPath")
  {
    xiiStringBuilder p;

    p = "C:\\temp/temp//tut";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "C:/temp/temp/tut");

    p = "\\temp/temp//tut\\\\";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "/temp/temp/tut/");

    p = "\\";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "/");

    p = "file";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "file");

    p = "C:\\temp/..//tut";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "C:/tut");

    p = "C:\\temp/..";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "C:/temp/..");

    p = "C:\\temp/..\\";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "C:/");

    p = "\\//temp/../bla\\\\blub///..\\temp//tut/tat/..\\\\..\\//ploep";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "//bla/temp/ploep");

    p = "a/b/c/../../../../e/f";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "../e/f");

    p = "/../../a/../../e/f";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "../../e/f");

    p = "/../../a/../../e/f/../";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "../../e/");

    p = "/../../a/../../e/f/..";
    p.MakeCleanPath();
    XII_TEST_BOOL(p == "../../e/f/..");

    p = "\\//temp/./bla\\\\blub///.\\temp//tut/tat/..\\.\\.\\//ploep";
    p.MakeCleanPath();
    XII_TEST_STRING(p.GetData(), "//temp/bla/blub/temp/tut/ploep");

    p = "./";
    p.MakeCleanPath();
    XII_TEST_STRING(p.GetData(), "");

    p = "/./././";
    p.MakeCleanPath();
    XII_TEST_STRING(p.GetData(), "/");

    p = "./.././";
    p.MakeCleanPath();
    XII_TEST_STRING(p.GetData(), "../");

    // more than two dots are invalid, so the should be kept as is
    p = "./..././abc/...\\def";
    p.MakeCleanPath();
    XII_TEST_STRING(p.GetData(), ".../abc/.../def");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PathParentDirectory")
  {
    xiiStringBuilder p;

    p = "C:\\temp/temp//tut";
    p.PathParentDirectory();
    XII_TEST_BOOL(p == "C:/temp/temp/");

    p = "C:\\temp/temp//tut\\\\";
    p.PathParentDirectory();
    XII_TEST_BOOL(p == "C:/temp/temp/");

    p = "file";
    p.PathParentDirectory();
    XII_TEST_BOOL(p == "");

    p = "/file";
    p.PathParentDirectory();
    XII_TEST_BOOL(p == "/");

    p = "C:\\temp/..//tut";
    p.PathParentDirectory();
    XII_TEST_BOOL(p == "C:/");

    p = "file";
    p.PathParentDirectory(3);
    XII_TEST_BOOL(p == "../../");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "AppendPath")
  {
    xiiStringBuilder p;

    p = "this/is\\my//path";
    p.AppendPath("orly/nowai");
    XII_TEST_BOOL(p == "this/is\\my//path/orly/nowai");

    p = "this/is\\my//path///";
    p.AppendPath("orly/nowai");
    XII_TEST_BOOL(p == "this/is\\my//path///orly/nowai");

    p = "";
    p.AppendPath("orly/nowai");
    XII_TEST_BOOL(p == "orly/nowai");

    // It should be valid to append an absolute path to an empty string.
    {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      const char* szAbsPath             = "C:\\folder";
      const char* szAbsPathAppendResult = "C:\\folder/File.ext";
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
      const char* szAbsPath             = "/folder";
      const char* szAbsPathAppendResult = "/folder/File.ext";
#else
#  error "An absolute path example must be defined for the 'AppendPath' test for each platform!"
#endif

      p = "";
      p.AppendPath(szAbsPath, "File.ext");
      XII_TEST_BOOL(p == szAbsPathAppendResult);
    }

    p = "bla";
    p.AppendPath("");
    XII_TEST_BOOL(p == "bla");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ChangeFileName")
  {
    xiiStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileName("bla");
    XII_TEST_BOOL(p == "C:/test/test/bla.ext");

    p = "test/test/tut/troet.toeff";
    p.ChangeFileName("toeff");
    XII_TEST_BOOL(p == "test/test/tut/toeff.toeff");

    p = "test/test/tut/murpf";
    p.ChangeFileName("toeff");
    XII_TEST_BOOL(p == "test/test/tut/toeff");

    p = "test/test/tut/murpf/";
    p.ChangeFileName("toeff");
    XII_TEST_BOOL(p == "test/test/tut/murpf/toeff"); // filename is EMPTY -> thus ADDS it

    p = "test/test/tut/murpf/.extension"; // folders that start with a dot must be considered to be empty filenames with an extension
    p.ChangeFileName("toeff");
    XII_TEST_BOOL(p == "test/test/tut/murpf/toeff.extension");

    p = "test/test/tut/murpf/.extension/"; // folders that start with a dot ARE considered as folders, if the path ends with a slash
    p.ChangeFileName("toeff");
    XII_TEST_BOOL(p == "test/test/tut/murpf/.extension/toeff");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ChangeFileNameAndExtension")
  {
    xiiStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileNameAndExtension("bla.pups");
    XII_TEST_BOOL(p == "C:/test/test/bla.pups");

    p = "test/test/tut/troet.toeff";
    p.ChangeFileNameAndExtension("toeff");
    XII_TEST_BOOL(p == "test/test/tut/toeff");

    p = "test/test/tut/murpf";
    p.ChangeFileNameAndExtension("toeff.tut");
    XII_TEST_BOOL(p == "test/test/tut/toeff.tut");

    p = "test/test/tut/murpf/";
    p.ChangeFileNameAndExtension("toeff.blo");
    XII_TEST_BOOL(p == "test/test/tut/murpf/toeff.blo"); // filename is EMPTY -> thus ADDS it

    p = "test/test/tut/murpf/.extension"; // folders that start with a dot must be considered to be empty filenames with an extension
    p.ChangeFileNameAndExtension("toeff.ext");
    XII_TEST_BOOL(p == "test/test/tut/murpf/toeff.ext");

    p = "test/test/tut/murpf/.extension/"; // folders that start with a dot ARE considered as folders, if the path ends with a slash
    p.ChangeFileNameAndExtension("toeff");
    XII_TEST_BOOL(p == "test/test/tut/murpf/.extension/toeff");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ChangeFileExtension")
  {
    xiiStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileExtension("pups");
    XII_TEST_BOOL(p == "C:/test/test/tut.pups");

    p = "C:/test/test/tut";
    p.ChangeFileExtension("pups");
    XII_TEST_BOOL(p == "C:/test/test/tut.pups");

    p = "C:/test/test/tut.ext";
    p.ChangeFileExtension("");
    XII_TEST_BOOL(p == "C:/test/test/tut.");

    p = "C:/test/test/tut";
    p.ChangeFileExtension("");
    XII_TEST_BOOL(p == "C:/test/test/tut.");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HasAnyExtension")
  {
    xiiStringBuilder p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(p.HasAnyExtension());

    p = "This/Is\\My//Path.dot\\file_no_extension";
    XII_TEST_BOOL(!p.HasAnyExtension());
    XII_TEST_BOOL(!p.HasAnyExtension());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HasExtension")
  {
    xiiStringBuilder p;

    p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(p.HasExtension(".Extension"));

    p = "This/Is\\My//Path.dot\\file.ext";
    XII_TEST_BOOL(p.HasExtension("EXT"));

    p = "This/Is\\My//Path.dot\\file.ext";
    XII_TEST_BOOL(!p.HasExtension("NEXT"));

    p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(!p.HasExtension(".Ext"));

    p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(!p.HasExtension("sion"));

    p = "";
    XII_TEST_BOOL(!p.HasExtension("ext"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileExtension")
  {
    xiiStringBuilder p;

    p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(p.GetFileExtension() == "extension");

    p = "This/Is\\My//Path.dot\\file";
    XII_TEST_BOOL(p.GetFileExtension() == "");

    p = "";
    XII_TEST_BOOL(p.GetFileExtension() == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileNameAndExtension")
  {
    xiiStringBuilder p;

    p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(p.GetFileNameAndExtension() == "file.extension");

    p = "This/Is\\My//Path.dot\\.extension";
    XII_TEST_BOOL(p.GetFileNameAndExtension() == ".extension");

    p = "This/Is\\My//Path.dot\\file";
    XII_TEST_BOOL(p.GetFileNameAndExtension() == "file");

    p = "\\file";
    XII_TEST_BOOL(p.GetFileNameAndExtension() == "file");

    p = "";
    XII_TEST_BOOL(p.GetFileNameAndExtension() == "");

    p = "/";
    XII_TEST_BOOL(p.GetFileNameAndExtension() == "");

    p = "This/Is\\My//Path.dot\\";
    XII_TEST_BOOL(p.GetFileNameAndExtension() == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileName")
  {
    xiiStringBuilder p;

    p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(p.GetFileName() == "file");

    p = "This/Is\\My//Path.dot\\file";
    XII_TEST_BOOL(p.GetFileName() == "file");

    p = "\\file";
    XII_TEST_BOOL(p.GetFileName() == "file");

    p = "";
    XII_TEST_BOOL(p.GetFileName() == "");

    p = "/";
    XII_TEST_BOOL(p.GetFileName() == "");

    p = "This/Is\\My//Path.dot\\";
    XII_TEST_BOOL(p.GetFileName() == "");

    // so far we treat file and folders whose names start with a '.' as extensions
    p = "This/Is\\My//Path.dot\\.stupidfile";
    XII_TEST_BOOL(p.GetFileName() == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileDirectory")
  {
    xiiStringBuilder p;

    p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This/Is\\My//Path.dot\\.extension";
    XII_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This/Is\\My//Path.dot\\file";
    XII_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "\\file";
    XII_TEST_BOOL(p.GetFileDirectory() == "\\");

    p = "";
    XII_TEST_BOOL(p.GetFileDirectory() == "");

    p = "/";
    XII_TEST_BOOL(p.GetFileDirectory() == "/");

    p = "This/Is\\My//Path.dot\\";
    XII_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This";
    XII_TEST_BOOL(p.GetFileDirectory() == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsAbsolutePath / IsRelativePath / IsRootedPath")
  {
    xiiStringBuilder p;

    p = "";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    p = "C:\\temp.stuff";
    XII_TEST_BOOL(p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "C:/temp.stuff";
    XII_TEST_BOOL(p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "\\\\myserver\\temp.stuff";
    XII_TEST_BOOL(p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "\\myserver\\temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath()); // neither absolute nor relativ, just stupid
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "/temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath()); // bloed
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "\\temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath()); // bloed
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "..\\temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = ".\\temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = ":MyDataDir\bla";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath());
    XII_TEST_BOOL(p.IsRootedPath());

    p = ":\\MyDataDir\bla";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath());
    XII_TEST_BOOL(p.IsRootedPath());

    p = ":/MyDataDir/bla";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath());
    XII_TEST_BOOL(p.IsRootedPath());

#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)

    p = "C:\\temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "/temp.stuff";
    XII_TEST_BOOL(p.IsAbsolutePath());
    XII_TEST_BOOL(!p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = "..\\temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

    p = ".\\temp.stuff";
    XII_TEST_BOOL(!p.IsAbsolutePath());
    XII_TEST_BOOL(p.IsRelativePath());
    XII_TEST_BOOL(!p.IsRootedPath());

#else
#  error "Unknown platform."
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRootedPathRootName")
  {
    xiiStringBuilder p;

    p = ":root\\bla";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":root/bla";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = "://root/bla";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":/\\/root\\/bla";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = "://\\root";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "noroot\\bla";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "C:\\noroot/bla";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "/noroot/bla";
    XII_TEST_BOOL(p.GetRootedPathRootName() == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsPathBelowFolder")
  {
    xiiStringBuilder p;

    p = "a/b\\c//d\\\\e/f";
    XII_TEST_BOOL(!p.IsPathBelowFolder("/a/b\\c"));
    XII_TEST_BOOL(p.IsPathBelowFolder("a/b\\c"));
    XII_TEST_BOOL(p.IsPathBelowFolder("a/b\\c//"));
    XII_TEST_BOOL(p.IsPathBelowFolder("a/b\\c//d/\\e\\f")); // equal paths are considered 'below'
    XII_TEST_BOOL(!p.IsPathBelowFolder("a/b\\c//d/\\e\\f/g"));
    XII_TEST_BOOL(p.IsPathBelowFolder("a"));
    XII_TEST_BOOL(!p.IsPathBelowFolder("b"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeRelativeTo")
  {
    xiiStringBuilder p;

    p = reinterpret_cast<const char*>(u8"ä/b\\c/d\\\\e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c")).Succeeded());
    XII_TEST_BOOL(p == "d/e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c")).Failed());
    XII_TEST_BOOL(p == "d/e/f/g");

    p = reinterpret_cast<const char*>(u8"ä/b\\c//d\\\\e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c")).Succeeded());
    XII_TEST_BOOL(p == "d/e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c")).Failed());
    XII_TEST_BOOL(p == "d/e/f/g");

    p = reinterpret_cast<const char*>(u8"ä/b\\c/d\\\\e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c/")).Succeeded());
    XII_TEST_BOOL(p == "d/e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c/")).Failed());
    XII_TEST_BOOL(p == "d/e/f/g");

    p = reinterpret_cast<const char*>(u8"ä/b\\c//d\\\\e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c/")).Succeeded());
    XII_TEST_BOOL(p == "d/e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c/")).Failed());
    XII_TEST_BOOL(p == "d/e/f/g");

    p = reinterpret_cast<const char*>(u8"ä/b\\c//d\\\\e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c\\/d/\\e\\f/g")).Succeeded());
    XII_TEST_BOOL(p == "");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c\\/d/\\e\\f/g")).Failed());
    XII_TEST_BOOL(p == "");

    p = reinterpret_cast<const char*>(u8"ä/b\\c//d\\\\e/f/g/");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c\\/d//e\\f/g\\h/i")).Succeeded());
    XII_TEST_BOOL(p == "../../");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c\\/d//e\\f/g\\h/i")).Failed());
    XII_TEST_BOOL(p == "../../");

    p = reinterpret_cast<const char*>(u8"ä/b\\c//d\\\\e/f/g/j/k");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c\\/d//e\\f/g\\h/i")).Succeeded());
    XII_TEST_BOOL(p == "../../j/k");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c\\/d//e\\f/g\\h/i")).Failed());
    XII_TEST_BOOL(p == "../../j/k");

    p = reinterpret_cast<const char*>(u8"ä/b\\c//d\\\\e/f/ge");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c//d/\\e\\f/g\\h/i")).Succeeded());
    XII_TEST_BOOL(p == "../../../ge");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c//d/\\e\\f/g\\h/i")).Failed());
    XII_TEST_BOOL(p == "../../../ge");

    p = reinterpret_cast<const char*>(u8"ä/b\\c//d\\\\e/f/g.txt");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c//d//e\\f/g\\h/i")).Succeeded());
    XII_TEST_BOOL(p == "../../../g.txt");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c//d//e\\f/g\\h/i")).Failed());
    XII_TEST_BOOL(p == "../../../g.txt");

    p = reinterpret_cast<const char*>(u8"ä/b\\c//d\\\\e/f/g");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c//d//e\\f/g\\h/i")).Succeeded());
    XII_TEST_BOOL(p == "../../");
    XII_TEST_BOOL(p.MakeRelativeTo(reinterpret_cast<const char*>(u8"ä\\b/c//d//e\\f/g\\h/i")).Failed());
    XII_TEST_BOOL(p == "../../");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakePathSeparatorsNative")
  {
    xiiStringBuilder p;
    p = "This/is\\a/temp\\\\path//to/my///file";

    p.MakePathSeparatorsNative();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    XII_TEST_STRING(p.GetData(), "This\\is\\a\\temp\\path\\to\\my\\file");
#else
    XII_TEST_STRING(p.GetData(), "This/is/a/temp/path/to/my/file");
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadAll")
  {
    xiiDefaultMemoryStreamStorage StreamStorage;

    xiiMemoryStreamWriter MemoryWriter(&StreamStorage);
    xiiMemoryStreamReader MemoryReader(&StreamStorage);

    const char* szText =
      "l;kjasdflkjdfasjlk asflkj asfljwe oiweq2390432 4 @#$ otrjk3l;2rlkhitoqhrn324:R l324h32kjr hnasfhsakfh234fas1440687873242321245";

    MemoryWriter.WriteBytes(szText, xiiStringUtils::GetStringElementCount(szText)).IgnoreResult();

    xiiStringBuilder s;
    s.ReadAll(MemoryReader);

    XII_TEST_BOOL(s == szText);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetSubString_FromTo")
  {
    xiiStringBuilder sb = "basf";

    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    sb.SetSubString_FromTo(sz + 5, sz + 13);
    XII_TEST_BOOL(sb == "fghijklm");

    sb.SetSubString_FromTo(sz + 17, sz + 30);
    XII_TEST_BOOL(sb == "rstuvwxyz");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetSubString_ElementCount")
  {
    xiiStringBuilder sb = "basf";

    xiiStringUtf8 sz(L"aäbcödefügh");

    sb.SetSubString_ElementCount(sz.GetData() + 5, 5);
    XII_TEST_BOOL(sb == xiiStringUtf8(L"ödef").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetSubString_CharacterCount")
  {
    xiiStringBuilder sb = "basf";

    xiiStringUtf8 sz(L"aäbcödefgh");

    sb.SetSubString_CharacterCount(sz.GetData() + 5, 5);
    XII_TEST_BOOL(sb == xiiStringUtf8(L"ödefg").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveFileExtension")
  {
    xiiStringBuilder sb = L"⺅⻩⽇⿕.〄㈷㑧䆴.ؼݻ༺.";

    sb.RemoveFileExtension();
    XII_TEST_STRING_UNICODE(sb.GetData(), xiiStringUtf8(L"⺅⻩⽇⿕.〄㈷㑧䆴.ؼݻ༺").GetData());

    sb.RemoveFileExtension();
    XII_TEST_STRING_UNICODE(sb.GetData(), xiiStringUtf8(L"⺅⻩⽇⿕.〄㈷㑧䆴").GetData());

    sb.RemoveFileExtension();
    XII_TEST_STRING_UNICODE(sb.GetData(), xiiStringUtf8(L"⺅⻩⽇⿕").GetData());

    sb.RemoveFileExtension();
    XII_TEST_STRING_UNICODE(sb.GetData(), xiiStringUtf8(L"⺅⻩⽇⿕").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Trim")
  {
    // Empty input
    xiiStringBuilder sb = L"";
    sb.Trim(" \t");
    XII_TEST_STRING(sb.GetData(), xiiStringUtf8(L"").GetData());
    sb.Trim(nullptr, " \t");
    XII_TEST_STRING(sb.GetData(), xiiStringUtf8(L"").GetData());
    sb.Trim(" \t", nullptr);
    XII_TEST_STRING(sb.GetData(), xiiStringUtf8(L"").GetData());

    // Clear all from one side
    auto sUnicode = L"私はクリストハさんです";
    sb            = sUnicode;
    sb.Trim(nullptr, xiiStringUtf8(sUnicode).GetData());
    XII_TEST_STRING(sb.GetData(), "");
    sb = sUnicode;
    sb.Trim(xiiStringUtf8(sUnicode).GetData(), nullptr);
    XII_TEST_STRING(sb.GetData(), "");

    // Clear partial side
    sb = L"ですですですAにぱにぱにぱ";
    sb.Trim(nullptr, xiiStringUtf8(L"にぱ").GetData());
    XII_TEST_STRING_UNICODE(sb.GetData(), xiiStringUtf8(L"ですですですA").GetData());
    sb.Trim(xiiStringUtf8(L"です").GetData(), nullptr);
    XII_TEST_STRING_UNICODE(sb.GetData(), xiiStringUtf8(L"A").GetData());

    sb = L"ですですですAにぱにぱにぱ";
    sb.Trim(xiiStringUtf8(L"ですにぱ").GetData());
    XII_TEST_STRING(sb.GetData(), xiiStringUtf8(L"A").GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TrimWordStart")
  {
    xiiStringBuilder sb;

    {
      sb = "<test>abc<test>";
      XII_TEST_BOOL(sb.TrimWordStart("<test>"));
      XII_TEST_STRING(sb, "abc<test>");
      XII_TEST_BOOL(sb.TrimWordStart("<test>") == false);
      XII_TEST_STRING(sb, "abc<test>");
    }

    {
      sb = "<test><tut><test><test><tut>abc<tut><test>";
      XII_TEST_BOOL(!sb.TrimWordStart("<tut>"));
      XII_TEST_BOOL(sb.TrimWordStart("<test>"));
      XII_TEST_BOOL(sb.TrimWordStart("<tut>"));
      XII_TEST_BOOL(sb.TrimWordStart("<test>"));
      XII_TEST_BOOL(sb.TrimWordStart("<test>"));
      XII_TEST_BOOL(sb.TrimWordStart("<tut>"));
      XII_TEST_STRING(sb, "abc<tut><test>");
      XII_TEST_BOOL(sb.TrimWordStart("<tut>") == false);
      XII_TEST_BOOL(sb.TrimWordStart("<test>") == false);
      XII_TEST_STRING(sb, "abc<tut><test>");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>abc";

      while (sb.TrimWordStart("<a>") ||
             sb.TrimWordStart("<b>") ||
             sb.TrimWordStart("<c>") ||
             sb.TrimWordStart("<d>") ||
             sb.TrimWordStart("<e>"))
      {
      }

      XII_TEST_STRING(sb, "abc");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordStart("<a>") ||
             sb.TrimWordStart("<b>") ||
             sb.TrimWordStart("<c>") ||
             sb.TrimWordStart("<d>") ||
             sb.TrimWordStart("<e>"))
      {
      }

      XII_TEST_STRING(sb, "");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TrimWordEnd")
  {
    xiiStringBuilder sb;

    {
      sb = "<test>abc<test>";
      XII_TEST_BOOL(sb.TrimWordEnd("<test>"));
      XII_TEST_STRING(sb, "<test>abc");
      XII_TEST_BOOL(sb.TrimWordEnd("<test>") == false);
      XII_TEST_STRING(sb, "<test>abc");
    }

    {
      sb = "<tut><test>abc<test><tut><test><test><tut>";
      XII_TEST_BOOL(sb.TrimWordEnd("<tut>"));
      XII_TEST_BOOL(sb.TrimWordEnd("<test>"));
      XII_TEST_BOOL(sb.TrimWordEnd("<test>"));
      XII_TEST_BOOL(sb.TrimWordEnd("<tut>"));
      XII_TEST_BOOL(sb.TrimWordEnd("<test>"));
      XII_TEST_STRING(sb, "<tut><test>abc");
      XII_TEST_BOOL(sb.TrimWordEnd("<tut>") == false);
      XII_TEST_BOOL(sb.TrimWordEnd("<test>") == false);
      XII_TEST_STRING(sb, "<tut><test>abc");
    }

    {
      sb = "abc<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordEnd("<a>") ||
             sb.TrimWordEnd("<b>") ||
             sb.TrimWordEnd("<c>") ||
             sb.TrimWordEnd("<d>") ||
             sb.TrimWordEnd("<e>"))
      {
      }

      XII_TEST_STRING(sb, "abc");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordEnd("<a>") ||
             sb.TrimWordEnd("<b>") ||
             sb.TrimWordEnd("<c>") ||
             sb.TrimWordEnd("<d>") ||
             sb.TrimWordEnd("<e>"))
      {
      }

      XII_TEST_STRING(sb, "");
    }
  }
}

#pragma optimize("", on)
