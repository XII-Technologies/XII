#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_TweenProperty.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>

namespace
{
  constexpr bool CanInterpolate(xiiVariantType::Enum variantType)
  {
    return (variantType >= xiiVariantType::Int8 && variantType <= xiiVariantType::Vector4) || variantType == xiiVariantType::Quaternion;
  }

  struct LerpFunc
  {
    template <typename T>
    XII_ALWAYS_INLINE void operator()(const xiiVariant& a, const xiiVariant& b, float x, xiiVariant& out_res)
    {
      if constexpr (std::is_same_v<T, xiiQuat>)
      {
        xiiQuat q;
        q.SetSlerp(a.Get<xiiQuat>(), b.Get<xiiQuat>(), x);
        out_res = q;
      }
      else if constexpr (CanInterpolate(static_cast<xiiVariantType::Enum>(xiiVariantTypeDeduction<T>::value)))
      {
        out_res = xiiMath::Lerp(a.Get<T>(), b.Get<T>(), x);
      }
    }
  };

} // namespace

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

void xiiScriptCoroutine_TweenProperty::Start(xiiComponentHandle hComponent, xiiStringView sPropertyName, xiiVariant targetValue, xiiTime duration, xiiEnum<xiiEasingFunction> easingFunction)
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
  if (variantType == xiiVariantType::Invalid || CanInterpolate(variantType) == false)
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

  m_pProperty      = static_cast<xiiAbstractMemberProperty*>(pProp);
  m_hComponent     = hComponent;
  m_SourceValue    = xiiReflectionUtils::GetMemberPropertyValue(m_pProperty, pComponent);
  m_EasingFunction = easingFunction;

  m_Duration   = duration;
  m_TimePassed = xiiTime::Zero();
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

    const double fDuration = m_Duration.GetSeconds();
    double       fCurrentX = xiiMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX              = xiiEasingFunction::GetValue(m_EasingFunction, fCurrentX);

    LerpFunc   func;
    xiiVariant currentValue;
    xiiVariant::DispatchTo(func, m_TargetValue.GetType(), m_SourceValue, m_TargetValue, static_cast<float>(fCurrentX), currentValue);

    xiiReflectionUtils::SetMemberPropertyValue(m_pProperty, pComponent, currentValue);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}
