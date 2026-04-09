#pragma once

#include <Core/Scripting/ScriptCoroutine.h>

/// Script coroutine that pauses execution for a specified duration.
///
/// Simple timing coroutine that delays script execution for a given time period.
/// Useful for creating delays in script sequences or implementing timed behaviors.
class XII_CORE_DLL xiiScriptCoroutine_Wait : public xiiTypedScriptCoroutine<xiiScriptCoroutine_Wait, xiiTime>
{
public:
  /// Initiates the wait period for the specified duration.
  void           Start(xiiTime timeout);
  virtual Result Update(xiiTime deltaTimeSinceLastUpdate) override;

private:
  xiiTime m_TimeRemaing;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptCoroutine_Wait);
