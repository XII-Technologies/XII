#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

#include <Foundation/Types/ScopeExit.h>
#include <stdarg.h>

void TestFormat(const xiiFormatString& str, const char* szExpected)
{
  xiiStringBuilder sb;
  xiiStringView    sText = str.GetText(sb);

  XII_TEST_STRING(sText, szExpected);
}

void TestFormatWChar(const xiiFormatString& str, const wchar_t* pExpected)
{
  xiiStringBuilder sb;
  xiiStringView    sText = str.GetText(sb);

  XII_TEST_WSTRING(xiiStringWChar(sText), pExpected);
}

void CompareSnprintf(xiiStringBuilder& ref_sLog, const xiiFormatString& str, const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char Temp1[256];
  char Temp2[256];

  // reusing args list crashes on GCC / Clang
  xiiStringUtils::vsnprintf(Temp1, 256, szFormat, args);
  vsnprintf(Temp2, 256, szFormat, args);
  XII_TEST_STRING(Temp1, Temp2);

  xiiTime      t1, t2, t3;
  xiiStopwatch sw;
  {
    sw.StopAndReset();

    for (xiiUInt32 i = 0; i < 10000; ++i)
    {
      xiiStringUtils::vsnprintf(Temp1, 256, szFormat, args);
    }

    t1 = sw.Checkpoint();
  }

  {
    sw.StopAndReset();

    for (xiiUInt32 i = 0; i < 10000; ++i)
    {
      vsnprintf(Temp2, 256, szFormat, args);
    }

    t2 = sw.Checkpoint();
  }

  {
    xiiStringBuilder sb;

    sw.StopAndReset();
    for (xiiUInt32 i = 0; i < 10000; ++i)
    {
      xiiStringView sText = str.GetText(sb);
    }

    t3 = sw.Checkpoint();
  }

  ref_sLog.AppendFormat("xii: {0} msec, std: {1} msec, xiiFmt: {2} msec : {3} -> {4}\n", xiiArgF(t1.GetMilliseconds(), 2), xiiArgF(t2.GetMilliseconds(), 2),
                        xiiArgF(t3.GetMilliseconds(), 2), szFormat, Temp1);

  va_end(args);
}

XII_CREATE_SIMPLE_TEST(Strings, FormatString)
{
  xiiStringBuilder perfLog;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basics")
  {
    const char* tmp = "stringviewstuff";

    const char*      sz     = "sz";
    xiiString        string = "string";
    xiiStringBuilder sb     = "builder";
    xiiStringView    sv(tmp + 6, tmp + 10);

    TestFormat(xiiFmt("{0}, {1}, {2}, {3}", xiiInt8(-1), xiiInt16(-2), xiiInt32(-3), xiiInt64(-4)), "-1, -2, -3, -4");
    TestFormat(xiiFmt("{0}, {1}, {2}, {3}", xiiUInt8(1), xiiUInt16(2), xiiUInt32(3), xiiUInt64(4)), "1, 2, 3, 4");

    TestFormat(xiiFmt("{0}, {1}", xiiArgHumanReadable(0ll), xiiArgHumanReadable(1ll)), "0, 1");
    TestFormat(xiiFmt("{0}, {1}", xiiArgHumanReadable(-0ll), xiiArgHumanReadable(-1ll)), "0, -1");
    TestFormat(xiiFmt("{0}, {1}", xiiArgHumanReadable(999ll), xiiArgHumanReadable(1000ll)), "999, 1.00K");
    TestFormat(xiiFmt("{0}, {1}", xiiArgHumanReadable(-999ll), xiiArgHumanReadable(-1000ll)), "-999, -1.00K");
    // 999.999 gets rounded up for precision 2, so result is 1000.00K not 999.99K
    TestFormat(xiiFmt("{0}, {1}", xiiArgHumanReadable(999'999ll), xiiArgHumanReadable(1'000'000ll)), "1000.00K, 1.00M");
    TestFormat(xiiFmt("{0}, {1}", xiiArgHumanReadable(-999'999ll), xiiArgHumanReadable(-1'000'000ll)), "-1000.00K, -1.00M");

    TestFormat(xiiFmt("{0}, {1}", xiiArgFileSize(0u), xiiArgFileSize(1u)), "0B, 1B");
    TestFormat(xiiFmt("{0}, {1}", xiiArgFileSize(1023u), xiiArgFileSize(1024u)), "1023B, 1.00KB");
    // 1023.999 gets rounded up for precision 2, so result is 1024.00KB not 1023.99KB
    TestFormat(xiiFmt("{0}, {1}", xiiArgFileSize(1024u * 1024u - 1u), xiiArgFileSize(1024u * 1024u)), "1024.00KB, 1.00MB");

    const char* const suffixes[]  = {" Foo", " Bar", " Foobar"};
    const xiiUInt32   suffixCount = XII_ARRAY_SIZE(suffixes);
    TestFormat(xiiFmt("{0}", xiiArgHumanReadable(0ll, 25u, suffixes, suffixCount)), "0 Foo");
    TestFormat(xiiFmt("{0}", xiiArgHumanReadable(25ll, 25u, suffixes, suffixCount)), "1.00 Bar");
    TestFormat(xiiFmt("{0}", xiiArgHumanReadable(25ll * 25ll * 2ll, 25u, suffixes, suffixCount)), "2.00 Foobar");

    TestFormat(xiiFmt("{0}", xiiArgHumanReadable(-0ll, 25u, suffixes, suffixCount)), "0 Foo");
    TestFormat(xiiFmt("{0}", xiiArgHumanReadable(-25ll, 25u, suffixes, suffixCount)), "-1.00 Bar");
    TestFormat(xiiFmt("{0}", xiiArgHumanReadable(-25ll * 25ll * 2ll, 25u, suffixes, suffixCount)), "-2.00 Foobar");

    TestFormat(xiiFmt("'{0}, {1}'", "inl", sz), "'inl, sz'");
    TestFormat(xiiFmt("'{0}'", string), "'string'");
    TestFormat(xiiFmt("'{0}'", sb), "'builder'");
    TestFormat(xiiFmt("'{0}'", sv), "'view'");

    TestFormat(xiiFmt("{3}, {1}, {0}, {2}", xiiArgF(23.12345f, 1), xiiArgI(42), 17, 12.34f), "12.34, 42, 23.1, 17");

    const wchar_t* wsz = L"wsz";
    TestFormatWChar(xiiFmt("'{0}, {1}'", "inl", wsz), L"'inl, wsz'");
    TestFormatWChar(xiiFmt("'{0}, {1}'", L"inl", wsz), L"'inl, wsz'");
    // Temp buffer limit is 63 byte (64 including trailing zero). Each character in UTF-8 can potentially use 4 byte.
    // All input characters are 1 byte, so the 60th character is the last with 4 bytes left in the buffer.
    // Thus we end up with truncation after 60 characters.
    const wchar_t* wszTooLong          = L"123456789.123456789.123456789.123456789.123456789.123456789.WAAAAAAAAAAAAAAH";
    const wchar_t* wszTooLongExpected  = L"123456789.123456789.123456789.123456789.123456789.123456789.";
    const wchar_t* wszTooLongExpected2 = L"'123456789.123456789.123456789.123456789.123456789.123456789., 123456789.123456789.123456789.123456789.123456789.123456789.'";
    TestFormatWChar(xiiFmt("{0}", wszTooLong), wszTooLongExpected);
    TestFormatWChar(xiiFmt("'{0}, {1}'", wszTooLong, wszTooLong), wszTooLongExpected2);
  }

  XII_TEST_BLOCK(xiiTestBlock::DisabledNoWarning, "Compare Performance")
  {
    CompareSnprintf(perfLog, xiiFmt("Hello {0}, i = {1}, f = {2}", "World", 42, xiiArgF(3.141f, 2)), "Hello %s, i = %i, f = %.2f", "World", 42, 3.141f);
    CompareSnprintf(perfLog, xiiFmt("No formatting at all"), "No formatting at all");
    CompareSnprintf(perfLog, xiiFmt("{0}, {1}, {2}, {3}, {4}", "AAAAAA", "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE"), "%s, %s, %s, %s, %s", "AAAAAA",
                    "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE");
    CompareSnprintf(perfLog, xiiFmt("{0}", 23), "%i", 23);
    CompareSnprintf(perfLog, xiiFmt("{0}", 23.123456789), "%f", 23.123456789);
    CompareSnprintf(perfLog, xiiFmt("{0}", xiiArgF(23.123456789, 2)), "%.2f", 23.123456789);
    CompareSnprintf(perfLog, xiiFmt("{0}", xiiArgI(123456789, 20, true)), "%020i", 123456789);
    CompareSnprintf(perfLog, xiiFmt("{0}", xiiArgI(123456789, 20, true, 16)), "%020X", 123456789);
    CompareSnprintf(perfLog, xiiFmt("{0}", xiiArgU(1234567890987ll, 30, false, 16)), "%30llx", 1234567890987ll);
    CompareSnprintf(perfLog, xiiFmt("{0}", xiiArgU(1234567890987ll, 30, false, 16, true)), "%30llX", 1234567890987ll);
    CompareSnprintf(perfLog, xiiFmt("{0}, {1}, {2}, {3}, {4}", 0, 1, 2, 3, 4), "%i, %i, %i, %i, %i", 0, 1, 2, 3, 4);
    CompareSnprintf(perfLog, xiiFmt("{0}, {1}, {2}, {3}, {4}", 0.1, 1.1, 2.1, 3.1, 4.1), "%.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1);
    CompareSnprintf(perfLog, xiiFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9),
                    "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    CompareSnprintf(perfLog, xiiFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1),
                    "%.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1);
    CompareSnprintf(perfLog, xiiFmt("{0}", xiiArgC('z')), "%c", 'z');

    CompareSnprintf(perfLog, xiiFmt("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9), "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i",
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

#if 0
    FILE* file = fopen("D:\\snprintf_perf.txt", "wb");
    if (file)
    {
      fwrite(perfLog.GetData(), 1, perfLog.GetElementCount(), file);
      fclose(file);
    }
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Auto Increment")
  {
    TestFormat(xiiFmt("{}{}{}{}", xiiInt8(1), xiiInt16(2), xiiInt32(3), xiiInt64(4)), "1234");
    TestFormat(xiiFmt("{3}{2}{1}{0}", xiiInt8(1), xiiInt16(2), xiiInt32(3), xiiInt64(4)), "4321");

    TestFormat(xiiFmt("{}, {}, {}, {}", xiiInt8(-1), xiiInt16(-2), xiiInt32(-3), xiiInt64(-4)), "-1, -2, -3, -4");
    TestFormat(xiiFmt("{}, {}, {}, {}", xiiUInt8(1), xiiUInt16(2), xiiUInt32(3), xiiUInt64(4)), "1, 2, 3, 4");

    TestFormat(xiiFmt("{0}, {}, {}, {}", xiiUInt8(1), xiiUInt16(2), xiiUInt32(3), xiiUInt64(4)), "1, 2, 3, 4");

    TestFormat(xiiFmt("{1}, {}, {}, {}", xiiUInt8(1), xiiUInt16(2), xiiUInt32(3), xiiUInt64(4), xiiUInt64(5)), "2, 3, 4, 5");

    TestFormat(xiiFmt("{2}, {}, {1}, {}", xiiUInt8(1), xiiUInt16(2), xiiUInt32(3), xiiUInt64(4), xiiUInt64(5)), "3, 4, 2, 3");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiTime")
  {
    TestFormat(xiiFmt("{}", xiiTime()), "0ns");
    TestFormat(xiiFmt("{}", xiiTime::Nanoseconds(999)), "999ns");
    TestFormat(xiiFmt("{}", xiiTime::Nanoseconds(999.1)), "999.1ns");
    TestFormat(xiiFmt("{}", xiiTime::Microseconds(999)), reinterpret_cast<const char*>(u8"999\u00B5s"));     // Utf-8 encoding for the microsecond sign
    TestFormat(xiiFmt("{}", xiiTime::Microseconds(999.2)), reinterpret_cast<const char*>(u8"999.2\u00B5s")); // Utf-8 encoding for the microsecond sign
    TestFormat(xiiFmt("{}", xiiTime::Milliseconds(-999)), "-999ms");
    TestFormat(xiiFmt("{}", xiiTime::Milliseconds(-999.3)), "-999.3ms");
    TestFormat(xiiFmt("{}", xiiTime::Seconds(59)), "59sec");
    TestFormat(xiiFmt("{}", xiiTime::Seconds(-59.9)), "-59.9sec");
    TestFormat(xiiFmt("{}", xiiTime::Seconds(75)), "1min 15sec");
    TestFormat(xiiFmt("{}", xiiTime::Seconds(-75.4)), "-1min 15sec");
    TestFormat(xiiFmt("{}", xiiTime::Minutes(59)), "59min 0sec");
    TestFormat(xiiFmt("{}", xiiTime::Minutes(-1)), "-1min 0sec");
    TestFormat(xiiFmt("{}", xiiTime::Minutes(90)), "1h 30min 0sec");
    TestFormat(xiiFmt("{}", xiiTime::Minutes(-90.5)), "-1h 30min 30sec");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiDateTime")
  {
    {
      xiiDateTime dt;
      dt.SetYear(2019);
      dt.SetMonth(6);
      dt.SetDay(12);
      dt.SetHour(13);
      dt.SetMinute(26);
      dt.SetSecond(51);
      dt.SetMicroseconds(7000);

      TestFormat(xiiFmt("{}", dt), "2019-06-12_13-26-51-007");
    }

    {
      xiiDateTime dt;
      dt.SetYear(0);
      dt.SetMonth(1);
      dt.SetDay(1);
      dt.SetHour(0);
      dt.SetMinute(0);
      dt.SetSecond(0);
      dt.SetMicroseconds(0);

      TestFormat(xiiFmt("{}", dt), "0000-01-01_00-00-00-000");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sensitive Info")
  {
    auto prev = xiiArgSensitive::s_BuildStringCB;
    XII_SCOPE_EXIT(xiiArgSensitive::s_BuildStringCB = prev);

    xiiArgSensitive::s_BuildStringCB = xiiArgSensitive::BuildString_SensitiveUserData_Hash;

    xiiStringBuilder fmt;

    fmt.Format("Password: {}", xiiArgSensitive("hunter2", "pwd"));
    XII_TEST_STRING(fmt, "Password: sud:pwd#96d66ce6($7)");

    fmt.Format("Password: {}", xiiArgSensitive("hunter2"));
    XII_TEST_STRING(fmt, "Password: sud:#96d66ce6($7)");
  }
}
