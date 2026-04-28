/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Easing.h>

/// Script coroutine that animates a component property value over time.
///
/// Provides smooth interpolation between the current and target property values using configurable easing curves.
/// Supports any property type that can be represented as a variant and interpolated.
class XII_CORE_DLL xiiScriptCoroutine_TweenProperty : public xiiTypedScriptCoroutine<xiiScriptCoroutine_TweenProperty, xiiComponentHandle, xiiStringView, xiiVariant, xiiTime, xiiEnum<xiiEasingFunction>>
{
public:
  /// Initiates the property animation to the specified target value.
  void           Start(xiiComponentHandle hComponent, xiiStringView sPropertyName, xiiVariant targetValue, xiiTime duration, xiiEnum<xiiEasingFunction> easing);
  virtual Result Update(xiiTime deltaTimeSinceLastUpdate) override;

private:
  const xiiAbstractMemberProperty* m_pProperty = nullptr;
  xiiComponentHandle               m_hComponent;
  xiiVariant                       m_SourceValue;
  xiiVariant                       m_TargetValue;
  xiiEnum<xiiEasingFunction>       m_Easing;

  xiiTime m_Duration;
  xiiTime m_TimePassed;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptCoroutine_TweenProperty);
