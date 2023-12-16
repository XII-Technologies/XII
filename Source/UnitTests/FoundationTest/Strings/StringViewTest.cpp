#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

XII_CREATE_SIMPLE_TEST(Strings, StringView)
{
  xiiStringBuilder tmp;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (simple)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    xiiStringView it(sz);

    XII_TEST_BOOL(it.GetStartPointer() == sz);
    XII_TEST_STRING(it.GetData(tmp), sz);
    XII_TEST_BOOL(it.GetEndPointer() == sz + 26);
    XII_TEST_INT(it.GetElementCount(), 26);

    xiiStringView it2(sz + 15);

    XII_TEST_BOOL(it2.GetStartPointer() == &sz[15]);
    XII_TEST_STRING(it2.GetData(tmp), &sz[15]);
    XII_TEST_BOOL(it2.GetEndPointer() == sz + 26);
    XII_TEST_INT(it2.GetElementCount(), 11);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (complex, YARLY!)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    xiiStringView it(sz + 3, sz + 17);
    it.SetStartPosition(sz + 5);

    XII_TEST_BOOL(it.GetStartPointer() == sz + 5);
    XII_TEST_STRING(it.GetData(tmp), "fghijklmnopq");
    XII_TEST_BOOL(it.GetEndPointer() == sz + 17);
    XII_TEST_INT(it.GetElementCount(), 12);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor constexpr")
  {
    constexpr xiiStringView b = xiiStringView("Hello World", 10);
    XII_TEST_INT(b.GetElementCount(), 10);
    XII_TEST_STRING(b.GetData(tmp), "Hello Worl");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "String literal")
  {
    constexpr xiiStringView a = "Hello World"_xiisv;
    XII_TEST_INT(a.GetElementCount(), 11);
    XII_TEST_STRING(a.GetData(tmp), "Hello World");

    xiiStringView b = "Hello Worl"_xiisv;
    XII_TEST_INT(b.GetElementCount(), 10);
    XII_TEST_STRING(b.GetData(tmp), "Hello Worl");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator++")
  {
    const char*   sz = "abcdefghijklmnopqrstuvwxyz";
    xiiStringView it(sz);

    for (xiiInt32 i = 0; i < 26; ++i)
    {
      XII_TEST_INT(it.GetCharacter(), sz[i]);
      XII_TEST_BOOL(it.IsValid());
      it.Shrink(1, 0);
    }

    XII_TEST_BOOL(!it.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== / operator!=")
  {
    xiiString s1(L"abcdefghiäöüß€");
    xiiString s2(L"ghiäöüß€abdef");

    xiiStringView it1 = s1.GetSubString(8, 4);
    xiiStringView it2 = s2.GetSubString(2, 4);
    xiiStringView it3 = s2.GetSubString(2, 5);

    XII_TEST_BOOL(it1 == it2);
    XII_TEST_BOOL(it1 != it3);

    XII_TEST_BOOL(it1 == xiiString(L"iäöü").GetData());
    XII_TEST_BOOL(it2 == xiiString(L"iäöü").GetData());
    XII_TEST_BOOL(it3 == xiiString(L"iäöüß").GetData());

    s1 = "abcdefghijkl";
    s2 = "oghijklm";

    it1 = s1.GetSubString(6, 4);
    it2 = s2.GetSubString(1, 4);
    it3 = s2.GetSubString(1, 5);

    XII_TEST_BOOL(it1 == it2);
    XII_TEST_BOOL(it1 != it3);

    XII_TEST_BOOL(it1 == "ghij");
    XII_TEST_BOOL(it1 != "ghijk");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    const char*   sz = "abcdef";
    xiiStringView it(sz);

    XII_TEST_BOOL(it.IsEqual(xiiStringView("abcdef")));
    XII_TEST_BOOL(!it.IsEqual(xiiStringView("abcde")));
    XII_TEST_BOOL(!it.IsEqual(xiiStringView("abcdefg")));

    xiiStringView it2(sz + 2, sz + 5);

    const char*   szRhs = "Abcdef";
    xiiStringView it3(szRhs + 2, szRhs + 5);
    XII_TEST_BOOL(it2.IsEqual(it3));
    it3 = xiiStringView(szRhs + 1, szRhs + 5);
    XII_TEST_BOOL(!it2.IsEqual(it3));
    it3 = xiiStringView(szRhs + 2, szRhs + 6);
    XII_TEST_BOOL(!it2.IsEqual(it3));
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator+=")
  {
    const char*   sz = "abcdefghijklmnopqrstuvwxyz";
    xiiStringView it(sz);

    for (xiiInt32 i = 0; i < 26; i += 2)
    {
      XII_TEST_INT(it.GetCharacter(), sz[i]);
      XII_TEST_BOOL(it.IsValid());
      it.Shrink(2, 0);
    }

    XII_TEST_BOOL(!it.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCharacter")
  {
    xiiStringUtf8 s(L"abcäöü€");
    xiiStringView it = xiiStringView(s.GetData());

    XII_TEST_INT(it.GetCharacter(), xiiUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[0]));
    it.Shrink(1, 0);
    XII_TEST_INT(it.GetCharacter(), xiiUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[1]));
    it.Shrink(1, 0);
    XII_TEST_INT(it.GetCharacter(), xiiUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[2]));
    it.Shrink(1, 0);
    XII_TEST_INT(it.GetCharacter(), xiiUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[3]));
    it.Shrink(1, 0);
    XII_TEST_INT(it.GetCharacter(), xiiUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[5]));
    it.Shrink(1, 0);
    XII_TEST_INT(it.GetCharacter(), xiiUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[7]));
    it.Shrink(1, 0);
    XII_TEST_INT(it.GetCharacter(), xiiUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[9]));
    it.Shrink(1, 0);
    XII_TEST_BOOL(!it.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetElementCount")
  {
    xiiStringUtf8 s(L"abcäöü€");
    xiiStringView it = xiiStringView(s.GetData());

    XII_TEST_INT(it.GetElementCount(), 12);
    it.Shrink(1, 0);
    XII_TEST_BOOL(it.IsValid());
    XII_TEST_INT(it.GetElementCount(), 11);
    it.Shrink(1, 0);
    XII_TEST_BOOL(it.IsValid());
    XII_TEST_INT(it.GetElementCount(), 10);
    it.Shrink(1, 0);
    XII_TEST_BOOL(it.IsValid());
    XII_TEST_INT(it.GetElementCount(), 9);
    it.Shrink(1, 0);
    XII_TEST_BOOL(it.IsValid());
    XII_TEST_INT(it.GetElementCount(), 7);
    it.Shrink(1, 0);
    XII_TEST_BOOL(it.IsValid());
    XII_TEST_INT(it.GetElementCount(), 5);
    it.Shrink(1, 0);
    XII_TEST_BOOL(it.IsValid());
    XII_TEST_INT(it.GetElementCount(), 3);
    it.Shrink(1, 0);
    XII_TEST_BOOL(!it.IsValid());
    XII_TEST_INT(it.GetElementCount(), 0);
    it.Shrink(1, 0);
    XII_TEST_BOOL(!it.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetStartPosition")
  {
    const char*   sz = "abcdefghijklmnopqrstuvwxyz";
    xiiStringView it(sz);

    for (xiiInt32 i = 0; i < 26; ++i)
    {
      it.SetStartPosition(sz + i);
      XII_TEST_BOOL(it.IsValid());
      XII_TEST_BOOL(it.StartsWith(&sz[i]));
    }

    XII_TEST_BOOL(it.IsValid());
    it.Shrink(1, 0);
    XII_TEST_BOOL(!it.IsValid());

    it = xiiStringView(sz);
    for (xiiInt32 i = 0; i < 26; ++i)
    {
      it.SetStartPosition(sz + i);
      XII_TEST_BOOL(it.IsValid());
      XII_TEST_BOOL(it.StartsWith(&sz[i]));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetStartPosition / GetEndPosition / GetData")
  {
    const char*   sz = "abcdefghijklmnopqrstuvwxyz";
    xiiStringView it(sz + 7, sz + 19);

    XII_TEST_BOOL(it.GetStartPointer() == sz + 7);
    XII_TEST_BOOL(it.GetEndPointer() == sz + 19);
    XII_TEST_STRING(it.GetData(tmp), "hijklmnopqrs");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shrink")
  {
    xiiStringUtf8 s(L"abcäöü€def");
    xiiStringView it(s.GetData());

    XII_TEST_BOOL(it.GetStartPointer() == &s.GetData()[0]);
    XII_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    XII_TEST_STRING(it.GetData(tmp), &s.GetData()[0]);
    XII_TEST_BOOL(it.IsValid());

    it.Shrink(1, 0);

    XII_TEST_BOOL(it.GetStartPointer() == &s.GetData()[1]);
    XII_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    XII_TEST_STRING(it.GetData(tmp), &s.GetData()[1]);
    XII_TEST_BOOL(it.IsValid());

    it.Shrink(3, 0);

    XII_TEST_BOOL(it.GetStartPointer() == &s.GetData()[5]);
    XII_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    XII_TEST_STRING(it.GetData(tmp), &s.GetData()[5]);
    XII_TEST_BOOL(it.IsValid());

    it.Shrink(0, 4);

    XII_TEST_BOOL(it.GetStartPointer() == &s.GetData()[5]);
    XII_TEST_BOOL(it.GetEndPointer() == &s.GetData()[9]);
    XII_TEST_STRING(it.GetData(tmp), (const char*)u8"öü");
    XII_TEST_BOOL(it.IsValid());

    it.Shrink(1, 1);

    XII_TEST_BOOL(it.GetStartPointer() == &s.GetData()[7]);
    XII_TEST_BOOL(it.GetEndPointer() == &s.GetData()[7]);
    XII_TEST_STRING(it.GetData(tmp), "");
    XII_TEST_BOOL(!it.IsValid());

    it.Shrink(10, 10);

    XII_TEST_BOOL(it.GetStartPointer() == &s.GetData()[7]);
    XII_TEST_BOOL(it.GetEndPointer() == &s.GetData()[7]);
    XII_TEST_STRING(it.GetData(tmp), "");
    XII_TEST_BOOL(!it.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ChopAwayFirstCharacterUtf8")
  {
    xiiStringUtf8 utf8(L"О, Господи!");
    xiiStringView s(utf8.GetData());

    const char* szOrgStart = s.GetStartPointer();
    const char* szOrgEnd   = s.GetEndPointer();

    while (!s.IsEmpty())
    {
      const xiiUInt32 uiNumCharsBefore = xiiStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer());
      s.ChopAwayFirstCharacterUtf8();
      const xiiUInt32 uiNumCharsAfter = xiiStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer());

      XII_TEST_INT(uiNumCharsBefore, uiNumCharsAfter + 1);
    }

    // This needs to be true, some code relies on the fact that the start pointer always moves forwards
    XII_TEST_BOOL(s.GetStartPointer() == szOrgEnd);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ChopAwayFirstCharacterAscii")
  {
    xiiStringUtf8 utf8(L"Wosn Schmarrn");
    xiiStringView s("");

    const char* szOrgStart = s.GetStartPointer();
    const char* szOrgEnd   = s.GetEndPointer();

    while (!s.IsEmpty())
    {
      const xiiUInt32 uiNumCharsBefore = s.GetElementCount();
      s.ChopAwayFirstCharacterAscii();
      const xiiUInt32 uiNumCharsAfter = s.GetElementCount();

      XII_TEST_INT(uiNumCharsBefore, uiNumCharsAfter + 1);
    }

    // This needs to be true, some code relies on the fact that the start pointer always moves forwards
    XII_TEST_BOOL(s.GetStartPointer() == szOrgEnd);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Trim")
  {
    // Empty input
    xiiStringUtf8 utf8(L"");
    xiiStringView view(utf8.GetData());
    view.Trim(" \t");
    XII_TEST_BOOL(view.IsEqual(xiiStringUtf8(L"").GetData()));
    view.Trim(nullptr, " \t");
    XII_TEST_BOOL(view.IsEqual(xiiStringUtf8(L"").GetData()));
    view.Trim(" \t", nullptr);
    XII_TEST_BOOL(view.IsEqual(xiiStringUtf8(L"").GetData()));

    // Clear all from one side
    xiiStringUtf8 sUnicode(L"私はクリストハさんです");
    view = sUnicode.GetData();
    view.Trim(nullptr, sUnicode.GetData());
    XII_TEST_BOOL(view.IsEqual(""));
    view = sUnicode.GetData();
    view.Trim(sUnicode.GetData(), nullptr);
    XII_TEST_BOOL(view.IsEqual(""));

    // Clear partial side
    sUnicode = L"ですですですAにぱにぱにぱ";
    view     = sUnicode.GetData();
    view.Trim(nullptr, xiiStringUtf8(L"にぱ").GetData());
    sUnicode = L"ですですですA";
    XII_TEST_BOOL(view.IsEqual(sUnicode.GetData()));
    view.Trim(xiiStringUtf8(L"です").GetData(), nullptr);
    XII_TEST_BOOL(view.IsEqual(xiiStringUtf8(L"A").GetData()));

    sUnicode = L"ですですですAにぱにぱにぱ";
    view     = sUnicode.GetData();
    view.Trim(xiiStringUtf8(L"ですにぱ").GetData());
    XII_TEST_BOOL(view.IsEqual(xiiStringUtf8(L"A").GetData()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TrimWordStart")
  {
    xiiStringView sb;

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
    xiiStringView sb;

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Split")
  {
    xiiStringView s = "|abc,def<>ghi|,<>jkl|mno,pqr|stu";

    xiiDeque<xiiStringView> SubStrings;

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HasAnyExtension")
  {
    xiiStringView p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(p.HasAnyExtension());

    p = "This/Is\\My//Path.dot\\file_no_extension";
    XII_TEST_BOOL(!p.HasAnyExtension());
    XII_TEST_BOOL(!p.HasAnyExtension());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HasExtension")
  {
    xiiStringView p;

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
    xiiStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    XII_TEST_BOOL(p.GetFileExtension() == "extension");

    p = "This/Is\\My//Path.dot\\file";
    XII_TEST_BOOL(p.GetFileExtension() == "");

    p = "";
    XII_TEST_BOOL(p.GetFileExtension() == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileNameAndExtension")
  {
    xiiStringView p;

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
    xiiStringView p;

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
    xiiStringView p;

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
    xiiStringView p;

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
    xiiStringView p;

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
}
