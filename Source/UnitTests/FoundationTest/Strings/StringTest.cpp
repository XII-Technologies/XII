#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/String.h>

static xiiString GetString(const char* szSz)
{
  xiiString s;
  s = szSz;
  return s;
}

static xiiStringBuilder GetStringBuilder(const char* szSz)
{
  xiiStringBuilder s;

  for (xiiUInt32 i = 0; i < 10; ++i)
    s.Append(szSz);

  return s;
}

XII_CREATE_SIMPLE_TEST(Strings, String)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiString s1;
    XII_TEST_BOOL(s1 == "");

    xiiString s2("abc");
    XII_TEST_BOOL(s2 == "abc");

    xiiString s3(s2);
    XII_TEST_BOOL(s2 == s3);
    XII_TEST_BOOL(s3 == "abc");

    xiiString s4(L"abc");
    XII_TEST_BOOL(s4 == "abc");

    xiiStringView it = s4.GetFirst(2);
    xiiString     s5(it);
    XII_TEST_BOOL(s5 == "ab");

    xiiStringBuilder strB("wobwob");
    xiiString        s6(strB);
    XII_TEST_BOOL(s6 == "wobwob");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=")
  {
    xiiString s2;
    s2 = "abc";
    XII_TEST_BOOL(s2 == "abc");

    xiiString s3;
    s3 = s2;
    XII_TEST_BOOL(s2 == s3);
    XII_TEST_BOOL(s3 == "abc");

    xiiString s4;
    s4 = L"abc";
    XII_TEST_BOOL(s4 == "abc");

    xiiString     s5(L"abcdefghijklm");
    xiiStringView it(s5.GetData() + 2, s5.GetData() + 10);
    xiiString     s5b = it;
    XII_TEST_STRING(s5b, "cdefghij");

    xiiString        s6(L"aölsdföasld");
    xiiStringBuilder strB("wobwob");
    s6 = strB;
    XII_TEST_BOOL(s6 == "wobwob");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "convert to xiiStringView")
  {
    xiiString        s(L"aölsdföasld");
    xiiStringBuilder tmp;

    xiiStringView sv = s;

    XII_TEST_STRING(sv.GetData(tmp), xiiStringUtf8(L"aölsdföasld").GetData());
    XII_TEST_BOOL(sv == xiiStringUtf8(L"aölsdföasld").GetData());

    s = "abcdef";

    XII_TEST_STRING(sv.GetStartPointer(), "abcdef");
    XII_TEST_BOOL(sv == "abcdef");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move constructor / operator")
  {
    xiiString s1(GetString("move me"));
    XII_TEST_STRING(s1.GetData(), "move me");

    s1 = GetString("move move move move move move move move ");
    XII_TEST_STRING(s1.GetData(), "move move move move move move move move ");

    xiiString s2(GetString("move move move move move move move move "));
    XII_TEST_STRING(s2.GetData(), "move move move move move move move move ");

    s2 = GetString("move me");
    XII_TEST_STRING(s2.GetData(), "move me");

    s1 = s2;
    XII_TEST_STRING(s1.GetData(), "move me");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move constructor / operator (StringBuilder)")
  {
    const xiiString s1(GetStringBuilder("move me"));
    const xiiString s2(GetStringBuilder("move move move move move move move move "));

    xiiString s3(GetStringBuilder("move me"));
    XII_TEST_BOOL(s3 == s1);

    s3 = GetStringBuilder("move move move move move move move move ");
    XII_TEST_BOOL(s3 == s2);

    xiiString s4(GetStringBuilder("move move move move move move move move "));
    XII_TEST_BOOL(s4 == s2);

    s4 = GetStringBuilder("move me");
    XII_TEST_BOOL(s4 == s1);

    s3 = s4;
    XII_TEST_BOOL(s3 == s1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    xiiString s("abcdef");
    XII_TEST_BOOL(s == "abcdef");

    s.Clear();
    XII_TEST_BOOL(s.IsEmpty());
    XII_TEST_BOOL(s == "");
    XII_TEST_BOOL(s == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetData")
  {
    const char* sz = "abcdef";

    xiiString s(sz);
    XII_TEST_BOOL(s.GetData() != sz); // it should NOT be the exact same string
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetElementCount / GetCharacterCount")
  {
    xiiString s(L"abcäöü€");

    XII_TEST_INT(s.GetElementCount(), 12);
    XII_TEST_INT(s.GetCharacterCount(), 7);

    s = "testtest";
    XII_TEST_INT(s.GetElementCount(), 8);
    XII_TEST_INT(s.GetCharacterCount(), 8);

    s.Clear();

    XII_TEST_INT(s.GetElementCount(), 0);
    XII_TEST_INT(s.GetCharacterCount(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Convert to xiiStringView")
  {
    xiiString s(L"abcäöü€def");

    xiiStringView view = s;
    XII_TEST_BOOL(view.StartsWith("abc"));
    XII_TEST_BOOL(view.EndsWith("def"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetSubString")
  {
    xiiString     s(L"abcäöü€def");
    xiiStringUtf8 s8(L"äöü€");

    xiiStringView it = s.GetSubString(3, 4);
    XII_TEST_BOOL(it == s8.GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFirst")
  {
    xiiString s(L"abcäöü€def");

    XII_TEST_BOOL(s.GetFirst(3) == "abc");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLast")
  {
    xiiString s(L"abcäöü€def");

    XII_TEST_BOOL(s.GetLast(3) == "def");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadAll")
  {
    xiiDefaultMemoryStreamStorage StreamStorage;

    xiiMemoryStreamWriter MemoryWriter(&StreamStorage);
    xiiMemoryStreamReader MemoryReader(&StreamStorage);

    const char* szText =
      "l;kjasdflkjdfasjlk asflkj asfljwe oiweq2390432 4 @#$ otrjk3l;2rlkhitoqhrn324:R l324h32kjr hnasfhsakfh234fas1440687873242321245";

    MemoryWriter.WriteBytes(szText, xiiStringUtils::GetStringElementCount(szText)).IgnoreResult();

    xiiString s;
    s.ReadAll(MemoryReader);

    XII_TEST_BOOL(s == szText);
  }
}
