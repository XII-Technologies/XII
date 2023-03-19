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

void xiiFormatString::SBAppendView(xiiStringBuilder& sb, const xiiStringView& sub)
{
  sb.Append(sub);
}

void xiiFormatString::SBClear(xiiStringBuilder& sb)
{
  sb.Clear();
}

void xiiFormatString::SBAppendChar(xiiStringBuilder& sb, xiiUInt32 uiChar)
{
  sb.Append(uiChar);
}

const char* xiiFormatString::SBReturn(xiiStringBuilder& sb)
{
  return sb.GetData();
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgI& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedInt(tmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase);
  tmp[writepos] = '\0';
  return xiiStringView(tmp, tmp + writepos);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiInt64 arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedInt(tmp, uiLength, writepos, arg, 1, false, 10);
  tmp[writepos] = '\0';
  return xiiStringView(tmp, tmp + writepos);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiInt32 arg)
{
  return BuildString(tmp, uiLength, (xiiInt64)arg);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgU& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedUInt(tmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase, arg.m_bUpperCase);
  tmp[writepos] = '\0';
  return xiiStringView(tmp, tmp + writepos);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiUInt64 arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedUInt(tmp, uiLength, writepos, arg, 1, false, 10, false);
  tmp[writepos] = '\0';
  return xiiStringView(tmp, tmp + writepos);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiUInt32 arg)
{
  return BuildString(tmp, uiLength, (xiiUInt64)arg);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgF& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedFloat(tmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_iPrecision, arg.m_bScientific);
  tmp[writepos] = '\0';
  return xiiStringView(tmp, tmp + writepos);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, double arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedFloat(tmp, uiLength, writepos, arg, 1, false, -1, false);
  tmp[writepos] = '\0';
  return xiiStringView(tmp, tmp + writepos);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, bool arg)
{
  if (arg)
    return "true";

  return "false";
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const char* arg)
{
  return arg;
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const wchar_t* arg)
{
  const char* start = tmp;
  if (arg != nullptr)
  {
    // Code points in UTF-8 can be up to 4 byte, so the end pointer is 3 byte "earlier" than for
    // for a single byte character. One byte for trailing zero is already accounted for in uiLength.
    const char* tmpEnd = tmp + uiLength - 3u;
    while (*arg != '\0' && tmp < tmpEnd)
    {
      // decode utf8 to utf32
      const xiiUInt32 uiUtf32 = xiiUnicodeUtils::DecodeWCharToUtf32(arg);

      // encode utf32 to wchar_t
      xiiUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tmp);
    }
  }

  // Append terminator. As the extra byte for trailing zero is accounted for in uiLength, this is safe.
  *tmp = '\0';

  return start;
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiString& arg)
{
  return xiiStringView(arg.GetData(), arg.GetData() + arg.GetElementCount());
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiHashedString& arg)
{
  return xiiStringView(arg.GetData(), arg.GetData() + arg.GetString().GetElementCount());
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiStringBuilder& arg)
{
  return xiiStringView(arg.GetData(), arg.GetData() + arg.GetElementCount());
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiUntrackedString& arg)
{
  return xiiStringView(arg.GetData(), arg.GetData() + arg.GetElementCount());
}

const xiiStringView& BuildString(char* tmp, xiiUInt32 uiLength, const xiiStringView& arg)
{
  return arg;
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgC& arg)
{
  tmp[0] = arg.m_Value;
  tmp[1] = '\0';

  return xiiStringView(&tmp[0], &tmp[1]);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgP& arg)
{
  xiiStringUtils::snprintf(tmp, uiLength, "%p", arg.m_Value);
  return xiiStringView(tmp);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiResult arg)
{
  if (arg.Failed())
    return "<failed>";
  else
    return "<succeeded>";
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiVariant& arg)
{
  xiiString sString = arg.ConvertTo<xiiString>();
  xiiStringUtils::snprintf(tmp, uiLength, "%s", sString.GetData());
  return xiiStringView(tmp);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiAngleTemplate<float>& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedFloat(tmp, uiLength - 2, writepos, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  tmp[writepos + 0] = (char)0xC2;
  tmp[writepos + 1] = (char)0xB0;
  tmp[writepos + 2] = '\0';

  return xiiStringView(tmp, tmp + writepos + 2);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiAngleTemplate<double>& arg)
{
  xiiUInt32 writepos = 0;
  xiiStringUtils::OutputFormattedFloat(tmp, uiLength - 2, writepos, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  tmp[writepos + 0] = (char)0xC2;
  tmp[writepos + 1] = (char)0xB0;
  tmp[writepos + 2] = '\0';

  return xiiStringView(tmp, tmp + writepos + 2);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiRational& arg)
{
  xiiUInt32 writepos = 0;

  if (arg.IsIntegral())
  {
    xiiStringUtils::OutputFormattedInt(tmp, uiLength, writepos, arg.GetIntegralResult(), 1, false, 10);

    return xiiStringView(tmp, tmp + writepos);
  }
  else
  {
    xiiStringUtils::snprintf(tmp, uiLength, "%i/%i", arg.GetNumerator(), arg.GetDenominator());

    return xiiStringView(tmp);
  }
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiTime& arg)
{
  xiiUInt32 writepos = 0;

  const double fAbsSec = xiiMath::Abs(arg.GetSeconds());

  if (fAbsSec < 0.000001)
  {
    xiiStringUtils::OutputFormattedFloat(tmp, uiLength - 5, writepos, arg.GetNanoseconds(), 1, false, 1, false, true);
    // tmp[writepos++] = ' ';
    tmp[writepos++] = 'n';
    tmp[writepos++] = 's';
  }
  else if (fAbsSec < 0.001)
  {
    xiiStringUtils::OutputFormattedFloat(tmp, uiLength - 5, writepos, arg.GetMicroseconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    // Utf-8 representation of the microsecond (us) sign
    tmp[writepos++] = (char)0xC2;
    tmp[writepos++] = (char)0xB5;
    tmp[writepos++] = 's';
  }
  else if (fAbsSec < 1.0)
  {
    xiiStringUtils::OutputFormattedFloat(tmp, uiLength - 5, writepos, arg.GetMilliseconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    tmp[writepos++] = 'm';
    tmp[writepos++] = 's';
  }
  else if (fAbsSec < 60.0)
  {
    xiiStringUtils::OutputFormattedFloat(tmp, uiLength - 5, writepos, arg.GetSeconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    tmp[writepos++] = 's';
    tmp[writepos++] = 'e';
    tmp[writepos++] = 'c';
  }
  else if (fAbsSec < 60.0 * 60.0)
  {
    double tRem = fAbsSec;

    xiiInt32 iMin = static_cast<xiiInt32>(xiiMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;
    iMin *= xiiMath::Sign(static_cast<xiiInt32>(arg.GetSeconds()));

    const xiiInt32 iSec = static_cast<xiiInt32>(xiiMath::Trunc(tRem));

    writepos = xiiStringUtils::snprintf(tmp, uiLength, "%imin %isec", iMin, iSec);
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

    writepos = xiiStringUtils::snprintf(tmp, uiLength, "%ih %imin %isec", iHrs, iMin, iSec);
  }

  tmp[writepos] = '\0';
  return xiiStringView(tmp, tmp + writepos);
}

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgHumanReadable& arg)
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
    xiiStringUtils::OutputFormattedInt(tmp, uiLength, writepos, static_cast<xiiInt64>(arg.m_Value), 1, false, 10);
  }
  else
  {
    xiiStringUtils::OutputFormattedFloat(tmp, uiLength, writepos, arg.m_Value / divider, 1, false, 2, false);
  }
  xiiStringUtils::Copy(tmp + writepos, uiLength - writepos, arg.m_Suffixes[suffixIndex]);

  return xiiStringView(tmp);
}

xiiArgSensitive::BuildStringCallback xiiArgSensitive::s_BuildStringCB = nullptr;

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgSensitive& arg)
{
  if (xiiArgSensitive::s_BuildStringCB)
  {
    return xiiArgSensitive::s_BuildStringCB(tmp, uiLength, arg);
  }

  return arg.m_sSensitiveInfo;
}

xiiStringView xiiArgSensitive::BuildString_SensitiveUserData_Hash(char* tmp, xiiUInt32 uiLength, const xiiArgSensitive& arg)
{
  const xiiUInt32 len = arg.m_sSensitiveInfo.GetElementCount();

  if (len == 0)
    return xiiStringView();

  if (!xiiStringUtils::IsNullOrEmpty(arg.m_szContext))
  {
    xiiStringUtils::snprintf(
      tmp, uiLength, "sud:%s#%08x($%u)", arg.m_szContext, xiiHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }
  else
  {
    xiiStringUtils::snprintf(tmp, uiLength, "sud:#%08x($%u)", xiiHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }

  return tmp;
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgErrorCode& arg)
{
  LPVOID lpMsgBuf = nullptr;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, arg.m_ErrorCode,
                     MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&lpMsgBuf, 0, nullptr) == 0)
  {
    DWORD err = GetLastError();
    xiiStringUtils::snprintf(tmp, uiLength, "%i (FormatMessageW failed with error code %i)", arg.m_ErrorCode, err);
    return xiiStringView(tmp);
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
