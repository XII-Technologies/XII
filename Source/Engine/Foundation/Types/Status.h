#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class xiiLogInterface;

/// \brief An xiiResult with an additional message for the reason of failure
struct [[nodiscard]] XII_FOUNDATION_DLL xiiStatus
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

  /// \brief Used to silence compiler warnings, when success or failure doesn't matter.
  XII_ALWAYS_INLINE void IgnoreResult()
  {
    /* To be called when a return value is [[nodiscard]] but the result is not needed. */
  }

  /// \brief If the state is XII_FAILURE, the message is written to the given log (or the currently active thread-local log).
  ///
  /// The return value is the same as 'Failed()' but isn't marked as [[nodiscard]], ie returns true, if a failure happened.
  bool LogFailure(xiiLogInterface* pLog = nullptr);

  /// \brief Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a msg is given, this will be the assert message.
  /// Additionally m_sMessage will be included as a detailed message.
  void AssertSuccess(const char* szMsg = nullptr) const;

  xiiResult m_Result;
  xiiString m_sMessage;
};

XII_ALWAYS_INLINE xiiResult xiiToResult(const xiiStatus& result)
{
  return result.m_Result;
}
