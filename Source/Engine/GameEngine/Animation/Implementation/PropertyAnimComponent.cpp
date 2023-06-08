#include <GameEngine/GameEnginePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/Animation/PropertyAnimComponent.h>

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_COMPONENT_TYPE(xiiPropertyAnimComponent, 3, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("Animation", GetPropertyAnimFile, SetPropertyAnimFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Property_Animation")),
    XII_MEMBER_PROPERTY("Playing", m_bPlaying)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiPropertyAnimMode, m_AnimationMode),
    XII_MEMBER_PROPERTY("RandomOffset", m_RandomOffset)->AddAttributes(new xiiClampValueAttribute(xiiTime::Seconds(0), xiiVariant())),
    XII_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(-10.0f, +10.0f)),
    XII_MEMBER_PROPERTY("RangeLow", m_AnimationRangeLow)->AddAttributes(new xiiClampValueAttribute(xiiTime(), xiiVariant())),
    XII_MEMBER_PROPERTY("RangeHigh", m_AnimationRangeHigh)->AddAttributes(new xiiClampValueAttribute(xiiTime(), xiiVariant()), new xiiDefaultValueAttribute(xiiTime::Seconds(60 * 60))),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES{
    new xiiCategoryAttribute("Animation"),
  } XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS{
    XII_MESSAGE_HANDLER(xiiMsgSetPlaying, OnMsgSetPlaying),
  } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_MESSAGESENDERS{
    XII_MESSAGE_SENDER(m_EventTrackMsgSender),
    XII_MESSAGE_SENDER(m_ReachedEndMsgSender),
  } XII_END_MESSAGESENDERS;
  XII_BEGIN_FUNCTIONS{XII_SCRIPT_FUNCTION_PROPERTY(PlayAnimationRange, In, "RangeLow", In, "RangeHigh")} XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiPropertyAnimComponent::xiiPropertyAnimComponent()
{
  m_AnimationRangeHigh = xiiTime::Seconds(60.0 * 60.0);
}

xiiPropertyAnimComponent::~xiiPropertyAnimComponent() = default;

void xiiPropertyAnimComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  s << m_hPropertyAnim;
  s << m_AnimationMode;
  s << m_RandomOffset;
  s << m_fSpeed;
  s << m_AnimationTime;
  s << m_bReverse;
  s << m_AnimationRangeLow;
  s << m_AnimationRangeHigh;

  s << m_bPlaying;

  /// \todo Somehow store the animation state (not necessary for new scenes, but for quicksaves)
}

void xiiPropertyAnimComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();

  s >> m_hPropertyAnim;

  if (uiVersion >= 2)
  {
    s >> m_AnimationMode;
    s >> m_RandomOffset;
    s >> m_fSpeed;
    s >> m_AnimationTime;
    s >> m_bReverse;
    s >> m_AnimationRangeLow;
    s >> m_AnimationRangeHigh;
  }

  if (uiVersion >= 3)
  {
    s >> m_bPlaying;
  }
}

void xiiPropertyAnimComponent::SetPropertyAnimFile(const char* szFile)
{
  xiiPropertyAnimResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiPropertyAnimResource>(szFile);
  }

  SetPropertyAnim(hResource);
}

const char* xiiPropertyAnimComponent::GetPropertyAnimFile() const
{
  if (!m_hPropertyAnim.IsValid())
    return "";

  return m_hPropertyAnim.GetResourceID();
}

void xiiPropertyAnimComponent::SetPropertyAnim(const xiiPropertyAnimResourceHandle& hPropertyAnim)
{
  m_hPropertyAnim = hPropertyAnim;
}

void xiiPropertyAnimComponent::PlayAnimationRange(xiiTime rangeLow, xiiTime rangeHigh)
{
  m_AnimationRangeLow  = rangeLow;
  m_AnimationRangeHigh = rangeHigh;

  m_bPlaying = true;

  StartPlayback();
}


void xiiPropertyAnimComponent::OnMsgSetPlaying(xiiMsgSetPlaying& ref_msg)
{
  m_bPlaying = ref_msg.m_bPlay;
}

void xiiPropertyAnimComponent::CreatePropertyBindings()
{
  m_ColorBindings.Clear();
  m_ComponentFloatBindings.Clear();
  m_GoFloatBindings.Clear();

  m_pAnimDesc = nullptr;

  if (!m_hPropertyAnim.IsValid())
    return;

  xiiResourceLock<xiiPropertyAnimResource> pAnimation(m_hPropertyAnim, xiiResourceAcquireMode::BlockTillLoaded);

  if (!pAnimation || pAnimation.GetAcquireResult() == xiiResourceAcquireResult::MissingFallback)
    return;

  m_pAnimDesc = pAnimation->GetDescriptor();

  for (const xiiFloatPropertyAnimEntry& anim : m_pAnimDesc->m_FloatAnimations)
  {
    xiiHybridArray<xiiGameObject*, 8> targets;
    GetOwner()->SearchForChildrenByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti, targets);

    for (xiiGameObject* pTargetObject : targets)
    {
      // allow to animate properties on the xiiGameObject
      if (anim.m_pComponentRtti == nullptr)
      {
        CreateGameObjectBinding(&anim, xiiGetStaticRTTI<xiiGameObject>(), pTargetObject, pTargetObject->GetHandle());
      }
      else
      {
        xiiComponent* pComp;
        if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
        {
          CreateFloatPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
        }
      }
    }
  }

  for (const xiiColorPropertyAnimEntry& anim : m_pAnimDesc->m_ColorAnimations)
  {
    xiiHybridArray<xiiGameObject*, 8> targets;
    GetOwner()->SearchForChildrenByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti, targets);

    for (xiiGameObject* pTargetObject : targets)
    {
      xiiComponent* pComp;
      if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
      {
        CreateColorPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
      }
    }
  }
}

void xiiPropertyAnimComponent::CreateGameObjectBinding(const xiiFloatPropertyAnimEntry* pAnim, const xiiRTTI* pOwnerRtti, void* pObject, const xiiGameObjectHandle& hGameObject)
{
  if (pAnim->m_Target < xiiPropertyAnimTarget::Number || pAnim->m_Target > xiiPropertyAnimTarget::RotationZ)
    return;

  xiiAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != xiiPropertyCategory::Member)
    return;

  xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbstract);

  const xiiRTTI* pPropRtti = pMember->GetSpecificType();

  if (pAnim->m_Target == xiiPropertyAnimTarget::Number)
  {
    // Game objects only support to animate Position, Rotation,
    // Non-Uniform Scale, the one single-float Uniform scale value
    // and the active flag
    if (pPropRtti != xiiGetStaticRTTI<float>() && pPropRtti != xiiGetStaticRTTI<bool>())
      return;
  }
  else if (pAnim->m_Target >= xiiPropertyAnimTarget::RotationX && pAnim->m_Target <= xiiPropertyAnimTarget::RotationZ)
  {
    if (pPropRtti != xiiGetStaticRTTI<xiiQuat>())
      return;
  }
  else
  {
    if (pPropRtti != xiiGetStaticRTTI<xiiVec2>() && pPropRtti != xiiGetStaticRTTI<xiiVec3>() && pPropRtti != xiiGetStaticRTTI<xiiVec4>())
      return;
  }

  GameObjectBinding* binding = nullptr;
  for (xiiUInt32 i = 0; i < m_GoFloatBindings.GetCount(); ++i)
  {
    auto& b = m_GoFloatBindings[i];

    if (b.m_hObject == hGameObject && b.m_pMemberProperty == pMember && b.m_pObject == pObject)
    {
      binding = &b;
      break;
    }
  }

  if (binding == nullptr)
  {
    binding = &m_GoFloatBindings.ExpandAndGetRef();
  }

  binding->m_hObject         = hGameObject;
  binding->m_pObject         = pObject;
  binding->m_pMemberProperty = pMember;

  // we can store a direct pointer here, because our sharedptr keeps the descriptor alive

  if (pAnim->m_Target >= xiiPropertyAnimTarget::VectorX && pAnim->m_Target <= xiiPropertyAnimTarget::VectorW)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)xiiPropertyAnimTarget::VectorX] = pAnim;
  }
  else if (pAnim->m_Target >= xiiPropertyAnimTarget::RotationX && pAnim->m_Target <= xiiPropertyAnimTarget::RotationZ)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)xiiPropertyAnimTarget::RotationX] = pAnim;
  }
  else if (pAnim->m_Target >= xiiPropertyAnimTarget::Number)
  {
    binding->m_pAnimation[0] = pAnim;
  }
  else
  {
    XII_REPORT_FAILURE("Invalid animation target type '{0}'", pAnim->m_Target.GetValue());
  }
}

void xiiPropertyAnimComponent::CreateFloatPropertyBinding(const xiiFloatPropertyAnimEntry* pAnim, const xiiRTTI* pOwnerRtti, void* pObject, const xiiComponentHandle& hComponent)
{
  if (pAnim->m_Target < xiiPropertyAnimTarget::Number || pAnim->m_Target > xiiPropertyAnimTarget::VectorW)
    return;

  xiiAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != xiiPropertyCategory::Member)
    return;

  xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbstract);

  const xiiRTTI* pPropRtti = pMember->GetSpecificType();

  if (pAnim->m_Target == xiiPropertyAnimTarget::Number)
  {
    if (pPropRtti != xiiGetStaticRTTI<float>() && pPropRtti != xiiGetStaticRTTI<double>() && pPropRtti != xiiGetStaticRTTI<bool>() && pPropRtti != xiiGetStaticRTTI<xiiInt64>() && pPropRtti != xiiGetStaticRTTI<xiiInt32>() && pPropRtti != xiiGetStaticRTTI<xiiInt16>() &&
        pPropRtti != xiiGetStaticRTTI<xiiInt8>() && pPropRtti != xiiGetStaticRTTI<xiiUInt64>() && pPropRtti != xiiGetStaticRTTI<xiiUInt32>() && pPropRtti != xiiGetStaticRTTI<xiiUInt16>() && pPropRtti != xiiGetStaticRTTI<xiiUInt8>() && pPropRtti != xiiGetStaticRTTI<xiiAngle>() &&
        pPropRtti != xiiGetStaticRTTI<xiiTime>())
      return;
  }
  else if (pAnim->m_Target >= xiiPropertyAnimTarget::VectorX && pAnim->m_Target <= xiiPropertyAnimTarget::VectorW)
  {
    if (pPropRtti != xiiGetStaticRTTI<xiiVec2>() && pPropRtti != xiiGetStaticRTTI<xiiVec3>() && pPropRtti != xiiGetStaticRTTI<xiiVec4>())
      return;
  }
  else
  {
    // Quaternions are not supported for regular types
    return;
  }

  ComponentFloatBinding* binding = nullptr;
  for (xiiUInt32 i = 0; i < m_ComponentFloatBindings.GetCount(); ++i)
  {
    auto& b = m_ComponentFloatBindings[i];

    if (b.m_hComponent == hComponent && b.m_pMemberProperty == pMember && b.m_pObject == pObject)
    {
      binding = &b;
      break;
    }
  }

  if (binding == nullptr)
  {
    binding = &m_ComponentFloatBindings.ExpandAndGetRef();
  }

  binding->m_hComponent      = hComponent;
  binding->m_pObject         = pObject;
  binding->m_pMemberProperty = pMember;

  // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  if (pAnim->m_Target >= xiiPropertyAnimTarget::VectorX && pAnim->m_Target <= xiiPropertyAnimTarget::VectorW)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)xiiPropertyAnimTarget::VectorX] = pAnim;
  }
  else if (pAnim->m_Target >= xiiPropertyAnimTarget::RotationX && pAnim->m_Target <= xiiPropertyAnimTarget::RotationZ)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)xiiPropertyAnimTarget::RotationX] = pAnim;
  }
  else if (pAnim->m_Target >= xiiPropertyAnimTarget::Number)
  {
    binding->m_pAnimation[0] = pAnim;
  }
  else
  {
    XII_REPORT_FAILURE("Invalid animation target type '{0}'", pAnim->m_Target.GetValue());
  }
}

void xiiPropertyAnimComponent::CreateColorPropertyBinding(const xiiColorPropertyAnimEntry* pAnim, const xiiRTTI* pOwnerRtti, void* pObject, const xiiComponentHandle& hComponent)
{
  if (pAnim->m_Target != xiiPropertyAnimTarget::Color)
    return;

  xiiAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != xiiPropertyCategory::Member)
    return;

  xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbstract);

  const xiiRTTI* pPropRtti = pMember->GetSpecificType();

  if (pPropRtti != xiiGetStaticRTTI<xiiColor>() && pPropRtti != xiiGetStaticRTTI<xiiColorGammaUB>())
    return;

  ColorBinding& binding     = m_ColorBindings.ExpandAndGetRef();
  binding.m_hComponent      = hComponent;
  binding.m_pObject         = pObject;
  binding.m_pAnimation      = pAnim; // we can store a direct pointer here, because our SharedPtr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;
}

void xiiPropertyAnimComponent::ApplyAnimations(const xiiTime& tDiff)
{
  if (m_fSpeed == 0.0f || m_pAnimDesc == nullptr)
    return;

  const xiiTime fLookupPos = ComputeAnimationLookup(tDiff);

  for (xiiUInt32 i = 0; i < m_ComponentFloatBindings.GetCount();)
  {
    const auto& binding = m_ComponentFloatBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      xiiComponent* pComponent;
      if (!GetWorld()->TryGetComponent(binding.m_hComponent, pComponent))
      {
        // remove dead references
        m_ComponentFloatBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pComponent);
    }

    ApplyFloatAnimation(m_ComponentFloatBindings[i], fLookupPos);

    ++i;
  }

  for (xiiUInt32 i = 0; i < m_ColorBindings.GetCount();)
  {
    const auto& binding = m_ColorBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      xiiComponent* pComponent;
      if (!GetWorld()->TryGetComponent(binding.m_hComponent, pComponent))

      {
        // remove dead references
        m_ColorBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pComponent);
    }

    ApplyColorAnimation(m_ColorBindings[i], fLookupPos);

    ++i;
  }

  for (xiiUInt32 i = 0; i < m_GoFloatBindings.GetCount();)
  {
    const auto& binding = m_GoFloatBindings[i];

    // if we have a game object handle, use it to check that the component is still alive
    if (!binding.m_hObject.IsInvalidated())
    {
      xiiGameObject* pObject;
      if (!GetWorld()->TryGetObject(binding.m_hObject, pObject))
      {
        // remove dead references
        m_GoFloatBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pObject);
    }

    ApplyFloatAnimation(m_GoFloatBindings[i], fLookupPos);

    ++i;
  }
}

xiiTime xiiPropertyAnimComponent::ComputeAnimationLookup(xiiTime tDiff)
{
  m_AnimationRangeLow  = xiiMath::Clamp(m_AnimationRangeLow, xiiTime::Zero(), m_pAnimDesc->m_AnimationDuration);
  m_AnimationRangeHigh = xiiMath::Clamp(m_AnimationRangeHigh, m_AnimationRangeLow, m_pAnimDesc->m_AnimationDuration);

  const xiiTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

  if (duration.IsZero())
  {
    m_bPlaying = false;
    return m_AnimationRangeLow;
  }

  tDiff = m_fSpeed * tDiff;

  xiiMsgAnimationReachedEnd reachedEndMsg;
  xiiTime                   tStart = m_AnimationTime;

  if (m_AnimationMode == xiiPropertyAnimMode::Once)
  {
    m_AnimationTime += tDiff;

    if (m_fSpeed > 0 && m_AnimationTime >= m_AnimationRangeHigh)
    {
      m_AnimationTime = m_AnimationRangeHigh;
      m_bPlaying      = false;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
    }
    else if (m_fSpeed < 0 && m_AnimationTime <= m_AnimationRangeLow)
    {
      m_AnimationTime = m_AnimationRangeLow;
      m_bPlaying      = false;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
    }

    EvaluateEventTrack(tStart, m_AnimationTime);
  }
  else if (m_AnimationMode == xiiPropertyAnimMode::Loop)
  {
    m_AnimationTime += tDiff;

    while (m_AnimationTime > m_AnimationRangeHigh)
    {
      m_AnimationTime -= duration;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());

      EvaluateEventTrack(tStart, m_AnimationRangeHigh);
      tStart = m_AnimationRangeLow;
    }

    while (m_AnimationTime < m_AnimationRangeLow)
    {
      m_AnimationTime += duration;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());

      EvaluateEventTrack(tStart, m_AnimationRangeLow);
      tStart = m_AnimationRangeHigh;
    }

    EvaluateEventTrack(tStart, m_AnimationTime);
  }
  else if (m_AnimationMode == xiiPropertyAnimMode::BackAndForth)
  {
    const bool bReverse = m_fSpeed < 0 ? !m_bReverse : m_bReverse;

    if (bReverse)
      m_AnimationTime -= tDiff;
    else
      m_AnimationTime += tDiff;

    // ping pong back and forth as long as the current animation time is outside the valid range
    while (true)
    {
      if (m_AnimationTime > m_AnimationRangeHigh)
      {
        m_AnimationTime = m_AnimationRangeHigh - (m_AnimationTime - m_AnimationRangeHigh);
        m_bReverse      = true;

        EvaluateEventTrack(tStart, m_AnimationRangeHigh);
        tStart = m_AnimationRangeHigh;

        m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
      }
      else if (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime = m_AnimationRangeLow + (m_AnimationRangeLow - m_AnimationTime);
        m_bReverse      = false;

        EvaluateEventTrack(tStart, m_AnimationRangeLow);
        tStart = m_AnimationRangeLow;

        m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
      }
      else
      {
        EvaluateEventTrack(tStart, m_AnimationTime);
        break;
      }
    }
  }

  return m_AnimationTime;
}

void xiiPropertyAnimComponent::EvaluateEventTrack(xiiTime startTime, xiiTime endTime)
{
  const xiiEventTrack& et = m_pAnimDesc->m_EventTrack;

  if (et.IsEmpty())
    return;

  xiiHybridArray<xiiHashedString, 8> events;
  et.Sample(startTime, endTime, events);

  for (const xiiHashedString& sEvent : events)
  {
    xiiMsgGenericEvent msg;
    msg.m_sMessage = sEvent;
    m_EventTrackMsgSender.SendEventMessage(msg, this, GetOwner());
  }
}

void xiiPropertyAnimComponent::OnSimulationStarted()
{
  CreatePropertyBindings();

  StartPlayback();
}

void xiiPropertyAnimComponent::StartPlayback()
{
  if (m_pAnimDesc == nullptr)
    return;

  m_AnimationRangeLow  = xiiMath::Clamp(m_AnimationRangeLow, xiiTime::Zero(), m_pAnimDesc->m_AnimationDuration);
  m_AnimationRangeHigh = xiiMath::Clamp(m_AnimationRangeHigh, m_AnimationRangeLow, m_pAnimDesc->m_AnimationDuration);

  // when starting with a negative speed, start at the end of the animation and play backwards
  // important for play-once mode
  if (m_fSpeed < 0.0f)
  {
    m_AnimationTime = m_AnimationRangeHigh;
  }
  else
  {
    m_AnimationTime = m_AnimationRangeLow;
  }

  if (!m_RandomOffset.IsZero() && m_pAnimDesc->m_AnimationDuration.IsPositive())
  {
    // should the random offset also be scaled by the speed factor? I guess not
    m_AnimationTime += xiiMath::Abs(m_fSpeed) * xiiTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_RandomOffset.GetSeconds()));

    const xiiTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

    if (duration.IsZeroOrNegative())
    {
      m_AnimationTime = m_AnimationRangeLow;
    }
    else
    {
      // adjust current time to be inside the valid range
      // do not clamp, as that would give a skewed random chance
      while (m_AnimationTime > m_AnimationRangeHigh)
      {
        m_AnimationTime -= duration;
      }

      while (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime += duration;
      }
    }
  }
}

void xiiPropertyAnimComponent::ApplySingleFloatAnimation(const FloatBinding& binding, xiiTime lookupTime)
{
  const xiiRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  double fFinalValue = 0;
  {
    const xiiCurve1D& curve = binding.m_pAnimation[0]->m_Curve;

    if (curve.IsEmpty())
      return;

    fFinalValue = curve.Evaluate(lookupTime.GetSeconds());
  }

  if (pRtti == xiiGetStaticRTTI<bool>())
  {
    xiiTypedMemberProperty<bool>* pTyped = static_cast<xiiTypedMemberProperty<bool>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject,
                     fFinalValue > 0.99); // this is close to what xiiVariant does (not identical, that does an int cast != 0), but faster to evaluate
    return;
  }
  else if (pRtti == xiiGetStaticRTTI<xiiAngle>())
  {
    xiiTypedMemberProperty<xiiAngle>* pTyped = static_cast<xiiTypedMemberProperty<xiiAngle>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, xiiAngle::Degree((float)fFinalValue));
    return;
  }
  else if (pRtti == xiiGetStaticRTTI<xiiTime>())
  {
    xiiTypedMemberProperty<xiiTime>* pTyped = static_cast<xiiTypedMemberProperty<xiiTime>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, xiiTime::Seconds(fFinalValue));
    return;
  }

  // this handles float, double, all int types, etc.
  xiiVariant value = fFinalValue;
  if (pRtti->GetVariantType() != xiiVariantType::Invalid && value.CanConvertTo(pRtti->GetVariantType()))
  {
    xiiReflectionUtils::SetMemberPropertyValue(binding.m_pMemberProperty, binding.m_pObject, value);
  }
}

void xiiPropertyAnimComponent::ApplyFloatAnimation(const FloatBinding& binding, xiiTime lookupTime)
{
  if (binding.m_pAnimation[0] != nullptr && binding.m_pAnimation[0]->m_Target == xiiPropertyAnimTarget::Number)
  {
    ApplySingleFloatAnimation(binding, lookupTime);
    return;
  }

  const xiiRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  float fCurValue[4] = {0, 0, 0, 0};

  if (pRtti == xiiGetStaticRTTI<xiiVec2>())
  {
    xiiTypedMemberProperty<xiiVec2>* pTyped = static_cast<xiiTypedMemberProperty<xiiVec2>*>(binding.m_pMemberProperty);
    const xiiVec2                    value  = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
  }
  else if (pRtti == xiiGetStaticRTTI<xiiVec3>())
  {
    xiiTypedMemberProperty<xiiVec3>* pTyped = static_cast<xiiTypedMemberProperty<xiiVec3>*>(binding.m_pMemberProperty);
    const xiiVec3                    value  = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
    fCurValue[2] = value.z;
  }
  else if (pRtti == xiiGetStaticRTTI<xiiVec4>())
  {
    xiiTypedMemberProperty<xiiVec4>* pTyped = static_cast<xiiTypedMemberProperty<xiiVec4>*>(binding.m_pMemberProperty);
    const xiiVec4                    value  = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
    fCurValue[2] = value.z;
    fCurValue[3] = value.w;
  }
  else if (pRtti == xiiGetStaticRTTI<xiiQuat>())
  {
    xiiTypedMemberProperty<xiiQuat>* pTyped = static_cast<xiiTypedMemberProperty<xiiQuat>*>(binding.m_pMemberProperty);
    const xiiQuat                    value  = pTyped->GetValue(binding.m_pObject);

    xiiAngle euler[3];
    value.GetAsEulerAngles(euler[0], euler[1], euler[2]);
    fCurValue[0] = euler[0].GetDegree();
    fCurValue[1] = euler[1].GetDegree();
    fCurValue[2] = euler[2].GetDegree();
  }

  // evaluate all available curves
  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    if (binding.m_pAnimation[i] != nullptr)
    {
      const xiiCurve1D& curve = binding.m_pAnimation[i]->m_Curve;

      if (!curve.IsEmpty())
      {
        fCurValue[i] = (float)curve.Evaluate(lookupTime.GetSeconds());
      }
    }
  }

  if (pRtti == xiiGetStaticRTTI<xiiVec2>())
  {
    xiiTypedMemberProperty<xiiVec2>* pTyped = static_cast<xiiTypedMemberProperty<xiiVec2>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, xiiVec2(fCurValue[0], fCurValue[1]));
  }
  else if (pRtti == xiiGetStaticRTTI<xiiVec3>())
  {
    xiiTypedMemberProperty<xiiVec3>* pTyped = static_cast<xiiTypedMemberProperty<xiiVec3>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, xiiVec3(fCurValue[0], fCurValue[1], fCurValue[2]));
  }
  else if (pRtti == xiiGetStaticRTTI<xiiVec4>())
  {
    xiiTypedMemberProperty<xiiVec4>* pTyped = static_cast<xiiTypedMemberProperty<xiiVec4>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, xiiVec4(fCurValue[0], fCurValue[1], fCurValue[2], fCurValue[3]));
  }
  else if (pRtti == xiiGetStaticRTTI<xiiQuat>())
  {
    xiiTypedMemberProperty<xiiQuat>* pTyped = static_cast<xiiTypedMemberProperty<xiiQuat>*>(binding.m_pMemberProperty);

    xiiQuat rot;
    rot.SetFromEulerAngles(xiiAngle::Degree(fCurValue[0]), xiiAngle::Degree(fCurValue[1]), xiiAngle::Degree(fCurValue[2]));

    pTyped->SetValue(binding.m_pObject, rot);
  }
}

void xiiPropertyAnimComponent::ApplyColorAnimation(const ColorBinding& binding, xiiTime lookupTime)
{
  const xiiRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  if (pRtti == xiiGetStaticRTTI<xiiColorGammaUB>())
  {
    xiiColorGammaUB gamma;
    float           intensity;
    binding.m_pAnimation->m_Gradient.Evaluate(lookupTime.AsFloatInSeconds(), gamma, intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &gamma);
    return;
  }

  if (pRtti == xiiGetStaticRTTI<xiiColor>())
  {
    xiiColorGammaUB gamma;
    float           intensity;
    binding.m_pAnimation->m_Gradient.Evaluate(lookupTime.AsFloatInSeconds(), gamma, intensity);

    xiiColor finalColor = gamma;
    finalColor.ScaleRGB(intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &finalColor);
    return;
  }
}

void xiiPropertyAnimComponent::Update()
{
  if (m_bPlaying == false || !m_hPropertyAnim.IsValid())
    return;

  if (m_pAnimDesc == nullptr)
  {
    CreatePropertyBindings();
  }

  ApplyAnimations(GetWorld()->GetClock().GetTimeDiff());
}



XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PropertyAnimComponent);
