#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

xiiFormatString::xiiFormatString(const xiiStringBuilder& s)
{
  m_sString = s.GetView();
}

const char* xiiFormatString::GetTextCStr(xiiStringBuilder& out_sString) const
{
  out_sString = m_sString;
  return out_sString.GetData();
}

xiiStringView xiiFormatString::BuildFormattedText(xiiStringBuilder& ref_sStorage, xiiStringView* pArgs, xiiUInt32 uiNumArgs) const
{
  xiiStringView sString = m_sString;

  xiiUInt32 uiLastParam = xiiInvalidIndex;

  ref_sStorage.Clear();
  while (!sString.IsEmpty())
  {
    if (sString.StartsWith("%"))
    {
      if (sString.TrimWordStart("%%"))
      {
        ref_sStorage.Append("%"_xiisv);
      }
      else
      {
        XII_ASSERT_DEBUG(false, "Single percentage signs are not allowed in xiiFormatString. Did you forgot to migrate a printf-style string? Use double percentage signs for the actual character.");
      }
    }
    else if (sString.GetElementCount() >= 3 && *sString.GetStartPointer() == '{' && *(sString.GetStartPointer() + 1) >= '0' && *(sString.GetStartPointer() + 1) <= '9' && *(sString.GetStartPointer() + 2) == '}')
    {
      uiLastParam = *(sString.GetStartPointer() + 1) - '0';
      XII_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }

      sString.ChopAwayFirstCharacterAscii();
      sString.ChopAwayFirstCharacterAscii();
      sString.ChopAwayFirstCharacterAscii();
    }
    else if (sString.TrimWordStart("{}"))
    {
      ++uiLastParam;
      XII_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }
    }
    else
    {
      const xiiUInt32 character = sString.GetCharacter();
      ref_sStorage.Append(character);
      sString.ChopAwayFirstCharacterUtf8();
    }
  }

  return ref_sStorage.GetView();
}

//////////////////////////////////////////////////////////////////////////

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgI& arg)
{
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedInt(szTmp, uiLength, uiWritePosition, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase);
  szTmp[uiWritePosition] = '\0';
  return xiiStringView(szTmp, szTmp + uiWritePosition);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiInt64 iArg)
{
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedInt(szTmp, uiLength, uiWritePosition, iArg, 1, false, 10);
  szTmp[uiWritePosition] = '\0';
  return xiiStringView(szTmp, szTmp + uiWritePosition);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiInt32 iArg)
{
  return BuildString(szTmp, uiLength, (xiiInt64)iArg);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgU& arg)
{
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedUInt(szTmp, uiLength, uiWritePosition, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase, arg.m_bUpperCase);
  szTmp[uiWritePosition] = '\0';
  return xiiStringView(szTmp, szTmp + uiWritePosition);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiUInt64 uiArg)
{
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedUInt(szTmp, uiLength, uiWritePosition, uiArg, 1, false, 10, false);
  szTmp[uiWritePosition] = '\0';
  return xiiStringView(szTmp, szTmp + uiWritePosition);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, xiiUInt32 uiArg)
{
  return BuildString(szTmp, uiLength, (xiiUInt64)uiArg);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgF& arg)
{
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedFloat(szTmp, uiLength, uiWritePosition, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_iPrecision, arg.m_bScientific);
  szTmp[uiWritePosition] = '\0';
  return xiiStringView(szTmp, szTmp + uiWritePosition);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, double fArg)
{
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedFloat(szTmp, uiLength, uiWritePosition, fArg, 1, false, -1, false);
  szTmp[uiWritePosition] = '\0';
  return xiiStringView(szTmp, szTmp + uiWritePosition);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, bool bArg)
{
  XII_IGNORE_UNUSED(szTmp);
  XII_IGNORE_UNUSED(uiLength);
  return bArg ? "true" : "false";
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const char* szArg)
{
  XII_IGNORE_UNUSED(szTmp);
  XII_IGNORE_UNUSED(uiLength);
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
  XII_IGNORE_UNUSED(szTmp);
  XII_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiHashedString& sArg)
{
  XII_IGNORE_UNUSED(szTmp);
  XII_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiStringBuilder& sArg)
{
  XII_IGNORE_UNUSED(szTmp);
  XII_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiUntrackedString& sArg)
{
  XII_IGNORE_UNUSED(szTmp);
  XII_IGNORE_UNUSED(uiLength);
  return sArg.GetView();
}

const xiiStringView& BuildString(char* szTmp, xiiUInt32 uiLength, const xiiStringView& sArg)
{
  XII_IGNORE_UNUSED(szTmp);
  XII_IGNORE_UNUSED(uiLength);
  return sArg;
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgC& arg)
{
  XII_IGNORE_UNUSED(uiLength);

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
  XII_IGNORE_UNUSED(szTmp);
  XII_IGNORE_UNUSED(uiLength);

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
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedFloat(szTmp, uiLength - 2, uiWritePosition, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  szTmp[uiWritePosition + 0] = /*(char)0xC2;*/ -62;
  szTmp[uiWritePosition + 1] = /*(char)0xB0;*/ -80;
  szTmp[uiWritePosition + 2] = '\0';

  return xiiStringView(szTmp, szTmp + uiWritePosition + 2);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiAngleTemplate<double>& arg)
{
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedFloat(szTmp, uiLength - 2, uiWritePosition, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  szTmp[uiWritePosition + 0] = /*(char)0xC2;*/ -62;
  szTmp[uiWritePosition + 1] = /*(char)0xB0;*/ -80;
  szTmp[uiWritePosition + 2] = '\0';

  return xiiStringView(szTmp, szTmp + uiWritePosition + 2);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiRational& arg)
{
  xiiUInt32 uiWritePosition = 0;

  if (arg.IsIntegral())
  {
    xiiStringUtils::OutputFormattedInt(szTmp, uiLength, uiWritePosition, arg.GetIntegralResult(), 1, false, 10);

    return xiiStringView(szTmp, szTmp + uiWritePosition);
  }
  else
  {
    xiiStringUtils::snprintf(szTmp, uiLength, "%i/%i", arg.GetNumerator(), arg.GetDenominator());

    return xiiStringView(szTmp);
  }
}

xiiStringView BuildString(char* pTmp, xiiUInt32 uiLength, const xiiTime& arg)
{
  xiiUInt32 uiWritePosition = 0;

  const double fAbsSec = xiiMath::Abs(arg.GetSeconds());

  if (fAbsSec < 0.000001)
  {
    xiiStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, uiWritePosition, arg.GetNanoseconds(), 1, false, 1, false, true);
    // szTmp[uiWritePosition++] = ' ';
    pTmp[uiWritePosition++] = 'n';
    pTmp[uiWritePosition++] = 's';
  }
  else if (fAbsSec < 0.001)
  {
    xiiStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, uiWritePosition, arg.GetMicroseconds(), 1, false, 1, false, true);

    // szTmp[uiWritePosition++] = ' ';
    // Utf-8 representation of the microsecond (us) sign
    pTmp[uiWritePosition++] = /*(char)0xC2;*/ -62;
    pTmp[uiWritePosition++] = /*(char)0xB5;*/ -75;
    pTmp[uiWritePosition++] = 's';
  }
  else if (fAbsSec < 1.0)
  {
    xiiStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, uiWritePosition, arg.GetMilliseconds(), 1, false, 1, false, true);

    // tmp[uiWritePosition++] = ' ';
    pTmp[uiWritePosition++] = 'm';
    pTmp[uiWritePosition++] = 's';
  }
  else if (fAbsSec < 60.0)
  {
    xiiStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, uiWritePosition, arg.GetSeconds(), 1, false, 1, false, true);

    // szTmp[uiWritePosition++] = ' ';
    pTmp[uiWritePosition++] = 's';
    pTmp[uiWritePosition++] = 'e';
    pTmp[uiWritePosition++] = 'c';
  }
  else if (fAbsSec < 60.0 * 60.0)
  {
    double tRem = fAbsSec;

    xiiInt32 iMin = static_cast<xiiInt32>(xiiMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;
    iMin *= xiiMath::Sign(static_cast<xiiInt32>(arg.GetSeconds()));

    const xiiInt32 iSec = static_cast<xiiInt32>(xiiMath::Trunc(tRem));

    uiWritePosition = xiiStringUtils::snprintf(pTmp, uiLength, "%imin %isec", iMin, iSec);
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

    uiWritePosition = xiiStringUtils::snprintf(pTmp, uiLength, "%ih %imin %isec", iHrs, iMin, iSec);
  }

  pTmp[uiWritePosition] = '\0';
  return xiiStringView(pTmp, pTmp + uiWritePosition);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgHumanReadable& arg)
{
  xiiUInt32 uiSuffixIndex = 0;
  xiiUInt64 uiDivider     = 1;
  double    fAbsValue     = xiiMath::Abs(arg.m_Value);
  while (fAbsValue / uiDivider >= arg.m_Base && uiSuffixIndex < arg.m_SuffixCount - 1)
  {
    uiDivider *= arg.m_Base;
    ++uiSuffixIndex;
  }

  xiiUInt32 uiWritePosition = 0;
  if (uiDivider == 1 && xiiMath::Fraction(arg.m_Value) == 0.0)
  {
    xiiStringUtils::OutputFormattedInt(szTmp, uiLength, uiWritePosition, static_cast<xiiInt64>(arg.m_Value), 1, false, 10);
  }
  else
  {
    xiiStringUtils::OutputFormattedFloat(szTmp, uiLength, uiWritePosition, arg.m_Value / uiDivider, 1, false, 2, false);
  }
  xiiStringUtils::Copy(szTmp + uiWritePosition, uiLength - uiWritePosition, arg.m_Suffixes[uiSuffixIndex]);

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

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiSizeU32& arg)
{
  xiiUInt32 uiWritePosition = 0;
  xiiStringUtils::OutputFormattedInt(szTmp, uiLength, uiWritePosition, arg.width, 1, false, 10);

  szTmp[uiWritePosition++] = 'x';
  xiiStringUtils::OutputFormattedInt(szTmp, uiLength, uiWritePosition, arg.height, 1, false, 10);

  szTmp[uiWritePosition] = '\0';
  return xiiStringView(szTmp, szTmp + uiWritePosition);
}

xiiStringView xiiArgSensitive::BuildString_SensitiveUserData_Hash(char* szTmp, xiiUInt32 uiLength, const xiiArgSensitive& arg)
{
  const xiiUInt32 len = arg.m_sSensitiveInfo.GetElementCount();

  if (len == 0)
    return xiiStringView();

  if (!xiiStringUtils::IsNullOrEmpty(arg.m_szContext))
  {
    xiiStringUtils::snprintf(szTmp, uiLength, "sud:%s#%08x($%u)", arg.m_szContext, xiiHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }
  else
  {
    xiiStringUtils::snprintf(szTmp, uiLength, "sud:#%08x($%u)", xiiHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }

  return szTmp;
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgEnum& arg)
{
  xiiStringBuilder sTemp;
  const auto       mode = arg.m_bFullyQualifiedName ? xiiReflectionUtils::EnumConversionMode::FullyQualifiedName : xiiReflectionUtils::EnumConversionMode::ValueNameOnly;
  xiiReflectionUtils::EnumerationToString(arg.m_pType, arg.m_iValue, sTemp, mode);
  xiiStringUtils::Copy(szTmp, uiLength, sTemp.GetData());
  return xiiStringView(szTmp);
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgErrorCode& arg)
{
  LPVOID lpMsgBuf = nullptr;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, arg.m_ErrorCode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&lpMsgBuf, 0, nullptr) == 0)
  {
    DWORD err = GetLastError();
    xiiStringUtils::snprintf(szTmp, uiLength, "%u (FormatMessageW failed with error code %u)", arg.m_ErrorCode, err);
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

  xiiStringUtils::snprintf(FullMessage, XII_ARRAY_SIZE(FullMessage), "%u (\"%s\")", arg.m_ErrorCode, xiiStringUtf8((LPWSTR)lpMsgBuf).GetData());
  LocalFree(lpMsgBuf);
  return xiiStringView(FullMessage);
}
#endif

#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <string.h>

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgErrno& arg)
{
  const char* szErrorMsg = std::strerror(arg.m_iErrno);
  xiiStringUtils::snprintf(szTmp, uiLength, "%i (\"%s\")", arg.m_iErrno, szErrorMsg);
  return xiiStringView(szTmp);
}

xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgErrorCode& arg)
{
  xiiStringUtils::snprintf(szTmp, uiLength, "%u", arg.m_ErrorCode);
  return xiiStringView(szTmp);
}
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_FormatString);
