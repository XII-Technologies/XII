#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

xiiFormatString::xiiFormatString(const xiiStringBuilder& s)
{
  m_szString = s.GetData();
}

const char* xiiFormatString::BuildFormattedText(xiiStringBuilder& ref_sStorage, xiiStringView* pArgs, xiiUInt32 uiNumArgs) const
{
  const char* szString = m_szString;

  xiiUInt32 uiLastParam = -1;

  ref_sStorage.Clear();
  while (*szString != '\0')
  {
    if (*szString == '%')
    {
      if (*(szString + 1) == '%')
      {
        ref_sStorage.Append("%"_xiisv);
      }
      else
      {
        XII_ASSERT_DEBUG(false, "Single percentage signs are not allowed in xiiFormatString. Did you forgot to migrate a printf-style "
                                "string? Use double percentage signs for the actual character.");
      }

      szString += 2;
    }
    else if (*szString == '{' && *(szString + 1) >= '0' && *(szString + 1) <= '9' && *(szString + 2) == '}')
    {
      uiLastParam = *(szString + 1) - '0';
      XII_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }

      szString += 3;
    }
    else if (*szString == '{' && *(szString + 1) == '}')
    {
      ++uiLastParam;
      XII_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }

      szString += 2;
    }
    else
    {
      const xiiUInt32 character = xiiUnicodeUtils::DecodeUtf8ToUtf32(szString);
      ref_sStorage.Append(character);
    }
  }

  return ref_sStorage.GetData();
}

//////////////////////////////////////////////////////////////////////////

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgI& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase);
  szTmp[writepos] = '\0';
  return xiiStringView(szTmp, szTmp + writepos);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiInt64 iArg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, iArg, 1, false, 10);
  szTmp[writepos] = '\0';
  return xiiStringView(szTmp, szTmp + writepos);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiInt32 iArg)
{
  return BuildString(szTmp, uiLength, (xiiInt64)iArg);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgU& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedUInt(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase, arg.m_bUpperCase);
  szTmp[writepos] = '\0';
  return xiiStringView(szTmp, szTmp + writepos);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiUInt64 uiArg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedUInt(szTmp, uiLength, writepos, uiArg, 1, false, 10, false);
  szTmp[writepos] = '\0';
  return xiiStringView(szTmp, szTmp + writepos);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiUInt32 uiArg)
{
  return BuildString(szTmp, uiLength, (xiiUInt64)uiArg);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgF& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_iPrecision, arg.m_bScientific);
  szTmp[writepos] = '\0';
  return xiiStringView(szTmp, szTmp + writepos);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, double fArg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, fArg, 1, false, -1, false);
  szTmp[writepos] = '\0';
  return xiiStringView(szTmp, szTmp + writepos);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, bool bArg)
{
  if (bArg)
    return "true";

  return "false";
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const char* szArg)
{
  return szArg;
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const wchar_t* pArg)
{
  const char* start = szTmp;
  if (pArg != nullptr)
  {
    // Code points in UTF-8 can be up to 4 byte, so the end pointer is 3 byte "earlier" than for
    // for a single byte character. One byte for trailing zero is already accounted for in uiLength.
    const char* tmpEnd = szTmp + uiLength - 3u;
    while (*pArg != '\0' && szTmp < tmpEnd)
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeWCharToUtf32(pArg);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, szTmp);
    }
  }

  // Append terminator. As the extra byte for trailing zero is accounted for in uiLength, this is safe.
  *szTmp = '\0';

  return start;
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiString& sArg)
{
  return xiiStringView(sArg.GetData(), sArg.GetData() + sArg.GetElementCount());
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiHashedString& sArg)
{
  return xiiStringView(sArg.GetData(), sArg.GetData() + sArg.GetString().GetElementCount());
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiStringBuilder& sArg)
{
  return xiiStringView(sArg.GetData(), sArg.GetData() + sArg.GetElementCount());
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiUntrackedString& sArg)
{
  return xiiStringView(sArg.GetData(), sArg.GetData() + sArg.GetElementCount());
}

const xiiStringView& BuildString(char* szTmp, xiiUInt32 uiLength, const xiiStringView& sArg)
{
  return sArg;
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgC& arg)
{
  szTmp[0] = arg.m_Value;
  szTmp[1] = '\0';

  return xiiStringView(&szTmp[0], &szTmp[1]);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgP& arg)
{
  xiiStringUtils::snprintf(szTmp, uiLength, "%p", arg.m_Value);
  return xiiStringView(szTmp);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiResult arg)
{
  if (arg.Failed())
    return "<failed>";
  else
    return "<succeeded>";
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiVariant& arg)
{
  xiiString sString = arg.ConvertTo<xiiString>();
  xiiStringUtils::snprintf(szTmp, uiLength, "%s", sString.GetData());
  return xiiStringView(szTmp);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiAngleTemplate<float>& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedFloat(szTmp, uiLength - 2, writepos, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  szTmp[writepos + 0] = (char)0xC2;
  szTmp[writepos + 1] = (char)0xB0;
  szTmp[writepos + 2] = '\0';

  return xiiStringView(szTmp, szTmp + writepos + 2);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiAngleTemplate<double>& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedFloat(szTmp, uiLength - 2, writepos, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  szTmp[writepos + 0] = (char)0xC2;
  szTmp[writepos + 1] = (char)0xB0;
  szTmp[writepos + 2] = '\0';

  return xiiStringView(szTmp, szTmp + writepos + 2);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiRational& arg)
{
  xiiUInt32 writepos = 0;

  if (arg.IsIntegral())
  {
    xiiStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.GetIntegralResult(), 1, false, 10);

    return xiiStringView(szTmp, szTmp + writepos);
  }
  else
  {
    xiiStringUtils::snprintf(szTmp, uiLength, "%i/%i", arg.GetNumerator(), arg.GetDenominator());

    return xiiStringView(szTmp);
  }
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiTime& arg)
{
  xiiUInt32 writepos = 0;

  const double fAbsSec = xiiMath::Abs(arg.GetSeconds());

  if (fAbsSec < 0.000001)
  {
    xiiStringUtils::OutputFormattedFloat(szTmp, uiLength - 5, writepos, arg.GetNanoseconds(), 1, false, 1, false, true);
    // tmp[writepos++] = ' ';
    szTmp[writepos++] = 'n';
    szTmp[writepos++] = 's';
  }
  else if (fAbsSec < 0.001)
  {
    xiiStringUtils::OutputFormattedFloat(szTmp, uiLength - 5, writepos, arg.GetMicroseconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    // Utf-8 representation of the microsecond (us) sign
    szTmp[writepos++] = (char)0xC2;
    szTmp[writepos++] = (char)0xB5;
    szTmp[writepos++] = 's';
  }
  else if (fAbsSec < 1.0)
  {
    xiiStringUtils::OutputFormattedFloat(szTmp, uiLength - 5, writepos, arg.GetMilliseconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    szTmp[writepos++] = 'm';
    szTmp[writepos++] = 's';
  }
  else if (fAbsSec < 60.0)
  {
    xiiStringUtils::OutputFormattedFloat(szTmp, uiLength - 5, writepos, arg.GetSeconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    szTmp[writepos++] = 's';
    szTmp[writepos++] = 'e';
    szTmp[writepos++] = 'c';
  }
  else if (fAbsSec < 60.0 * 60.0)
  {
    double tRem = fAbsSec;

    xiiInt32 iMin = static_cast<xiiInt32>(xiiMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;
    iMin *= xiiMath::Sign(static_cast<xiiInt32>(arg.GetSeconds()));

    const xiiInt32 iSec = static_cast<xiiInt32>(xiiMath::Trunc(tRem));

    writepos = xiiStringUtils::snprintf(szTmp, uiLength, "%imin %isec", iMin, iSec);
  }
  else
  {
    double tRem = fAbsSec;

    xiiInt32 iHrs = static_cast<xiiInt32>(xiiMath::Trunc(tRem / (60.0 * 60.0)));
    tRem -= iHrs * 60 * 60;
    iHrs *= xiiMath::Sign(static_cast<xiiInt32>(arg.GetSeconds()));

    const xiiInt32 iMin = static_cast<xiiInt32>(xiiMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;

    const xiiInt32 iSec = static_cast<xiiInt32>(xiiMath::Trunc(tRem));

    writepos = xiiStringUtils::snprintf(szTmp, uiLength, "%ih %imin %isec", iHrs, iMin, iSec);
  }

  szTmp[writepos] = '\0';
  return xiiStringView(szTmp, szTmp + writepos);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgHumanReadable& arg)
{
  xiiUInt32 suffixIndex = 0;
  xiiUInt64 divider     = 1;
  double    absValue    = xiiMath::Abs(arg.m_Value);
  while (absValue / divider >= arg.m_Base && suffixIndex < arg.m_SuffixCount - 1)
  {
    divider *= arg.m_Base;
    ++suffixIndex;
  }

  xiiUInt32 writepos = 0;
  if (divider == 1 && xiiMath::Fraction(arg.m_Value) == 0.0)
  {
    xiiStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, static_cast<xiiInt64>(arg.m_Value), 1, false, 10);
  }
  else
  {
    xiiStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, arg.m_Value / divider, 1, false, 2, false);
  }
  xiiStringUtils::Copy(szTmp + writepos, uiLength - writepos, arg.m_Suffixes[suffixIndex]);

  return xiiStringView(szTmp);
}

xiiArgSensitive::BuildStringCallback xiiArgSensitive::s_BuildStringCB = nullptr;

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgSensitive& arg)
{
  if (xiiArgSensitive::s_BuildStringCB)
  {
    return xiiArgSensitive::s_BuildStringCB(szTmp, uiLength, arg);
  }

  return arg.m_sSensitiveInfo;
}

xiiStringView xiiArgSensitive::BuildString_SensitiveUserData_Hash(char* szTmp, xiiUInt32 uiLength, const xiiArgSensitive& arg)
{
  const xiiUInt32 len = arg.m_sSensitiveInfo.GetElementCount();

  if (len == 0)
    return xiiStringView();

  if (!xiiStringUtils::IsNullOrEmpty(arg.m_szContext))
  {
    xiiStringUtils::snprintf(
      szTmp, uiLength, "sud:%s#%08x($%u)", arg.m_szContext, xiiHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }
  else
  {
    xiiStringUtils::snprintf(szTmp, uiLength, "sud:#%08x($%u)", xiiHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }

  return szTmp;
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgErrorCode& arg)
{
  LPVOID lpMsgBuf = nullptr;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, arg.m_ErrorCode,
                     MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&lpMsgBuf, 0, nullptr) == 0)
  {
    DWORD err = GetLastError();
    xiiStringUtils::snprintf(szTmp, uiLength, "%i (FormatMessageW failed with error code %i)", arg.m_ErrorCode, err);
    return xiiStringView(szTmp);
  }

  LPWSTR pCRLF = wcschr((LPWSTR)lpMsgBuf, L'\r');
  if (pCRLF != nullptr)
  {
    // remove the \r\n that FormatMessageW always appends
    *pCRLF = L'\0';
  }

  // we need a bigger boat
  static thread_local char FullMessage[256];

  xiiStringUtils::snprintf(FullMessage, XII_ARRAY_SIZE(FullMessage), "%i (\"%s\")", arg.m_ErrorCode, xiiStringUtf8((LPWSTR)lpMsgBuf).GetData());
  LocalFree(lpMsgBuf);
  return xiiStringView(FullMessage);
}
#endif


XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_FormatString);
