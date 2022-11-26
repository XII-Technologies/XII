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

  XII_ALWAYS_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  XII_ALWAYS_INLINE bool Failed() const { return m_Result.Failed(); }
  void                   LogFailure(xiiLogInterface* pLog = nullptr);

  xiiResult m_Result;
  xiiString m_sMessage;
};

XII_ALWAYS_INLINE xiiResult xiiToResult(const xiiStatus& result)
{
  return result.m_Result;
}
