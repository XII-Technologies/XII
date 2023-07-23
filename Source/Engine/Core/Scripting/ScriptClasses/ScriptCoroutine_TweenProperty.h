#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Easing.h>

class XII_CORE_DLL xiiScriptCoroutine_TweenProperty : public xiiTypedScriptCoroutine<xiiScriptCoroutine_TweenProperty, xiiComponentHandle, xiiStringView, xiiVariant, xiiTime, xiiEnum<xiiEasingFunction>>
{
public:
  void           Start(xiiComponentHandle hComponent, xiiStringView sPropertyName, xiiVariant targetValue, xiiTime duration, xiiEnum<xiiEasingFunction> easingFunction);
  virtual Result Update(xiiTime deltaTimeSinceLastUpdate) override;

private:
  xiiAbstractMemberProperty* m_pProperty = nullptr;
  xiiComponentHandle         m_hComponent;
  xiiVariant                 m_SourceValue;
  xiiVariant                 m_TargetValue;
  xiiEnum<xiiEasingFunction> m_EasingFunction;

  xiiTime m_Duration;
  xiiTime m_TimePassed;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptCoroutine_TweenProperty);
