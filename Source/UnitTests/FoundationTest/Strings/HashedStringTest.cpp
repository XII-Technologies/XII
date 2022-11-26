#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/HashedString.h>

XII_CREATE_SIMPLE_TEST(Strings, HashedString)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiHashedString s;
    xiiHashedString s2;

    s2.Assign("test"); // compile time hashing

    XII_TEST_INT(s.GetHash(), 0xef46db3751d8e999llu);
    XII_TEST_STRING(s.GetString().GetData(), "");
    XII_TEST_BOOL(s.GetString().IsEmpty());

    xiiTempHashedString ts("test"); // compile time hashing
    XII_TEST_INT(ts.GetHash(), 0x4fdcca5ddb678139llu);

    xiiStringBuilder    sb = "test2";
    xiiTempHashedString ts2(sb.GetData()); // runtime hashing
    XII_TEST_INT(ts2.GetHash(), 0x890e0a4c7111eb87llu);

    xiiTempHashedString ts3(s2);
    XII_TEST_INT(ts3.GetHash(), 0x4fdcca5ddb678139llu);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Assign")
  {
    xiiHashedString s;
    s.Assign("Test"); // compile time hashing

    XII_TEST_STRING(s.GetString().GetData(), "Test");
    XII_TEST_INT(s.GetHash(), 0xda83efc38a8922b4llu);

    xiiStringBuilder sb = "test2";
    s.Assign(sb.GetData()); // runtime hashing
    XII_TEST_STRING(s.GetString().GetData(), "test2");
    XII_TEST_INT(s.GetHash(), 0x890e0a4c7111eb87llu);

    xiiTempHashedString ts("dummy");
    ts = "test"; // compile time hashing
    XII_TEST_INT(ts.GetHash(), 0x4fdcca5ddb678139llu);

    ts = sb.GetData(); // runtime hashing
    XII_TEST_INT(ts.GetHash(), 0x890e0a4c7111eb87llu);

    s.Assign("");
    XII_TEST_INT(s.GetHash(), 0xef46db3751d8e999llu);
    XII_TEST_STRING(s.GetString().GetData(), "");
    XII_TEST_BOOL(s.GetString().IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TempHashedString")
  {
    xiiTempHashedString ts;
    xiiHashedString     hs;

    XII_TEST_INT(ts.GetHash(), hs.GetHash());

    XII_TEST_INT(ts.GetHash(), 0xef46db3751d8e999llu);

    ts                      = "Test";
    xiiTempHashedString ts2 = ts;
    XII_TEST_INT(ts.GetHash(), 0xda83efc38a8922b4llu);

    ts = "";
    ts2.Clear();
    XII_TEST_INT(ts.GetHash(), 0xef46db3751d8e999llu);
    XII_TEST_INT(ts2.GetHash(), 0xef46db3751d8e999llu);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== / operator!=")
  {
    xiiHashedString s1, s2, s3, s4;
    s1.Assign("Test1");
    s2.Assign("Test2");
    s3.Assign("Test1");
    s4.Assign("Test2");

    xiiTempHashedString t1("Test1");
    xiiTempHashedString t2("Test2");

    XII_TEST_STRING(s1.GetString().GetData(), "Test1");
    XII_TEST_STRING(s2.GetString().GetData(), "Test2");
    XII_TEST_STRING(s3.GetString().GetData(), "Test1");
    XII_TEST_STRING(s4.GetString().GetData(), "Test2");

    XII_TEST_BOOL(s1 == s1);
    XII_TEST_BOOL(s2 == s2);
    XII_TEST_BOOL(s3 == s3);
    XII_TEST_BOOL(s4 == s4);
    XII_TEST_BOOL(t1 == t1);
    XII_TEST_BOOL(t2 == t2);

    XII_TEST_BOOL(s1 != s2);
    XII_TEST_BOOL(s1 == s3);
    XII_TEST_BOOL(s1 != s4);
    XII_TEST_BOOL(s1 == t1);
    XII_TEST_BOOL(s1 != t2);

    XII_TEST_BOOL(s2 != s3);
    XII_TEST_BOOL(s2 == s4);
    XII_TEST_BOOL(s2 != t1);
    XII_TEST_BOOL(s2 == t2);

    XII_TEST_BOOL(s3 != s4);
    XII_TEST_BOOL(s3 == t1);
    XII_TEST_BOOL(s3 != t2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copying")
  {
    xiiHashedString s1;
    s1.Assign("blaa");

    xiiHashedString s2(s1);
    xiiHashedString s3;
    s3 = s2;

    XII_TEST_BOOL(s1 == s2);
    XII_TEST_BOOL(s1 == s3);

    xiiHashedString s4(std::move(s2));
    xiiHashedString s5;
    s5 = std::move(s3);

    XII_TEST_BOOL(s1 == s4);
    XII_TEST_BOOL(s1 == s5);
    XII_TEST_BOOL(s1 != s2);
    XII_TEST_BOOL(s1 != s3);

    xiiTempHashedString t1("blaa");

    xiiTempHashedString t2(t1);
    xiiTempHashedString t3("urg");
    t3 = t2;

    XII_TEST_BOOL(t1 == t2);
    XII_TEST_BOOL(t1 == t3);

    t3 = s1;
    XII_TEST_INT(t3.GetHash(), s1.GetHash());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator<")
  {
    xiiHashedString s1, s2, s3;
    s1.Assign("blaa");
    s2.Assign("blub");
    s3.Assign("tut");

    xiiMap<xiiHashedString, xiiInt32> m; // uses operator< internally
    m[s1] = 1;
    m[s2] = 2;
    m[s3] = 3;

    XII_TEST_INT(m[s1], 1);
    XII_TEST_INT(m[s2], 2);
    XII_TEST_INT(m[s3], 3);

    xiiTempHashedString t1("blaa");
    xiiTempHashedString t2("blub");
    xiiTempHashedString t3("tut");

    XII_TEST_BOOL((s1 < s1) == (t1 < t1));
    XII_TEST_BOOL((s1 < s2) == (t1 < t2));
    XII_TEST_BOOL((s1 < s3) == (t1 < t3));

    XII_TEST_BOOL((s1 < s1) == (s1 < t1));
    XII_TEST_BOOL((s1 < s2) == (s1 < t2));
    XII_TEST_BOOL((s1 < s3) == (s1 < t3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetString")
  {
    xiiHashedString s1, s2, s3;
    s1.Assign("blaa");
    s2.Assign("blub");
    s3.Assign("tut");

    XII_TEST_STRING(s1.GetString().GetData(), "blaa");
    XII_TEST_STRING(s2.GetString().GetData(), "blub");
    XII_TEST_STRING(s3.GetString().GetData(), "tut");
  }

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClearUnusedStrings")
  {
    xiiHashedString::ClearUnusedStrings();

    {
      xiiHashedString s1, s2, s3;
      s1.Assign("blaa");
      s2.Assign("blub");
      s3.Assign("tut");
    }

    XII_TEST_INT(xiiHashedString::ClearUnusedStrings(), 3);
    XII_TEST_INT(xiiHashedString::ClearUnusedStrings(), 0);
  }
#endif
}
