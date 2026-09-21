/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class xiiLogInterface;

/// A wrapper around xiiResult that includes an optional error message.
///
/// The xiiStatus structure represents a success or failure state.
/// If failure is indicated, an additional message provides context or details.
/// Intended to be returned from functions to communicate success/failure in a structured manner.
///
/// Usage example:
/// \code
/// xiiStatus status = SomeFunction();
/// if (status.Failed())
///   xiiLog::Error(status.GetMessageString());
/// \endcode
struct [[nodiscard]] XII_FOUNDATION_DLL xiiStatus
{
  /// \name Constructors
  /// @{

  /// Constructs a failure status with the given C-string error message.
  XII_ALWAYS_INLINE explicit xiiStatus(const char* szError) :
    m_Result(XII_FAILURE), m_sMessage(szError)
  {
  }

  /// Constructs a failure status with the given string view error message.
  XII_ALWAYS_INLINE explicit xiiStatus(xiiStringView sError) :
    m_Result(XII_FAILURE), m_sMessage(sError)
  {
  }

  /// Constructs a failure status from a formatted string.
  ///
  /// Useful for creating detailed messages with placeholders using xiiFmt().
  explicit xiiStatus(const xiiFormatString& fmt);

  /// Constructs a status with the given result (success or failure), without a message.
  XII_ALWAYS_INLINE xiiStatus(xiiResult result) :
    m_Result(result)
  {
  }

  /// Constructs a status with the given result enum, without a message.
  XII_ALWAYS_INLINE xiiStatus(xiiResultEnum result) :
    m_Result(result)
  {
  }

  /// @}

  /// \name Query Functions
  /// @{

  /// Returns the underlying xiiResult value.
  [[nodiscard]] XII_ALWAYS_INLINE xiiResult GetResult() const { return m_Result; }

  /// Returns true if the result indicates success.
  [[nodiscard]] XII_ALWAYS_INLINE bool Succeeded() const { return m_Result.Succeeded(); }

  /// Returns true if the result indicates failure.
  [[nodiscard]] XII_ALWAYS_INLINE bool Failed() const { return m_Result.Failed(); }

  /// Returns the stored error message string (may be empty).
  [[nodiscard]] XII_ALWAYS_INLINE const xiiString& GetMessageString() const { return m_sMessage; }

  /// @}

  /// \name Control and Logging
  /// @{

  /// Used to suppress [[nodiscard]] warnings when the result doesn't need handling.
  ///
  /// Call this if you're intentionally ignoring the result, e.g., inside a cleanup function.
  XII_ALWAYS_INLINE void IgnoreResult() {}

  /// Logs the error message if this represents a failure.
  ///
  /// Uses the provided log interface or falls back to the thread-local default.
  /// Returns true if this is a failure (same as Failed()), but not marked [[nodiscard]].
  bool LogFailure(xiiLogInterface* pLog = nullptr);

  /// Asserts that the status indicates success.
  ///
  /// If the assertion fails, the program will terminate.
  /// A custom message can be passed, and the internal error string is appended.
  void AssertSuccess(const char* szMsg = nullptr) const;

  /// @}

private:
  xiiResult m_Result;
  xiiString m_sMessage;
};

XII_ALWAYS_INLINE xiiResult xiiToResult(const xiiStatus& result)
{
  return result.GetResult();
}
