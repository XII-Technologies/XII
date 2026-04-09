#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Easing.h>

/// Script coroutine that smoothly moves a game object to a target position over time.
///
/// Provides interpolated movement with configurable easing curves for animation effects.
/// The object's position is updated each frame until the target is reached or the duration expires.
class XII_CORE_DLL xiiScriptCoroutine_MoveTo : public xiiTypedScriptCoroutine<xiiScriptCoroutine_MoveTo, xiiGameObjectHandle, xiiVec3, xiiTime, xiiEnum<xiiEasingFunction>>
{
public:
  /// Initiates the move operation to the specified target position.
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
