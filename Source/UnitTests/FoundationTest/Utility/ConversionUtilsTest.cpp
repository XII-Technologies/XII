#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/ConversionUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(Utility);

XII_CREATE_SIMPLE_TEST(Utility, ConversionUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StringToInt")
  {
    const char* szString    = "1a";
    const char* szResultPos = nullptr;

    xiiInt32 iRes = 42;
    szString      = "01234";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 1234);
    XII_TEST_BOOL(szResultPos == szString + 5);

    iRes     = 42;
    szString = "0";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 0);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "0000";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 0);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "-999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, -999999);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "-+999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, -999999);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "--999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 999999);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "++---+--+--999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, -999999);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "++--+--+--999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 999999);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "123+456";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 123);
    XII_TEST_BOOL(szResultPos == szString + 3);

    iRes     = 42;
    szString = "123_456";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 123);
    XII_TEST_BOOL(szResultPos == szString + 3);

    iRes     = 42;
    szString = "-123-456";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, -123);
    XII_TEST_BOOL(szResultPos == szString + 4);


    iRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(nullptr, iRes) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToInt("", iRes) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToInt("a", iRes) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToInt("a15", iRes) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToInt("+", iRes) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToInt("-", iRes) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes     = 42;
    szString = "1a";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 1);
    XII_TEST_BOOL(szResultPos == szString + 1);

    iRes     = 42;
    szString = "0 23";
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 0);
    XII_TEST_BOOL(szResultPos == szString + 1);

    // overflow check

    iRes     = 42;
    szString = "0002147483647"; // valid
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 2147483647);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "-2147483648"; // valid
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, (xiiInt32)0x80000000);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "0002147483648"; // invalid
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes     = 42;
    szString = "-2147483649"; // invalid
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes     = 42;
    szString = "100'000"; // valid with c++ separator
    XII_TEST_BOOL(xiiConversionUtils::StringToInt(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 100'000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StringToUInt")
  {
    const char* szString    = "1a";
    const char* szResultPos = nullptr;

    xiiUInt32 uiRes = 42;
    szString        = "01234";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 1234);
    XII_TEST_BOOL(szResultPos == szString + 5);

    uiRes    = 42;
    szString = "0";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 0);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    uiRes    = 42;
    szString = "0000";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 0);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    uiRes    = 42;
    szString = "-999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    uiRes    = 42;
    szString = "-+999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    uiRes    = 42;
    szString = "--999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 999999);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    uiRes    = 42;
    szString = "++---+--+--999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    uiRes    = 42;
    szString = "++--+--+--999999";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 999999);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    uiRes    = 42;
    szString = "123+456";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 123);
    XII_TEST_BOOL(szResultPos == szString + 3);

    uiRes    = 42;
    szString = "123_456";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 123);
    XII_TEST_BOOL(szResultPos == szString + 3);

    uiRes    = 42;
    szString = "-123-456";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);
    XII_TEST_BOOL(szResultPos == szString + 4);


    uiRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(nullptr, uiRes) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);

    uiRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt("", uiRes) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);

    uiRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt("a", uiRes) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);

    uiRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt("a15", uiRes) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);

    uiRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt("+", uiRes) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);

    uiRes = 42;
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt("-", uiRes) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);

    uiRes    = 42;
    szString = "1a";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 1);
    XII_TEST_BOOL(szResultPos == szString + 1);

    uiRes    = 42;
    szString = "0 23";
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 0);
    XII_TEST_BOOL(szResultPos == szString + 1);

    // overflow check

    uiRes    = 42;
    szString = "0004294967295"; // valid
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(uiRes, 4294967295u);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    uiRes    = 42;
    szString = "0004294967296"; // invalid
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);

    uiRes    = 42;
    szString = "-1"; // invalid
    XII_TEST_BOOL(xiiConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(uiRes, 42);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StringToInt64")
  {
    // overflow check
    xiiInt64    iRes        = 42;
    const char* szString    = "0002147483639"; // valid
    const char* szResultPos = nullptr;

    XII_TEST_BOOL(xiiConversionUtils::StringToInt64(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 2147483639);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "0002147483640"; // also valid with 64bit
    XII_TEST_BOOL(xiiConversionUtils::StringToInt64(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 2147483640);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "0009223372036854775807"; // last valid positive number
    XII_TEST_BOOL(xiiConversionUtils::StringToInt64(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, 9223372036854775807);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "0009223372036854775808"; // invalid
    XII_TEST_BOOL(xiiConversionUtils::StringToInt64(szString, iRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);

    iRes     = 42;
    szString = "-9223372036854775808"; // last valid negative number
    XII_TEST_BOOL(xiiConversionUtils::StringToInt64(szString, iRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_INT(iRes, (xiiInt64)0x8000000000000000);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    iRes     = 42;
    szString = "-9223372036854775809"; // invalid
    XII_TEST_BOOL(xiiConversionUtils::StringToInt64(szString, iRes, &szResultPos) == XII_FAILURE);
    XII_TEST_INT(iRes, 42);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StringToFloat")
  {
    const char* szString    = nullptr;
    const char* szResultPos = nullptr;

    double fRes = 42;
    szString    = "23.45";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 23.45, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "-2345";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, -2345.0, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "-0";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 0.0, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "0_0000.0_00000_";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 0.0, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "_0_0000.0_00000_";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_FAILURE);

    fRes     = 42;
    szString = ".123456789";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 0.123456789, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "+123E1";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 1230.0, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "  \r\t 123e0";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 123.0, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "\n123e6";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 123000000.0, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "\n1_2_3e+6";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 123000000.0, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = "  123E-6";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 0.000123, 0.00001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = " + - -+-123.45e-10";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, -0.000000012345, 0.0000001);
    XII_TEST_BOOL(szResultPos == szString + xiiStringUtils::GetStringElementCount(szString));

    fRes     = 42;
    szString = nullptr;
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_FAILURE);
    XII_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes     = 42;
    szString = "";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_FAILURE);
    XII_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes     = 42;
    szString = "-----";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_FAILURE);
    XII_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes     = 42;
    szString = " + - +++ - \r \n";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_FAILURE);
    XII_TEST_DOUBLE(fRes, 42.0, 0.00001);


    fRes     = 42;
    szString = "65.345789xabc";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 65.345789, 0.000001);
    XII_TEST_BOOL(szResultPos == szString + 9);

    fRes     = 42;
    szString = " \n \r \t + - 2314565.345789ff xabc";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, -2314565.345789, 0.000001);
    XII_TEST_BOOL(szResultPos == szString + 25);

    fRes     = 42;
    szString = "100'000.0";
    XII_TEST_BOOL(xiiConversionUtils::StringToFloat(szString, fRes, &szResultPos) == XII_SUCCESS);
    XII_TEST_DOUBLE(fRes, 100'000.0, 0.000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StringToBool")
  {
    const char* szString    = "";
    const char* szResultPos = nullptr;
    bool        bRes        = false;

    // true / false
    {
      bRes        = false;
      szString    = "true,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(bRes);
      XII_TEST_BOOL(*szResultPos == ',');

      bRes        = true;
      szString    = "FALSe,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(!bRes);
      XII_TEST_BOOL(*szResultPos == ',');
    }

    // on / off
    {
      bRes        = false;
      szString    = "\n on,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(bRes);
      XII_TEST_BOOL(*szResultPos == ',');

      bRes        = true;
      szString    = "\t\t \toFf,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(!bRes);
      XII_TEST_BOOL(*szResultPos == ',');
    }

    // 1 / 0
    {
      bRes        = false;
      szString    = "\r1,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(bRes);
      XII_TEST_BOOL(*szResultPos == ',');

      bRes        = true;
      szString    = "0,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(!bRes);
      XII_TEST_BOOL(*szResultPos == ',');
    }

    // yes / no
    {
      bRes        = false;
      szString    = "yes,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(bRes);
      XII_TEST_BOOL(*szResultPos == ',');

      bRes        = true;
      szString    = "NO,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(!bRes);
      XII_TEST_BOOL(*szResultPos == ',');
    }

    // enable / disable
    {
      bRes        = false;
      szString    = "enable,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(bRes);
      XII_TEST_BOOL(*szResultPos == ',');

      bRes        = true;
      szString    = "disABle,";
      szResultPos = nullptr;
      XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_SUCCESS);
      XII_TEST_BOOL(!bRes);
      XII_TEST_BOOL(*szResultPos == ',');
    }

    bRes = false;

    szString    = "of,";
    szResultPos = nullptr;
    XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_FAILURE);
    XII_TEST_BOOL(szResultPos == nullptr);

    szString    = "aon";
    szResultPos = nullptr;
    XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_FAILURE);
    XII_TEST_BOOL(szResultPos == nullptr);

    szString    = "";
    szResultPos = nullptr;
    XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_FAILURE);
    XII_TEST_BOOL(szResultPos == nullptr);

    szString    = nullptr;
    szResultPos = nullptr;
    XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_FAILURE);
    XII_TEST_BOOL(szResultPos == nullptr);

    szString    = "tut";
    szResultPos = nullptr;
    XII_TEST_BOOL(xiiConversionUtils::StringToBool(szString, bRes, &szResultPos) == XII_FAILURE);
    XII_TEST_BOOL(szResultPos == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HexCharacterToIntValue")
  {
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('0'), 0);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('1'), 1);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('2'), 2);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('3'), 3);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('4'), 4);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('5'), 5);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('6'), 6);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('7'), 7);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('8'), 8);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('9'), 9);

    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('a'), 10);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('b'), 11);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('c'), 12);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('d'), 13);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('e'), 14);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('f'), 15);

    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('A'), 10);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('B'), 11);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('C'), 12);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('D'), 13);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('E'), 14);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('F'), 15);

    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('g'), -1);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('h'), -1);
    XII_TEST_INT(xiiConversionUtils::HexCharacterToIntValue('i'), -1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ConvertHexStringToUInt32")
  {
    xiiUInt32 res;

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("", res).Succeeded());
    XII_TEST_BOOL(res == 0);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("0x", res).Succeeded());
    XII_TEST_BOOL(res == 0);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("0", res).Succeeded());
    XII_TEST_BOOL(res == 0);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("0x0", res).Succeeded());
    XII_TEST_BOOL(res == 0);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("a", res).Succeeded());
    XII_TEST_BOOL(res == 10);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("0xb", res).Succeeded());
    XII_TEST_BOOL(res == 11);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("000c", res).Succeeded());
    XII_TEST_BOOL(res == 12);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("AA", res).Succeeded());
    XII_TEST_BOOL(res == 170);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("aAjbB", res).Failed());

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("aAbB", res).Succeeded());
    XII_TEST_BOOL(res == 43707);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("FFFFffff", res).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFFF);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("0000FFFFffff", res).Succeeded());
    XII_TEST_BOOL(res == 0xFFFF);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt32("100000000", res).Succeeded());
    XII_TEST_BOOL(res == 0x10000000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ConvertHexStringToUInt64")
  {
    xiiUInt64 res;

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("", res).Succeeded());
    XII_TEST_BOOL(res == 0);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("0x", res).Succeeded());
    XII_TEST_BOOL(res == 0);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("0", res).Succeeded());
    XII_TEST_BOOL(res == 0);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("0x0", res).Succeeded());
    XII_TEST_BOOL(res == 0);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("a", res).Succeeded());
    XII_TEST_BOOL(res == 10);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("0xb", res).Succeeded());
    XII_TEST_BOOL(res == 11);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("000c", res).Succeeded());
    XII_TEST_BOOL(res == 12);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("AA", res).Succeeded());
    XII_TEST_BOOL(res == 170);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("aAjbB", res).Failed());

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("aAbB", res).Succeeded());
    XII_TEST_BOOL(res == 43707);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("FFFFffff", res).Succeeded());
    XII_TEST_BOOL(res == 4294967295);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("0000FFFFffff", res).Succeeded());
    XII_TEST_BOOL(res == 4294967295);

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("0xfffffffffffffffy", res).Failed());

    XII_TEST_BOOL(xiiConversionUtils::ConvertHexStringToUInt64("0xffffffffffffffffy", res).Succeeded());
    XII_TEST_BOOL(res == 0xFFFFFFFFFFFFFFFFllu);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ConvertBinaryToHex and ConvertHexStringToBinary")
  {
    xiiDynamicArray<xiiUInt8> binary;
    binary.SetCountUninitialized(1024);

    xiiRandom r;
    r.InitializeFromCurrentTime();

    for (auto& val : binary)
    {
      val = static_cast<xiiUInt8>(r.UIntInRange(256u));
    }

    xiiStringBuilder sHex;
    xiiConversionUtils::ConvertBinaryToHex(binary.GetData(), binary.GetCount(), [&sHex](const char* s) { sHex.Append(s); });

    xiiDynamicArray<xiiUInt8> binary2;
    binary2.SetCountUninitialized(1024);

    xiiConversionUtils::ConvertHexToBinary(sHex, binary2.GetData(), binary2.GetCount());

    XII_TEST_BOOL(binary == binary2);
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExtractFloatsFromString")
  {
    float v[16];

    const char* szText = "This 1 is 2.3 or 3.141 tests in 1.2 strings, maybe 4.5,6.78or9.101!";

    xiiMemoryUtils::ZeroFill(v, 16);
    XII_TEST_INT(xiiConversionUtils::ExtractFloatsFromString(szText, 0, v), 0);
    XII_TEST_FLOAT(v[0], 0.0f, 0.0f);

    xiiMemoryUtils::ZeroFill(v, 16);
    XII_TEST_INT(xiiConversionUtils::ExtractFloatsFromString(szText, 3, v), 3);
    XII_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    XII_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    XII_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    XII_TEST_FLOAT(v[3], 0.0f, 0.0f);

    xiiMemoryUtils::ZeroFill(v, 16);
    XII_TEST_INT(xiiConversionUtils::ExtractFloatsFromString(szText, 6, v), 6);
    XII_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    XII_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    XII_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    XII_TEST_FLOAT(v[3], 1.2f, 0.0001f);
    XII_TEST_FLOAT(v[4], 4.5f, 0.0001f);
    XII_TEST_FLOAT(v[5], 6.78f, 0.0001f);
    XII_TEST_FLOAT(v[6], 0.0f, 0.0f);

    xiiMemoryUtils::ZeroFill(v, 16);
    XII_TEST_INT(xiiConversionUtils::ExtractFloatsFromString(szText, 10, v), 7);
    XII_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    XII_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    XII_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    XII_TEST_FLOAT(v[3], 1.2f, 0.0001f);
    XII_TEST_FLOAT(v[4], 4.5f, 0.0001f);
    XII_TEST_FLOAT(v[5], 6.78f, 0.0001f);
    XII_TEST_FLOAT(v[6], 9.101f, 0.0001f);
    XII_TEST_FLOAT(v[7], 0.0f, 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ConvertStringToUuid and IsStringUuid")
  {
    xiiUuid          guid;
    xiiStringBuilder sGuid;

    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      guid = xiiUuid::MakeUuid();

      xiiConversionUtils::ToString(guid, sGuid);

      XII_TEST_BOOL(xiiConversionUtils::IsStringUuid(sGuid));

      xiiUuid guid2 = xiiConversionUtils::ConvertStringToUuid(sGuid);

      XII_TEST_BOOL(guid == guid2);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetColorName")
  {
    XII_TEST_STRING(xiiString(xiiConversionUtils::GetColorName(xiiColorGammaUB(1, 2, 3))), "#010203");
    XII_TEST_STRING(xiiString(xiiConversionUtils::GetColorName(xiiColorGammaUB(10, 20, 30, 40))), "#0A141E28");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetColorByName")
  {
    XII_TEST_BOOL(xiiConversionUtils::GetColorByName("#010203") == xiiColorGammaUB(1, 2, 3));
    XII_TEST_BOOL(xiiConversionUtils::GetColorByName("#0A141E28") == xiiColorGammaUB(10, 20, 30, 40));

    XII_TEST_BOOL(xiiConversionUtils::GetColorByName("#010203") == xiiColorGammaUB(1, 2, 3));
    XII_TEST_BOOL(xiiConversionUtils::GetColorByName("#0a141e28") == xiiColorGammaUB(10, 20, 30, 40));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetColorByName and GetColorName")
  {
#define Check(name)                                                                         \
  {                                                                                         \
    bool           valid = false;                                                           \
    const xiiColor c     = xiiConversionUtils::GetColorByName(XII_PP_STRINGIFY(name), &valid); \
    XII_TEST_BOOL(valid);                                                                   \
    xiiString sName = xiiConversionUtils::GetColorName(c);                                  \
    XII_TEST_STRING(sName, XII_PP_STRINGIFY(name));                                            \
  }

#define Check2(name, otherName)                                                             \
  {                                                                                         \
    bool           valid = false;                                                           \
    const xiiColor c     = xiiConversionUtils::GetColorByName(XII_PP_STRINGIFY(name), &valid); \
    XII_TEST_BOOL(valid);                                                                   \
    xiiString sName = xiiConversionUtils::GetColorName(c);                                  \
    XII_TEST_STRING(sName, XII_PP_STRINGIFY(otherName));                                       \
  }

    Check(AliceBlue);
    Check(AntiqueWhite);
    Check(Aqua);
    Check(Aquamarine);
    Check(Azure);
    Check(Beige);
    Check(Bisque);
    Check(Black);
    Check(BlanchedAlmond);
    Check(Blue);
    Check(BlueViolet);
    Check(Brown);
    Check(BurlyWood);
    Check(CadetBlue);
    Check(Chartreuse);
    Check(Chocolate);
    Check(Coral);
    Check(CornflowerBlue); // The Original!
    Check(Cornsilk);
    Check(Crimson);
    Check2(Cyan, Aqua);
    Check(DarkBlue);
    Check(DarkCyan);
    Check(DarkGoldenRod);
    Check(DarkGray);
    Check2(DarkGrey, DarkGray);
    Check(DarkGreen);
    Check(DarkKhaki);
    Check(DarkMagenta);
    Check(DarkOliveGreen);
    Check(DarkOrange);
    Check(DarkOrchid);
    Check(DarkRed);
    Check(DarkSalmon);
    Check(DarkSeaGreen);
    Check(DarkSlateBlue);
    Check(DarkSlateGray);
    Check2(DarkSlateGrey, DarkSlateGray);
    Check(DarkTurquoise);
    Check(DarkViolet);
    Check(DeepPink);
    Check(DeepSkyBlue);
    Check(DimGray);
    Check2(DimGrey, DimGray);
    Check(DodgerBlue);
    Check(FireBrick);
    Check(FloralWhite);
    Check(ForestGreen);
    Check(Fuchsia);
    Check(Gainsboro);
    Check(GhostWhite);
    Check(Gold);
    Check(GoldenRod);
    Check(Gray);
    Check2(Grey, Gray);
    Check(Green);
    Check(GreenYellow);
    Check(HoneyDew);
    Check(HotPink);
    Check(IndianRed);
    Check(Indigo);
    Check(Ivory);
    Check(Khaki);
    Check(Lavender);
    Check(LavenderBlush);
    Check(LawnGreen);
    Check(LemonChiffon);
    Check(LightBlue);
    Check(LightCoral);
    Check(LightCyan);
    Check(LightGoldenRodYellow);
    Check(LightGray);
    Check2(LightGrey, LightGray);
    Check(LightGreen);
    Check(LightPink);
    Check(LightSalmon);
    Check(LightSeaGreen);
    Check(LightSkyBlue);
    Check(LightSlateGray);
    Check2(LightSlateGrey, LightSlateGray);
    Check(LightSteelBlue);
    Check(LightYellow);
    Check(Lime);
    Check(LimeGreen);
    Check(Linen);
    Check2(Magenta, Fuchsia);
    Check(Maroon);
    Check(MediumAquaMarine);
    Check(MediumBlue);
    Check(MediumOrchid);
    Check(MediumPurple);
    Check(MediumSeaGreen);
    Check(MediumSlateBlue);
    Check(MediumSpringGreen);
    Check(MediumTurquoise);
    Check(MediumVioletRed);
    Check(MidnightBlue);
    Check(MintCream);
    Check(MistyRose);
    Check(Moccasin);
    Check(NavajoWhite);
    Check(Navy);
    Check(OldLace);
    Check(Olive);
    Check(OliveDrab);
    Check(Orange);
    Check(OrangeRed);
    Check(Orchid);
    Check(PaleGoldenRod);
    Check(PaleGreen);
    Check(PaleTurquoise);
    Check(PaleVioletRed);
    Check(PapayaWhip);
    Check(PeachPuff);
    Check(Peru);
    Check(Pink);
    Check(Plum);
    Check(PowderBlue);
    Check(Purple);
    Check(RebeccaPurple);
    Check(Red);
    Check(RosyBrown);
    Check(RoyalBlue);
    Check(SaddleBrown);
    Check(Salmon);
    Check(SandyBrown);
    Check(SeaGreen);
    Check(SeaShell);
    Check(Sienna);
    Check(Silver);
    Check(SkyBlue);
    Check(SlateBlue);
    Check(SlateGray);
    Check2(SlateGrey, SlateGray);
    Check(Snow);
    Check(SpringGreen);
    Check(SteelBlue);
    Check(Tan);
    Check(Teal);
    Check(Thistle);
    Check(Tomato);
    Check(Turquoise);
    Check(Violet);
    Check(Wheat);
    Check(White);
    Check(WhiteSmoke);
    Check(Yellow);
    Check(YellowGreen);
  }
}
