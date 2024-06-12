#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_TweenProperty.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptCoroutine_TweenProperty, xiiScriptCoroutine, 1, xiiRTTIDefaultAllocator<xiiScriptCoroutine_TweenProperty>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(Start, In, "Component", In, "PropertyName", In, "TargetValue", In, "Duration", In, "Easing"),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Coroutine::TweenProperty {PropertyName}"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiScriptCoroutine_TweenProperty::Start(xiiComponentHandle hComponent, xiiStringView sPropertyName, xiiVariant targetValue, xiiTime duration, xiiEnum<xiiEasingFunction> easing)
{
  xiiComponent* pComponent = nullptr;
  if (xiiWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent) == false)
  {
    xiiLog::Error("TweenProperty: The given component was not found.");
    return;
  }

  auto pType = pComponent->GetDynamicRTTI();
  auto pProp = pType->FindPropertyByName(sPropertyName);
  if (pProp == nullptr || pProp->GetCategory() != xiiPropertyCategory::Member)
  {
    xiiLog::Error("TweenProperty: The given component of type '{}' does not have a member property named '{}'.", pType->GetTypeName(), sPropertyName);
    return;
  }

  xiiVariantType::Enum variantType = pProp->GetSpecificType()->GetVariantType();
  if (variantType == xiiVariantType::Invalid)
  {
    xiiLog::Error("TweenProperty: Can't tween property '{}' of type '{}'.", sPropertyName, pProp->GetSpecificType()->GetTypeName());
    return;
  }

  xiiResult conversionStatus = XII_SUCCESS;
  m_TargetValue              = targetValue.ConvertTo(variantType, &conversionStatus);
  if (conversionStatus.Failed())
  {
    xiiLog::Error("TweenProperty: Can't convert given target value to '{}'.", pProp->GetSpecificType()->GetTypeName());
    return;
  }

  m_pProperty   = static_cast<const xiiAbstractMemberProperty*>(pProp);
  m_hComponent  = hComponent;
  m_SourceValue = xiiReflectionUtils::GetMemberPropertyValue(m_pProperty, pComponent);
  m_Easing      = easing;

  m_Duration   = duration;
  m_TimePassed = xiiTime::MakeZero();
}

xiiScriptCoroutine::Result xiiScriptCoroutine_TweenProperty::Update(xiiTime deltaTimeSinceLastUpdate)
{
  if (m_pProperty == nullptr)
  {
    return Result::Failed();
  }

  if (deltaTimeSinceLastUpdate.IsPositive())
  {
    xiiComponent* pComponent = nullptr;
    if (xiiWorld::GetWorld(m_hComponent)->TryGetComponent(m_hComponent, pComponent) == false)
    {
      return Result::Failed();
    }

    m_TimePassed += deltaTimeSinceLastUpdate;

    const double fDuration  = m_Duration.GetSeconds();
    double       fCurrentX  = xiiMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX               = xiiEasingFunction::GetValue(m_Easing, fCurrentX);
    xiiVariant currentValue = xiiMath::Lerp(m_SourceValue, m_TargetValue, fCurrentX);

    xiiReflectionUtils::SetMemberPropertyValue(m_pProperty, pComponent, currentValue);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}
