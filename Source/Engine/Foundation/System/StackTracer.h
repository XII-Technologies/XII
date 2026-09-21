/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Types/Delegate.h>

/// Helper class to capture the current stack and print a captured stack
class XII_FOUNDATION_DLL xiiStackTracer
{
public:
  /// Captures the current stack trace.
  ///
  /// The trace will contain not more than ref_trace.GetCount() entries.
  /// [Windows] If called in an exception handler, set pContext to PEXCEPTION_POINTERS::ContextRecord.
  /// Returns the actual number of captured entries.
  static xiiUInt32 GetStackTrace(xiiArrayPtr<void*>& ref_trace, void* pContext = nullptr);

  /// Callback-function to print a text somewhere
  using PrintFunc = xiiDelegate<void(const char* szText)>;

  /// Print a stack trace
  static void ResolveStackTrace(const xiiArrayPtr<void*>& trace, PrintFunc printFunc);

  /// Print a stack trace without resolving it
  static void PrintStackTrace(const xiiArrayPtr<void*>& trace, PrintFunc printFunc);

private:
  xiiStackTracer() = delete;

  static void OnPluginEvent(const xiiPluginEvent& e);

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, StackTracer);
};
