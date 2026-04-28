/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_Wait.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptCoroutine_Wait, xiiScriptCoroutine, 1, xiiRTTIDefaultAllocator<xiiScriptCoroutine_Wait>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(Start, In, "Timeout"),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Coroutine::Wait {Timeout}"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiScriptCoroutine_Wait::Start(xiiTime timeout)
{
  m_TimeRemaing = timeout;
}

xiiScriptCoroutine::Result xiiScriptCoroutine_Wait::Update(xiiTime deltaTimeSinceLastUpdate)
{
  m_TimeRemaing -= deltaTimeSinceLastUpdate;
  if (m_TimeRemaing.IsPositive())
  {
    // Don't wait for the full remaining time to prevent oversleeping due to scheduling precision.
    return Result::Running(m_TimeRemaing * 0.8);
  }

  return Result::Completed();
}

XII_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptCoroutine_Wait);
