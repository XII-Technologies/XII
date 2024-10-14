#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Platform/Windows/MinWindows.h>
#include <Foundation/Strings/String.h>

/// Build string implementation for HRESULT.
XII_FOUNDATION_DLL xiiString xiiHRESULTtoString(xiiMinWindows::HRESULT result);

/// Conversion of HRESULT to xiiResult.
XII_ALWAYS_INLINE xiiResult xiiToResult(xiiMinWindows::HRESULT result)
{
  return result >= 0 ? XII_SUCCESS : XII_FAILURE;
}

#define XII_HRESULT_TO_FAILURE(code)   \
  do                                   \
  {                                    \
    xiiMinWindows::HRESULT s = (code); \
    if (s < 0)                         \
      return XII_FAILURE;              \
  } while (false)

#define XII_HRESULT_TO_FAILURE_LOG(code)                                                           \
  do                                                                                               \
  {                                                                                                \
    xiiMinWindows::HRESULT s = (code);                                                             \
    if (s < 0)                                                                                     \
    {                                                                                              \
      xiiLog::Error("Call '{0}' failed with: {1}", XII_PP_STRINGIFY(code), xiiHRESULTtoString(s)); \
      return XII_FAILURE;                                                                          \
    }                                                                                              \
  } while (false)

#define XII_HRESULT_TO_LOG(code)                                                                   \
  do                                                                                               \
  {                                                                                                \
    xiiMinWindows::HRESULT s = (code);                                                             \
    if (s < 0)                                                                                     \
    {                                                                                              \
      xiiLog::Error("Call '{0}' failed with: {1}", XII_PP_STRINGIFY(code), xiiHRESULTtoString(s)); \
    }                                                                                              \
  } while (false)

#define XII_NO_RETURNVALUE

#define XII_HRESULT_TO_LOG_RET(code, ret)                                                          \
  do                                                                                               \
  {                                                                                                \
    xiiMinWindows::HRESULT s = (code);                                                             \
    if (s < 0)                                                                                     \
    {                                                                                              \
      xiiLog::Error("Call '{0}' failed with: {1}", XII_PP_STRINGIFY(code), xiiHRESULTtoString(s)); \
      return ret;                                                                                  \
    }                                                                                              \
  } while (false)

#define XII_HRESULT_TO_ASSERT(code)                                                                       \
  do                                                                                                      \
  {                                                                                                       \
    xiiMinWindows::HRESULT s = (code);                                                                    \
    XII_ASSERT_DEV(s >= 0, "Call '{0}' failed with: {1}", XII_PP_STRINGIFY(code), xiiHRESULTtoString(s)); \
  } while (false)
