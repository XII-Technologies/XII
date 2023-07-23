#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_MoveTo.h>
#include <Core/World/World.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptCoroutine_MoveTo, xiiScriptCoroutine, 1, xiiRTTIDefaultAllocator<xiiScriptCoroutine_MoveTo>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(Start, In, "Object", In, "TargetPos", In, "Duration", In, "Easing"),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Coroutine::MoveTo {TargetPos}"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiScriptCoroutine_MoveTo::Start(xiiGameObjectHandle hObject, const xiiVec3& vTargetPos, xiiTime duration, xiiEnum<xiiEasingFunction> easing)
{
  xiiGameObject* pObject = nullptr;
  if (xiiWorld::GetWorld(hObject)->TryGetObject(hObject, pObject) == false)
  {
    xiiLog::Error("MoveTo: The given game object was not found.");
    return;
  }

  m_hObject        = hObject;
  m_vSourcePos     = pObject->GetLocalPosition();
  m_vTargetPos     = vTargetPos;
  m_EasingFunction = easing;

  m_Duration   = duration;
  m_TimePassed = xiiTime::Zero();
}

xiiScriptCoroutine::Result xiiScriptCoroutine_MoveTo::Update(xiiTime deltaTimeSinceLastUpdate)
{
  if (deltaTimeSinceLastUpdate.IsPositive())
  {
    xiiGameObject* pObject = nullptr;
    if (xiiWorld::GetWorld(m_hObject)->TryGetObject(m_hObject, pObject) == false)
    {
      return Result::Failed();
    }

    m_TimePassed += deltaTimeSinceLastUpdate;

    const double fDuration = m_Duration.GetSeconds();
    double       fCurrentX = xiiMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX              = xiiEasingFunction::GetValue(m_EasingFunction, fCurrentX);

    xiiVec3 vCurrentPos = xiiMath::Lerp(m_vSourcePos, m_vTargetPos, static_cast<float>(fCurrentX));
    pObject->SetLocalPosition(vCurrentPos);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}
