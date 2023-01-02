#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringView.h>

class xiiStringBuilder;
class xiiVariant;
template <typename Type>
class xiiAngleTemplate;
class xiiRational;
struct xiiTime;

struct xiiArgI
{
  inline explicit xiiArgI(xiiInt64 value, xiiUInt8 uiWidth = 1, bool bPadWithZeros = false, xiiUInt8 uiBase = 10) :
    m_Value(value), m_uiWidth(uiWidth), m_bPadWithZeros(bPadWithZeros), m_uiBase(uiBase)
  {
  }

  xiiInt64 m_Value;
  xiiUInt8 m_uiWidth;
  bool     m_bPadWithZeros;
  xiiUInt8 m_uiBase;
};

struct xiiArgU
{
  inline explicit xiiArgU(xiiUInt64 value, xiiUInt8 uiWidth = 1, bool bPadWithZeros = false, xiiUInt8 uiBase = 10, bool bUpperCase = false) :
    m_Value(value), m_uiWidth(uiWidth), m_bPadWithZeros(bPadWithZeros), m_bUpperCase(bUpperCase), m_uiBase(uiBase)
  {
  }

  xiiUInt64 m_Value;
  xiiUInt8  m_uiWidth;
  bool      m_bPadWithZeros;
  bool      m_bUpperCase;
  xiiUInt8  m_uiBase;
};

struct xiiArgF
{
  inline explicit xiiArgF(double value, xiiInt8 iPrecision = -1, bool bScientific = false, xiiUInt8 uiWidth = 1, bool bPadWithZeros = false) :
    m_Value(value), m_uiWidth(uiWidth), m_bPadWithZeros(bPadWithZeros), m_bScientific(bScientific), m_iPrecision(iPrecision)
  {
  }

  double   m_Value;
  xiiUInt8 m_uiWidth;
  bool     m_bPadWithZeros;
  bool     m_bScientific;
  xiiInt8  m_iPrecision;
};

struct xiiArgC
{
  inline explicit xiiArgC(char value) :
    m_Value(value)
  {
  }

  char m_Value;
};

struct xiiArgP
{
  inline explicit xiiArgP(const void* value) :
    m_Value(value)
  {
  }

  const void* m_Value;
};


/// \brief Formats a given number such that it will be in format [0, base){suffix} with suffix
/// representing a power of base. Resulting numbers are output with a precision of 2 fractional digits
/// and fractional digits are subject to rounding, so numbers at the upper boundary of [0, base)
/// may be rounded up to the next power of base.
///
/// E.g.: For the default case base is 1000 and suffixes are the SI unit suffixes (i.e. K for kilo, M for mega etc.)
///       Thus 0 remains 0, 1 remains 1, 1000 becomes 1.00K, and 2534000 becomes 2.53M. But 999.999 will
///       end up being displayed as 1000.00K for base 1000 due to rounding.
struct xiiArgHumanReadable
{
  inline xiiArgHumanReadable(const double value, const xiiUInt64 base, const char* const* const suffixes, xiiUInt32 suffixCount) :
    m_Value(value), m_Base(base), m_Suffixes(suffixes), m_SuffixCount(suffixCount)
  {
  }

  inline xiiArgHumanReadable(const xiiInt64 value, const xiiUInt64 base, const char* const* const suffixes, xiiUInt32 suffixCount) :
    xiiArgHumanReadable(static_cast<double>(value), base, suffixes, suffixCount)
  {
  }

  inline explicit xiiArgHumanReadable(const double value) :
    xiiArgHumanReadable(value, 1000u, m_DefaultSuffixes, XII_ARRAY_SIZE(m_DefaultSuffixes))
  {
  }

  inline explicit xiiArgHumanReadable(const xiiInt64 value) :
    xiiArgHumanReadable(static_cast<double>(value), 1000u, m_DefaultSuffixes, XII_ARRAY_SIZE(m_DefaultSuffixes))
  {
  }

  const double             m_Value;
  const xiiUInt64          m_Base;
  const char* const* const m_Suffixes;
  const char* const        m_DefaultSuffixes[6] = {"", "K", "M", "G", "T", "P"};
  const xiiUInt32          m_SuffixCount;
};

struct xiiArgFileSize : public xiiArgHumanReadable
{
  inline explicit xiiArgFileSize(const xiiUInt64 value) :
    xiiArgHumanReadable(static_cast<double>(value), 1024u, m_ByteSuffixes, XII_ARRAY_SIZE(m_ByteSuffixes))
  {
  }

  const char* const m_ByteSuffixes[6] = {"B", "KB", "MB", "GB", "TB", "PB"};
};

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
struct xiiArgErrorCode
{
  inline explicit xiiArgErrorCode(xiiUInt32 errorCode) :
    m_ErrorCode(errorCode)
  {
  }

  xiiUInt32 m_ErrorCode;
};
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgErrorCode& arg);

#endif

/// \brief Wraps a string that may contain sensitive information, such as user file paths.
///
/// The application can specify a function to scramble this type of information. By default no such function is set.
/// A general purpose function is provided with 'BuildString_SensitiveUserData_Hash()'
///
/// \param sSensitiveInfo The information that may need to be scrambled.
/// \param szContext A custom string to identify the 'context', ie. what type of sensitive data is being scrambled.
///        This may be passed through unmodified, or can guide the scrambling function to choose how to output the sensitive data.
struct xiiArgSensitive
{
  inline explicit xiiArgSensitive(const xiiStringView& sSensitiveInfo, const char* szContext = nullptr) :
    m_sSensitiveInfo(sSensitiveInfo), m_szContext(szContext)
  {
  }

  const xiiStringView m_sSensitiveInfo;
  const char*         m_szContext;

  using BuildStringCallback = xiiStringView (*)(char*, xiiUInt32, const xiiArgSensitive&);
  XII_FOUNDATION_DLL static BuildStringCallback s_BuildStringCB;

  /// \brief Set s_BuildStringCB to this function to enable scrambling of sensitive data.
  XII_FOUNDATION_DLL static xiiStringView BuildString_SensitiveUserData_Hash(char* tmp, xiiUInt32 uiLength, const xiiArgSensitive& arg);
};

XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgI& arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiInt64 arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiInt32 arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgU& arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiUInt64 arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, xiiUInt32 arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgF& arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, double arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, bool arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const char* arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const wchar_t* arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiStringBuilder& arg);
XII_FOUNDATION_DLL const xiiStringView& BuildString(char* tmp, xiiUInt32 uiLength, const xiiStringView& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgC& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgP& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, xiiResult arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiVariant& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiAngleTemplate<float>& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiAngleTemplate<double>& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiRational& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgHumanReadable& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiTime& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* tmp, xiiUInt32 uiLength, const xiiArgSensitive& arg);


#if XII_ENABLED(XII_COMPILER_GCC) || XII_ENABLED(XII_COMPILER_CLANG)

// on these platforms "long int" is a different type from "long long int"

XII_ALWAYS_INLINE xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, long int arg)
{
  return BuildString(tmp, uiLength, static_cast<xiiInt64>(arg));
}

XII_ALWAYS_INLINE xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, unsigned long int arg)
{
  return BuildString(tmp, uiLength, static_cast<xiiUInt64>(arg));
}

#endif
