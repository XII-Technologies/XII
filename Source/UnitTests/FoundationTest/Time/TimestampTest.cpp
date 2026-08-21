/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

XII_CREATE_SIMPLE_TEST(Time, Timestamp)
{
  const xiiInt64 iFirstContactUnixTimeInSeconds = 2942956800LL;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructors / Valid Check")
  {
    xiiTimestamp invalidTimestamp;
    XII_TEST_BOOL(!invalidTimestamp.IsValid());

    xiiTimestamp validTimestamp = xiiTimestamp::MakeFromInt(0, xiiSIUnitOfTime::Second);
    XII_TEST_BOOL(validTimestamp.IsValid());
    validTimestamp = xiiTimestamp::MakeInvalid();
    XII_TEST_BOOL(!validTimestamp.IsValid());

    xiiTimestamp currentTimestamp = xiiTimestamp::CurrentTimestamp();
    // Kind of hard to hit a moving target, let's just test if it is in a probable range.
    XII_TEST_BOOL(currentTimestamp.IsValid());
    XII_TEST_BOOL_MSG(currentTimestamp.GetInt64(xiiSIUnitOfTime::Second) > 1384597970LL, "The current time is before this test was written!");
    XII_TEST_BOOL_MSG(currentTimestamp.GetInt64(xiiSIUnitOfTime::Second) < 32531209845LL, "This current time is after the year 3000! If this is actually the case, please fix this test.");

    // Sleep for 10 milliseconds
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    XII_TEST_BOOL_MSG(currentTimestamp.GetInt64(xiiSIUnitOfTime::Microsecond) < xiiTimestamp::CurrentTimestamp().GetInt64(xiiSIUnitOfTime::Microsecond), "Sleeping for 10 ms should cause the timestamp to change!");
    XII_TEST_BOOL_MSG(!currentTimestamp.Compare(xiiTimestamp::CurrentTimestamp(), xiiTimestamp::CompareMode::Identical), "Sleeping for 10 ms should cause the timestamp to change!");

    // a valid timestamp should always be 'newer' than an invalid one
    XII_TEST_BOOL(currentTimestamp.Compare(xiiTimestamp::MakeInvalid(), xiiTimestamp::CompareMode::Newer) == true);
    // an invalid timestamp should not be 'newer' than any valid one
    XII_TEST_BOOL(xiiTimestamp::MakeInvalid().Compare(currentTimestamp, xiiTimestamp::CompareMode::Newer) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Public Accessors")
  {
    const xiiTimestamp epoch        = xiiTimestamp::MakeFromInt(0, xiiSIUnitOfTime::Second);
    const xiiTimestamp firstContact = xiiTimestamp::MakeFromInt(iFirstContactUnixTimeInSeconds, xiiSIUnitOfTime::Second);
    XII_TEST_BOOL(epoch.IsValid());
    XII_TEST_BOOL(firstContact.IsValid());

    // GetInt64 / SetInt64
    xiiTimestamp firstContactTest = xiiTimestamp::MakeFromInt(iFirstContactUnixTimeInSeconds, xiiSIUnitOfTime::Second);
    XII_TEST_INT(firstContactTest.GetInt64(xiiSIUnitOfTime::Second), iFirstContactUnixTimeInSeconds);
    XII_TEST_INT(firstContactTest.GetInt64(xiiSIUnitOfTime::Millisecond), iFirstContactUnixTimeInSeconds * 1000LL);
    XII_TEST_INT(firstContactTest.GetInt64(xiiSIUnitOfTime::Microsecond), iFirstContactUnixTimeInSeconds * 1000000LL);
    XII_TEST_INT(firstContactTest.GetInt64(xiiSIUnitOfTime::Nanosecond), iFirstContactUnixTimeInSeconds * 1000000000LL);

    firstContactTest = xiiTimestamp::MakeFromInt(firstContactTest.GetInt64(xiiSIUnitOfTime::Second), xiiSIUnitOfTime::Second);
    XII_TEST_BOOL(firstContactTest.Compare(firstContact, xiiTimestamp::CompareMode::Identical));
    firstContactTest = xiiTimestamp::MakeFromInt(firstContactTest.GetInt64(xiiSIUnitOfTime::Millisecond), xiiSIUnitOfTime::Millisecond);
    XII_TEST_BOOL(firstContactTest.Compare(firstContact, xiiTimestamp::CompareMode::Identical));
    firstContactTest = xiiTimestamp::MakeFromInt(firstContactTest.GetInt64(xiiSIUnitOfTime::Microsecond), xiiSIUnitOfTime::Microsecond);
    XII_TEST_BOOL(firstContactTest.Compare(firstContact, xiiTimestamp::CompareMode::Identical));
    firstContactTest = xiiTimestamp::MakeFromInt(firstContactTest.GetInt64(xiiSIUnitOfTime::Nanosecond), xiiSIUnitOfTime::Nanosecond);
    XII_TEST_BOOL(firstContactTest.Compare(firstContact, xiiTimestamp::CompareMode::Identical));

    // IsEqual
    const xiiTimestamp firstContactPlusAFewMicroseconds = xiiTimestamp::MakeFromInt(firstContact.GetInt64(xiiSIUnitOfTime::Microsecond) + 42, xiiSIUnitOfTime::Microsecond);
    XII_TEST_BOOL(firstContact.Compare(firstContactPlusAFewMicroseconds, xiiTimestamp::CompareMode::FileTimeEqual));
    XII_TEST_BOOL(!firstContact.Compare(firstContactPlusAFewMicroseconds, xiiTimestamp::CompareMode::Identical));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operators")
  {
    const xiiTimestamp firstContact = xiiTimestamp::MakeFromInt(iFirstContactUnixTimeInSeconds, xiiSIUnitOfTime::Second);

    // Time span arithmetics
    const xiiTime timeSpan1000s = xiiTime::MakeFromSeconds(1000);
    XII_TEST_BOOL(timeSpan1000s.GetMicroseconds() == 1000000000LL);

    // operator +
    const xiiTimestamp firstContactPlus1000s = firstContact + timeSpan1000s;
    xiiInt64           iSpanDiff             = firstContactPlus1000s.GetInt64(xiiSIUnitOfTime::Microsecond) - firstContact.GetInt64(xiiSIUnitOfTime::Microsecond);
    XII_TEST_BOOL(iSpanDiff == 1000000000LL);
    // You can only subtract points in time
    XII_TEST_BOOL(firstContactPlus1000s - firstContact == timeSpan1000s);

    const xiiTimestamp T1000sPlusFirstContact = timeSpan1000s + firstContact;
    iSpanDiff                                 = T1000sPlusFirstContact.GetInt64(xiiSIUnitOfTime::Microsecond) - firstContact.GetInt64(xiiSIUnitOfTime::Microsecond);
    XII_TEST_BOOL(iSpanDiff == 1000000000LL);
    // You can only subtract points in time
    XII_TEST_BOOL(T1000sPlusFirstContact - firstContact == timeSpan1000s);

    // operator -
    const xiiTimestamp firstContactMinus1000s = firstContact - timeSpan1000s;
    iSpanDiff                                 = firstContactMinus1000s.GetInt64(xiiSIUnitOfTime::Microsecond) - firstContact.GetInt64(xiiSIUnitOfTime::Microsecond);
    XII_TEST_BOOL(iSpanDiff == -1000000000LL);
    // You can only subtract points in time
    XII_TEST_BOOL(firstContact - firstContactMinus1000s == timeSpan1000s);


    // operator += / -=
    xiiTimestamp testTimestamp = firstContact;
    testTimestamp += timeSpan1000s;
    XII_TEST_BOOL(testTimestamp.Compare(firstContactPlus1000s, xiiTimestamp::CompareMode::Identical));
    testTimestamp -= timeSpan1000s;
    XII_TEST_BOOL(testTimestamp.Compare(firstContact, xiiTimestamp::CompareMode::Identical));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiDateTime conversion")
  {
    // Constructor
    xiiDateTime invalidDateTime;
    XII_TEST_BOOL(!invalidDateTime.GetTimestamp().IsValid());

    const xiiTimestamp firstContact         = xiiTimestamp::MakeFromInt(iFirstContactUnixTimeInSeconds, xiiSIUnitOfTime::Second);
    xiiDateTime        firstContactDataTime = xiiDateTime::MakeFromTimestamp(firstContact);

    // Getter
    XII_TEST_INT(firstContactDataTime.GetYear(), 2063);
    XII_TEST_INT(firstContactDataTime.GetMonth(), 4);
    XII_TEST_INT(firstContactDataTime.GetDay(), 5);
    XII_TEST_BOOL(firstContactDataTime.GetDayOfWeek() == 4 || firstContactDataTime.GetDayOfWeek() == 255); // not supported on all platforms, should output 255 then
    XII_TEST_INT(firstContactDataTime.GetHour(), 0);
    XII_TEST_INT(firstContactDataTime.GetMinute(), 0);
    XII_TEST_INT(firstContactDataTime.GetSecond(), 0);
    XII_TEST_INT(firstContactDataTime.GetMicroseconds(), 0);

    // SetTimestamp / GetTimestamp
    xiiTimestamp currentTimestamp = xiiTimestamp::CurrentTimestamp();
    xiiDateTime  currentDateTime;
    currentDateTime.SetFromTimestamp(currentTimestamp).AssertSuccess();
    xiiTimestamp currentTimestamp2 = currentDateTime.GetTimestamp();
    // OS date time functions should be accurate within one second.
    xiiInt64 iDiff = xiiMath::Abs(currentTimestamp.GetInt64(xiiSIUnitOfTime::Microsecond) - currentTimestamp2.GetInt64(xiiSIUnitOfTime::Microsecond));
    XII_TEST_BOOL(iDiff <= 1000000);

    // Setter
    xiiDateTime oneSmallStep;
    oneSmallStep.SetYear(1969);
    oneSmallStep.SetMonth(7);
    oneSmallStep.SetDay(21);
    oneSmallStep.SetDayOfWeek(1);
    oneSmallStep.SetHour(2);
    oneSmallStep.SetMinute(56);
    oneSmallStep.SetSecond(0);
    oneSmallStep.SetMicroseconds(0);

    xiiTimestamp oneSmallStepTimestamp = oneSmallStep.GetTimestamp();
    XII_TEST_BOOL(oneSmallStepTimestamp.IsValid());
    XII_TEST_INT(oneSmallStepTimestamp.GetInt64(xiiSIUnitOfTime::Second), -14159040LL);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiDateTime formatting")
  {
    xiiDateTime dateTime;

    dateTime.SetYear(2019);
    dateTime.SetMonth(8);
    dateTime.SetDay(16);
    dateTime.SetDayOfWeek(5);
    dateTime.SetHour(13);
    dateTime.SetMinute(40);
    dateTime.SetSecond(30);
    dateTime.SetMicroseconds(345678);

    char szTimestampFormatted[256] = "";

    // no names, no UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::Default));
    XII_TEST_STRING("2019-08-16 - 13:40:30", szTimestampFormatted);
    // no names, no UTC, with milliseconds
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::Default | xiiArgDateTime::ShowMilliseconds));
    XII_TEST_STRING("2019-08-16 - 13:40:30.345", szTimestampFormatted);
    // no names, with UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::Default | xiiArgDateTime::ShowTimeZone));
    XII_TEST_STRING("2019-08-16 - 13:40:30 (UTC)", szTimestampFormatted);
    // no names, with UTC, with milliseconds
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::ShowDate | xiiArgDateTime::ShowMilliseconds | xiiArgDateTime::ShowTimeZone));
    XII_TEST_STRING("2019-08-16 - 13:40:30.345 (UTC)", szTimestampFormatted);
    // with names, no UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::DefaultTextual | xiiArgDateTime::ShowWeekday));
    XII_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30", szTimestampFormatted);
    // no names, no UTC, with milliseconds
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::DefaultTextual | xiiArgDateTime::ShowWeekday | xiiArgDateTime::ShowMilliseconds));
    XII_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30.345", szTimestampFormatted);
    // no names, with UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::DefaultTextual | xiiArgDateTime::ShowWeekday | xiiArgDateTime::ShowTimeZone));
    XII_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30 (UTC)", szTimestampFormatted);
    // no names, with UTC, with milliseconds
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::DefaultTextual | xiiArgDateTime::ShowWeekday | xiiArgDateTime::ShowMilliseconds | xiiArgDateTime::ShowTimeZone));
    XII_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30.345 (UTC)", szTimestampFormatted);

    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::ShowDate));
    XII_TEST_STRING("2019-08-16", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::TextualDate));
    XII_TEST_STRING("2019 Aug 16", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::ShowTime));
    XII_TEST_STRING("13:40", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, xiiArgDateTime(dateTime, xiiArgDateTime::ShowWeekday | xiiArgDateTime::ShowMilliseconds));
    XII_TEST_STRING("(Fri) - 13:40:30.345", szTimestampFormatted);
  }
}
