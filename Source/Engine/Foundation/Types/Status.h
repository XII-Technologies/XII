#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class xiiLogInterface;

/// \brief An xiiResult with an additional message for the reason of failure
struct XII_FOUNDATION_DLL xiiStatus
{
  XII_ALWAYS_INLINE explicit xiiStatus() :
    m_Result(XII_FAILURE)
  {
  }

  // This const char* version is needed for disambiguation.
  explicit xiiStatus(const char* szError) :
    m_Result(XII_FAILURE), m_sMessage(szError)
  {
  }

  explicit xiiStatus(xiiResult r, xiiStringView sError) :
    m_Result(r), m_sMessage(sError)
  {
  }

  explicit xiiStatus(xiiStringView sError) :
    m_Result(XII_FAILURE), m_sMessage(sError)
  {
  }

  XII_ALWAYS_INLINE xiiStatus(xiiResult r) :
    m_Result(r)
  {
  }

  explicit xiiStatus(const xiiFormatString& fmt);

  [[nodiscard]] XII_ALWAYS_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  [[nodiscard]] XII_ALWAYS_INLINE bool Failed() const { return m_Result.Failed(); }

  /// \brief Same as 'Succeeded()'.
  ///
  /// Allows xiiStatus to be used in if statements:
  ///  - if (r)
  ///  - if (!r)
  ///  - if (r1 && r2)
  ///  - if (r1 || r2)
  ///
  /// Disallows anything else implicitly, e.g. all these won't compile:
  ///   - if (r == true)
  ///   - bool b = r;
  ///   - void* p = r;
  ///   - return r; // with bool return type
  explicit operator bool() const { return m_Result.Succeeded(); }

  /// \brief Special case to prevent this from working: "bool b = !r"
  xiiResult operator!() const { return xiiResult(m_Result.Succeeded() ? XII_FAILURE : XII_SUCCESS); }

  /// \brief If the state is XII_FAILURE, the message is written to the given log (or the currently active thread-local log).
  void LogFailure(xiiLogInterface* pLog = nullptr);

  xiiResult m_Result;
  xiiString m_sMessage;
};

XII_ALWAYS_INLINE xiiResult xiiToResult(const xiiStatus& result)
{
  return result.m_Result;
}
