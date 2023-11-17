#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Easing.h>

class XII_CORE_DLL xiiScriptCoroutine_MoveTo : public xiiTypedScriptCoroutine<xiiScriptCoroutine_MoveTo, xiiGameObjectHandle, xiiVec3, xiiTime, xiiEnum<xiiEasingFunction>>
{
public:
  void           Start(xiiGameObjectHandle hObject, const xiiVec3& vTargetPos, xiiTime duration, xiiEnum<xiiEasingFunction> easing);
  virtual Result Update(xiiTime deltaTimeSinceLastUpdate) override;

private:
  xiiGameObjectHandle        m_hObject;
  xiiVec3                    m_vSourcePos;
  xiiVec3                    m_vTargetPos;
  xiiEnum<xiiEasingFunction> m_Easing;

  xiiTime m_Duration;
  xiiTime m_TimePassed;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptCoroutine_MoveTo);
