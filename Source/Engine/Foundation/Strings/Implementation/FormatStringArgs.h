/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifndef XII_INCLUDING_BASICS_H
#  error "FormatStringArgs.h must not be included directly, but instead include Foundation/Basics.h."
#endif

class xiiStringBuilder;
class xiiVariant;
template <typename Type>
class xiiAngleTemplate;
class xiiRational;
struct xiiTime;

template <typename T>
struct xiiEnum;
template <typename T>
struct xiiBitflags;

template <typename T>
const xiiRTTI* xiiGetStaticRTTI();

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


/// Formats a given number such that it will be in format [0, base){suffix} with suffix
/// representing a power of base. Resulting numbers are output with a precision of 2 fractional digits
/// and fractional digits are subject to rounding, so numbers at the upper boundary of [0, base)
/// may be rounded up to the next power of base.
///
/// E.g.: For the default case base is 1000 and suffixes are the SI unit suffixes (i.e. K for kilo, M for mega etc.)
///       Thus 0 remains 0, 1 remains 1, 1000 becomes 1.00K, and 2534000 becomes 2.53M. But 999.999 will
///       end up being displayed as 1000.00K for base 1000 due to rounding.
struct xiiArgHumanReadable
{
  inline xiiArgHumanReadable(const double value, const xiiUInt64 uiBase, const char* const* const pSuffixes, xiiUInt32 uiSuffixCount) :
    m_Value(value), m_Base(uiBase), m_Suffixes(pSuffixes), m_SuffixCount(uiSuffixCount)
  {
  }

  inline xiiArgHumanReadable(const xiiInt64 value, const xiiUInt64 uiBase, const char* const* const pSuffixes, xiiUInt32 uiSuffixCount) :
    xiiArgHumanReadable(static_cast<double>(value), uiBase, pSuffixes, uiSuffixCount)
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
/// Converts a windows HRESULT into an error code and a human-readable error message.
/// Pass in `GetLastError()` function or an HRESULT from another error source. Be careful when printing multiple values, a function could clear `GetLastError` as a side-effect so it is best to store it in a temp variable before printing a complex error message.
/// \sa https://learn.microsoft.com/en-gb/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
struct xiiArgErrorCode
{
  inline explicit xiiArgErrorCode(xiiUInt32 uiErrorCode) :
    m_ErrorCode(uiErrorCode)
  {
  }

  xiiUInt32 m_ErrorCode;
};
XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgErrorCode& arg);

#endif

#if XII_ENABLED(XII_PLATFORM_LINUX)
/// Many Linux APIs will fill out error on failure. This converts the error into an error code and a human-readable error message.
/// Pass in the linux `errno` symbol. Be careful when printing multiple values, a function could clear `errno` as a side-effect so it is best to store it in a temp variable before printing a complex error message.
/// You may have to include #include <errno.h> use this.
/// \sa https://man7.org/linux/man-pages/man3/errno.3.html
struct xiiArgErrno
{
  inline explicit xiiArgErrno(xiiInt32 iErrno) :
    m_iErrno(iErrno)
  {
  }

  xiiInt32 m_iErrno;
};
XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgErrno& arg);

struct xiiArgErrorCode
{
  inline explicit xiiArgErrorCode(xiiUInt32 uiErrorCode) :
    m_ErrorCode(uiErrorCode)
  {
  }

  xiiUInt32 m_ErrorCode;
};

XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgErrorCode& arg);
#endif

/// Wraps a string that may contain sensitive information, such as user file paths.
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

  /// Set s_BuildStringCB to this function to enable scrambling of sensitive data.
  XII_FOUNDATION_DLL static xiiStringView BuildString_SensitiveUserData_Hash(char* szTmp, xiiUInt32 uiLength, const xiiArgSensitive& arg);
};

/// Formats an xiiEnum or xiiBitflags value as its string representation using the reflection system.
///
/// By default the value name is output without the type prefix (e.g. "Value1" instead of "MyEnum::Value1"). Set bFullyQualifiedName to true to include the type prefix.
/// Requires that the enum/bitflags type has been registered with the reflection system via XII_BEGIN_STATIC_REFLECTED_ENUM / XII_BEGIN_STATIC_REFLECTED_BITFLAGS.
struct xiiArgEnum
{
  template <typename T>
  inline explicit xiiArgEnum(xiiEnum<T> value, bool bFullyQualifiedName = false) :
    m_pType(xiiGetStaticRTTI<T>()), m_iValue(static_cast<xiiInt64>(value.GetValue())), m_bFullyQualifiedName(bFullyQualifiedName)
  {
  }

  template <typename T>
  inline explicit xiiArgEnum(xiiBitflags<T> value, bool bFullyQualifiedName = false) :
    m_pType(xiiGetStaticRTTI<T>()), m_iValue(static_cast<xiiInt64>(value.GetValue())), m_bFullyQualifiedName(bFullyQualifiedName)
  {
  }

  const xiiRTTI* m_pType               = nullptr;
  xiiInt64       m_iValue              = 0;
  bool           m_bFullyQualifiedName = false;
};

XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgI& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, xiiInt64 iArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, xiiInt32 iArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgU& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, xiiUInt64 uiArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, xiiUInt32 uiArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgF& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, double fArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, bool bArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const char* szArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const wchar_t* pArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiStringBuilder& sArg);
XII_FOUNDATION_DLL const xiiStringView& BuildString(char* szTmp, xiiUInt32 uiLength, const xiiStringView& sArg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgC& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgP& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, xiiResult arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiVariant& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiAngleTemplate<float>& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiAngleTemplate<double>& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiRational& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgHumanReadable& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiTime& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgSensitive& arg);
XII_FOUNDATION_DLL xiiStringView        BuildString(char* szTmp, xiiUInt32 uiLength, const xiiArgEnum& arg);

#if XII_ENABLED(XII_COMPILER_GCC) || XII_ENABLED(XII_COMPILER_CLANG)

// on these platforms "long int" is a different type from "long long int"

XII_ALWAYS_INLINE xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, long int iArg)
{
  return BuildString(szTmp, uiLength, static_cast<xiiInt64>(iArg));
}

XII_ALWAYS_INLINE xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, unsigned long int uiArg)
{
  return BuildString(szTmp, uiLength, static_cast<xiiUInt64>(uiArg));
}

#endif
