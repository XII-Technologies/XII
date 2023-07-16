#pragma once

#include <Core/Scripting/ScriptCoroutine.h>

class XII_CORE_DLL xiiScriptCoroutine_Wait : public xiiTypedScriptCoroutine<xiiScriptCoroutine_Wait, xiiTime>
{
public:
  void           Start(xiiTime timeout);
  virtual Result Update(xiiTime deltaTimeSinceLastUpdate) override;

private:
  xiiTime m_TimeRemaing;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptCoroutine_Wait);
