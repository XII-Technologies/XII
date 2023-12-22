#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAnimationTrack, 1, xiiRTTIDefaultAllocator<xiiPropertyAnimationTrack>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectPath", m_sObjectSearchSequence),
    XII_MEMBER_PROPERTY("ComponentType", m_sComponentType),
    XII_MEMBER_PROPERTY("Property", m_sPropertyPath),
    XII_ENUM_MEMBER_PROPERTY("Target", xiiPropertyAnimTarget, m_Target),
    XII_MEMBER_PROPERTY("FloatCurve", m_FloatCurve),
    XII_MEMBER_PROPERTY("Gradient", m_ColorGradient),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAnimationTrackGroup, 1, xiiRTTIDefaultAllocator<xiiPropertyAnimationTrackGroup>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new xiiDefaultValueAttribute(60)),
    XII_MEMBER_PROPERTY("Duration", m_uiCurveDuration)->AddAttributes(new xiiDefaultValueAttribute(480)),
    XII_ARRAY_MEMBER_PROPERTY("Tracks", m_Tracks)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_MEMBER_PROPERTY("EventTrack", m_EventTrack),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAnimAssetDocument, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiPropertyAnimationTrackGroup::~xiiPropertyAnimationTrackGroup()
{
  for (xiiPropertyAnimationTrack* pTrack : m_Tracks)
  {
    XII_DEFAULT_DELETE(pTrack);
  }
}

xiiPropertyAnimAssetDocument::xiiPropertyAnimAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiPropertyAnimationTrackGroup, xiiGameObjectContextDocument>(
    XII_DEFAULT_NEW(xiiPropertyAnimObjectManager),
    sDocumentPath,
    xiiAssetDocEngineConnection::FullObjectMirroring)
{
  m_GameObjectContextEvents.AddEventHandler(xiiMakeDelegate(&xiiPropertyAnimAssetDocument::GameObjectContextEventHandler, this));
  m_pObjectAccessor = XII_DEFAULT_NEW(xiiPropertyAnimObjectAccessor, this, GetCommandHistory());
}

xiiPropertyAnimAssetDocument::~xiiPropertyAnimAssetDocument()
{
  m_GameObjectContextEvents.RemoveEventHandler(xiiMakeDelegate(&xiiPropertyAnimAssetDocument::GameObjectContextEventHandler, this));

  GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiPropertyAnimAssetDocument::TreePropertyEventHandler, this));
}

void xiiPropertyAnimAssetDocument::SetAnimationDurationTicks(xiiUInt64 uiNumTicks)
{
  const xiiPropertyAnimationTrackGroup* pProp = GetProperties();

  if (pProp->m_uiCurveDuration == uiNumTicks)
    return;

  {
    xiiCommandHistory* history = GetCommandHistory();
    history->StartTransaction("Set Animation Duration");

    xiiSetObjectPropertyCommand cmdSet;
    cmdSet.m_Object    = GetPropertyObject()->GetGuid();
    cmdSet.m_sProperty = "Duration";
    cmdSet.m_NewValue  = uiNumTicks;
    history->AddCommand(cmdSet).AssertSuccess();

    history->FinishTransaction();
  }

  {
    xiiPropertyAnimAssetDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type      = xiiPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged;
    m_PropertyAnimEvents.Broadcast(e);
  }
}

xiiUInt64 xiiPropertyAnimAssetDocument::GetAnimationDurationTicks() const
{
  const xiiPropertyAnimationTrackGroup* pProp = GetProperties();

  return pProp->m_uiCurveDuration;
}


xiiTime xiiPropertyAnimAssetDocument::GetAnimationDurationTime() const
{
  const xiiInt64 ticks = GetAnimationDurationTicks();

  return xiiTime::MakeFromSeconds(ticks / 4800.0);
}

void xiiPropertyAnimAssetDocument::AdjustDuration()
{
  xiiUInt64 uiDuration = 480;

  const xiiPropertyAnimationTrackGroup* pProp = GetProperties();

  for (xiiUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const xiiPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    for (const auto& cp : pTrack->m_FloatCurve.m_ControlPoints)
    {
      uiDuration = xiiMath::Max(uiDuration, (xiiUInt64)cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_ColorCPs)
    {
      uiDuration = xiiMath::Max<xiiInt64>(uiDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_AlphaCPs)
    {
      uiDuration = xiiMath::Max<xiiInt64>(uiDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_IntensityCPs)
    {
      uiDuration = xiiMath::Max<xiiInt64>(uiDuration, cp.m_iTick);
    }
  }

  SetAnimationDurationTicks(uiDuration);
}

bool xiiPropertyAnimAssetDocument::SetScrubberPosition(xiiUInt64 uiTick)
{
  if (!m_bPlayAnimation)
  {
    const xiiUInt32 uiTicksPerFrame = 4800 / GetProperties()->m_uiFramesPerSecond;
    uiTick                          = (xiiUInt64)xiiMath::RoundToMultiple((double)uiTick, (double)uiTicksPerFrame);
  }
  uiTick = xiiMath::Clamp<xiiUInt64>(uiTick, 0, GetAnimationDurationTicks());

  if (m_uiScrubberTickPos == uiTick)
    return false;

  m_uiScrubberTickPos = uiTick;
  ApplyAnimation();

  xiiPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged;
  m_PropertyAnimEvents.Broadcast(e);

  return true;
}

xiiTransformStatus xiiPropertyAnimAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const xiiPropertyAnimationTrackGroup* pProp = GetProperties();

  xiiPropertyAnimResourceDescriptor desc;
  desc.m_AnimationDuration = GetAnimationDurationTime();

  for (xiiUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const xiiPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    if (pTrack->m_Target == xiiPropertyAnimTarget::Color)
    {
      auto& anim                   = desc.m_ColorAnimations.ExpandAndGetRef();
      anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      anim.m_sComponentType        = pTrack->m_sComponentType;
      anim.m_sPropertyPath         = pTrack->m_sPropertyPath;
      anim.m_Target                = pTrack->m_Target;
      pTrack->m_ColorGradient.FillGradientData(anim.m_Gradient);
      anim.m_Gradient.SortControlPoints();
    }
    else
    {
      auto& anim                   = desc.m_FloatAnimations.ExpandAndGetRef();
      anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      anim.m_sComponentType        = pTrack->m_sComponentType;
      anim.m_sPropertyPath         = pTrack->m_sPropertyPath;
      anim.m_Target                = pTrack->m_Target;
      pTrack->m_FloatCurve.ConvertToRuntimeData(anim.m_Curve);
      anim.m_Curve.SortControlPoints();
      anim.m_Curve.ApplyTangentModes();
      anim.m_Curve.ClampTangents();
    }
  }

  // sort animation tracks by object path for better cache reuse at runtime
  {
    desc.m_FloatAnimations.Sort([](const xiiFloatPropertyAnimEntry& lhs, const xiiFloatPropertyAnimEntry& rhs) -> bool {
      const xiiInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType; });

    desc.m_ColorAnimations.Sort([](const xiiColorPropertyAnimEntry& lhs, const xiiColorPropertyAnimEntry& rhs) -> bool {
      const xiiInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType; });
  }

  pProp->m_EventTrack.ConvertToRuntimeData(desc.m_EventTrack);

  desc.Save(stream);

  return xiiStatus(XII_SUCCESS);
}


void xiiPropertyAnimAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = XII_DEFAULT_NEW(xiiIPCObjectMirrorEditor);
  // Filter needs to be set before base class init as that one sends the doc.
  // (Local mirror ignores temporaries, i.e. only mirrors the asset itself)
  m_ObjectMirror.SetFilterFunction([this](const xiiDocumentObject* pObject, xiiStringView sProperty) -> bool { return !static_cast<xiiPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, sProperty); });
  // (Remote IPC mirror only sends temporaries, i.e. the context)
  m_pMirror->SetFilterFunction([this](const xiiDocumentObject* pObject, xiiStringView sProperty) -> bool { return static_cast<xiiPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, sProperty); });
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  // Important to do these after base class init as we want our subscriptions to happen after the mirror of the base class.
  GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiPropertyAnimAssetDocument::TreePropertyEventHandler, this));
  // Subscribe here as otherwise base init will fire a context changed event when we are not set up yet.
  // RebuildMapping();
}

void xiiPropertyAnimAssetDocument::GameObjectContextEventHandler(const xiiGameObjectContextEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectContextEvent::Type::ContextAboutToBeChanged:
      static_cast<xiiPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(true);
      break;
    case xiiGameObjectContextEvent::Type::ContextChanged:
      static_cast<xiiPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(false);
      RebuildMapping();
      break;
  }
}

void xiiPropertyAnimAssetDocument::TreeStructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  auto pManager = static_cast<xiiPropertyAnimObjectManager*>(GetObjectManager());
  if (e.m_pPreviousParent && pManager->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty))
    return;
  if (e.m_pNewParent && pManager->IsTemporary(e.m_pNewParent, e.m_sParentProperty))
    return;

  if (e.m_pObject->GetType() == xiiGetStaticRTTI<xiiPropertyAnimationTrack>())
  {
    switch (e.m_EventType)
    {
      case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
      case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved:
        AddTrack(e.m_pObject->GetGuid());
        return;
      case xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
      case xiiDocumentObjectStructureEvent::Type::BeforeObjectMoved:
        RemoveTrack(e.m_pObject->GetGuid());
        return;

      default:
        break;
    }
  }
  else
  {
    ApplyAnimation();
  }
}

void xiiPropertyAnimAssetDocument::TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  auto pManager = static_cast<xiiPropertyAnimObjectManager*>(GetObjectManager());
  if (pManager->IsTemporary(e.m_pObject))
    return;

  if (e.m_pObject->GetType() == xiiGetStaticRTTI<xiiPropertyAnimationTrack>())
  {
    if (e.m_EventType == xiiDocumentObjectPropertyEvent::Type::PropertySet)
    {
      RemoveTrack(e.m_pObject->GetGuid());
      AddTrack(e.m_pObject->GetGuid());
      return;
    }
  }
  else
  {
    ApplyAnimation();
  }
}

void xiiPropertyAnimAssetDocument::RebuildMapping()
{
  while (!m_TrackTable.IsEmpty())
  {
    RemoveTrack(m_TrackTable.GetIterator().Key());
  }
  XII_ASSERT_DEBUG(m_PropertyTable.IsEmpty() && m_TrackTable.IsEmpty(), "All tracks should be removed.");

  const xiiAbstractProperty* pTracksProp = xiiGetStaticRTTI<xiiPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks");
  XII_ASSERT_DEBUG(pTracksProp, "Name of property xiiPropertyAnimationTrackGroup::m_Tracks has changed.");
  xiiHybridArray<xiiVariant, 16> values;
  m_pObjectAccessor->GetValues(GetPropertyObject(), pTracksProp, values).AssertSuccess();
  for (const xiiVariant& value : values)
  {
    AddTrack(value.Get<xiiUuid>());
  }
}

void xiiPropertyAnimAssetDocument::RemoveTrack(const xiiUuid& track)
{
  auto& keys = *m_TrackTable.GetValue(track);
  for (const xiiPropertyReference& key : keys)
  {
    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.RemoveAndSwap(track);
    ApplyAnimation(key, value);
    if (value.m_Tracks.IsEmpty())
      m_PropertyTable.Remove(key);
  }
  m_TrackTable.Remove(track);
}

void xiiPropertyAnimAssetDocument::AddTrack(const xiiUuid& track)
{
  XII_ASSERT_DEV(!m_TrackTable.Contains(track), "Track already exists.");
  auto&                    keys     = m_TrackTable[track];
  const xiiDocumentObject* pContext = GetContextObject();
  if (!pContext)
    return;

  auto pTrack = GetTrack(track);
  FindTrackKeys(pTrack->m_sObjectSearchSequence.GetData(), pTrack->m_sComponentType.GetData(), pTrack->m_sPropertyPath.GetData(), keys).IgnoreResult();

  for (const xiiPropertyReference& key : keys)
  {
    if (!m_PropertyTable.Contains(key))
    {
      PropertyValue value;
      XII_VERIFY(m_pObjectAccessor->GetValue(GetObjectManager()->GetObject(key.m_Object), key.m_pProperty, value.m_InitialValue, key.m_Index).Succeeded(),
                 "Computed key invalid, does not resolve to a value.");
      m_PropertyTable.Insert(key, value);
    }

    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.PushBack(track);
    ApplyAnimation(key, value);
  }
}


xiiStatus xiiPropertyAnimAssetDocument::FindTrackKeys(const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, xiiHybridArray<xiiPropertyReference, 1>& keys) const
{
  xiiObjectPropertyPathContext context = {GetContextObject(), m_pObjectAccessor.Borrow(), "TempObjects"};

  keys.Clear();
  return xiiObjectPropertyPath::ResolvePath(context, keys, szObjectSearchSequence, szComponentType, szPropertyPath);
}


void xiiPropertyAnimAssetDocument::GenerateTrackInfo(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiStringBuilder& sObjectSearchSequence, xiiStringBuilder& sComponentType, xiiStringBuilder& sPropertyPath) const
{
  xiiObjectPropertyPathContext context     = {GetContextObject(), m_pObjectAccessor.Borrow(), "TempObjects"};
  xiiPropertyReference         propertyRef = {pObject->GetGuid(), pProp, index};
  xiiObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath).AssertSuccess();
}

void xiiPropertyAnimAssetDocument::ApplyAnimation()
{
  for (auto it = m_PropertyTable.GetIterator(); it.IsValid(); ++it)
  {
    ApplyAnimation(it.Key(), it.Value());
  }
}

void xiiPropertyAnimAssetDocument::ApplyAnimation(const xiiPropertyReference& key, const PropertyValue& value)
{
  xiiVariant animValue = value.m_InitialValue;
  xiiAngle   euler[3];
  bool       bIsRotation = false;

  for (const xiiUuid& track : value.m_Tracks)
  {
    auto           pTrack    = GetTrack(track);
    const xiiRTTI* pPropRtti = key.m_pProperty->GetSpecificType();

    // #TODO apply pTrack to animValue
    switch (pTrack->m_Target)
    {
      case xiiPropertyAnimTarget::Number:
      {
        if (pPropRtti->GetVariantType() >= xiiVariantType::Bool && pPropRtti->GetVariantType() <= xiiVariantType::Double)
        {
          xiiVariant value2 = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);
          animValue         = value2.ConvertTo(animValue.GetType());
        }
      }
      break;

      case xiiPropertyAnimTarget::VectorX:
      case xiiPropertyAnimTarget::VectorY:
      case xiiPropertyAnimTarget::VectorZ:
      case xiiPropertyAnimTarget::VectorW:
      {
        if (pPropRtti->GetVariantType() >= xiiVariantType::Vector2 && pPropRtti->GetVariantType() <= xiiVariantType::Vector4U)
        {
          const double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);

          xiiReflectionUtils::SetComponent(animValue, (xiiUInt32)pTrack->m_Target - xiiPropertyAnimTarget::VectorX, fValue);
        }
      }
      break;

      case xiiPropertyAnimTarget::RotationX:
      case xiiPropertyAnimTarget::RotationY:
      case xiiPropertyAnimTarget::RotationZ:
      {
        if (pPropRtti->GetVariantType() == xiiVariantType::Quaternion)
        {
          bIsRotation         = true;
          const double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);

          euler[(xiiUInt32)pTrack->m_Target - xiiPropertyAnimTarget::RotationX] = xiiAngle::Degree(fValue);
        }
      }
      break;

      case xiiPropertyAnimTarget::Color:
      {
        if (pPropRtti->GetVariantType() == xiiVariantType::Color || pPropRtti->GetVariantType() == xiiVariantType::ColorGamma)
        {
          xiiVariant value2 = pTrack->m_ColorGradient.Evaluate(m_uiScrubberTickPos);
          animValue         = value2.ConvertTo(animValue.GetType());
        }
      }
      break;
    }
  }

  if (bIsRotation)
  {
    xiiQuat qRotation;
    qRotation = xiiQuat::MakeFromEulerAngles(euler[0], euler[1], euler[2]);
    animValue = qRotation;
  }

  xiiDocumentObject* pObj = GetObjectManager()->GetObject(key.m_Object);
  xiiVariant         oldValue;
  XII_VERIFY(m_pObjectAccessor->GetValue(pObj, key.m_pProperty, oldValue, key.m_Index).Succeeded(), "Retrieving old value failed.");

  if (oldValue != animValue)
    GetObjectManager()->SetValue(pObj, key.m_pProperty->GetPropertyName(), animValue, key.m_Index).AssertSuccess();

  // tell the gizmos and manipulators that they should update their transform
  // usually they listen to the command history and selection events, but in this case no commands are executed
  {
    xiiGameObjectEvent e;
    e.m_Type = xiiGameObjectEvent::Type::GizmoTransformMayBeInvalid;
    m_GameObjectEvents.Broadcast(e);
  }
}

void xiiPropertyAnimAssetDocument::SetPlayAnimation(bool bPlay)
{
  if (m_bPlayAnimation == bPlay)
    return;

  if (m_uiScrubberTickPos >= GetAnimationDurationTicks())
    m_uiScrubberTickPos = 0;

  m_bPlayAnimation = bPlay;
  if (!m_bPlayAnimation)
  {
    // During playback we do not round to frames, so we need to round it again on stop.
    SetScrubberPosition(GetScrubberPosition());
  }
  m_LastFrameTime = xiiTime::Now();

  xiiPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiPropertyAnimAssetDocumentEvent::Type::PlaybackChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

void xiiPropertyAnimAssetDocument::SetRepeatAnimation(bool bRepeat)
{
  if (m_bRepeatAnimation == bRepeat)
    return;

  m_bRepeatAnimation = bRepeat;

  xiiPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiPropertyAnimAssetDocumentEvent::Type::PlaybackChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

void xiiPropertyAnimAssetDocument::ExecuteAnimationPlaybackStep()
{
  const xiiTime   currentTime = xiiTime::Now();
  const xiiTime   tDiff       = (currentTime - m_LastFrameTime) * GetSimulationSpeed();
  const xiiUInt64 uiTicks     = (xiiUInt64)(tDiff.GetSeconds() * 4800.0);
  // Accumulate further if we render too fast and round ticks to zero.
  if (uiTicks == 0)
    return;

  m_LastFrameTime          = currentTime;
  const xiiUInt64 uiNewPos = GetScrubberPosition() + uiTicks;
  SetScrubberPosition(uiNewPos);

  if (uiNewPos > GetAnimationDurationTicks())
  {
    SetPlayAnimation(false);

    if (m_bRepeatAnimation)
      SetPlayAnimation(true);
  }
}

const xiiPropertyAnimationTrack* xiiPropertyAnimAssetDocument::GetTrack(const xiiUuid& track) const
{
  return const_cast<xiiPropertyAnimAssetDocument*>(this)->GetTrack(track);
}

xiiPropertyAnimationTrack* xiiPropertyAnimAssetDocument::GetTrack(const xiiUuid& track)
{
  auto obj = m_Context.GetObjectByGUID(track);
  XII_ASSERT_DEBUG(obj.m_pType == xiiGetStaticRTTI<xiiPropertyAnimationTrack>(),
                   "Track guid does not resolve to a track, "
                   "either the track is not yet created in the mirror or already destroyed. Make sure callbacks are executed in the right order.");
  auto pTrack = static_cast<xiiPropertyAnimationTrack*>(obj.m_pObject);
  return pTrack;
}


xiiStatus xiiPropertyAnimAssetDocument::CanAnimate(
  const xiiDocumentObject*    pObject,
  const xiiAbstractProperty*  pProp,
  xiiVariant                  index,
  xiiPropertyAnimTarget::Enum target) const
{
  if (!pObject)
    return xiiStatus("Object is null.");
  if (!pProp)
    return xiiStatus("Property is null.");
  if (index.IsValid())
    return xiiStatus("Property indices not supported.");

  if (!GetContextObject())
    return xiiStatus("No context set.");

  {
    const xiiDocumentObject* pNode = pObject;
    while (pNode && pNode != GetContextObject())
    {
      pNode = pNode->GetParent();
    }
    if (!pNode)
    {
      return xiiStatus("Object not below context sub-tree.");
    }
  }
  xiiPropertyReference key;
  key.m_Object    = pObject->GetGuid();
  key.m_pProperty = pProp;
  key.m_Index     = index;

  xiiStringBuilder sObjectSearchSequence;
  xiiStringBuilder sComponentType;
  xiiStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);

  const xiiAbstractProperty* pName = xiiGetStaticRTTI<xiiGameObject>()->FindPropertyByName("Name");
  const xiiDocumentObject*   pNode = pObject;
  while (pNode != GetContextObject() && pNode->GetType() != xiiGetStaticRTTI<xiiGameObject>())
  {
    pNode = pNode->GetParent();
  }
  xiiString sName = m_pObjectAccessor->Get<xiiString>(pNode, pName);

  if (sName.IsEmpty() && pNode != GetContextObject())
  {
    return xiiStatus("Empty node name only allowed on context root object animations.");
  }

  xiiHybridArray<xiiPropertyReference, 1> keys;
  return FindTrackKeys(sObjectSearchSequence.GetData(), sComponentType.GetData(), sPropertyPath.GetData(), keys);
}

xiiUuid xiiPropertyAnimAssetDocument::FindTrack(
  const xiiDocumentObject*    pObject,
  const xiiAbstractProperty*  pProp,
  xiiVariant                  index,
  xiiPropertyAnimTarget::Enum target) const
{
  xiiPropertyReference key;
  key.m_Object    = pObject->GetGuid();
  key.m_pProperty = pProp;
  key.m_Index     = index;
  if (const PropertyValue* value = m_PropertyTable.GetValue(key))
  {
    for (const xiiUuid& track : value->m_Tracks)
    {
      auto pTrack = GetTrack(track);
      if (pTrack->m_Target == target)
        return track;
    }
  }
  return xiiUuid();
}

static xiiColorGammaUB g_CurveColors[10][3] = {
  {xiiColorGammaUB(255, 102, 0), xiiColorGammaUB(76, 255, 0), xiiColorGammaUB(0, 255, 255)},
  {xiiColorGammaUB(239, 35, 0), xiiColorGammaUB(127, 255, 0), xiiColorGammaUB(0, 0, 255)},
  {xiiColorGammaUB(205, 92, 92), xiiColorGammaUB(120, 158, 39), xiiColorGammaUB(81, 120, 188)},
  {xiiColorGammaUB(255, 105, 180), xiiColorGammaUB(0, 250, 154), xiiColorGammaUB(0, 191, 255)},
  {xiiColorGammaUB(220, 20, 60), xiiColorGammaUB(0, 255, 127), xiiColorGammaUB(30, 144, 255)},
  {xiiColorGammaUB(240, 128, 128), xiiColorGammaUB(60, 179, 113), xiiColorGammaUB(135, 206, 250)},
  {xiiColorGammaUB(178, 34, 34), xiiColorGammaUB(46, 139, 87), xiiColorGammaUB(65, 105, 225)},
  {xiiColorGammaUB(211, 122, 122), xiiColorGammaUB(144, 238, 144), xiiColorGammaUB(135, 206, 235)},
  {xiiColorGammaUB(219, 112, 147), xiiColorGammaUB(0, 128, 0), xiiColorGammaUB(70, 130, 180)},
  {xiiColorGammaUB(255, 182, 193), xiiColorGammaUB(102, 205, 170), xiiColorGammaUB(100, 149, 237)},
};

static xiiColorGammaUB g_FloatColors[10] = {
  xiiColorGammaUB(138, 43, 226),
  xiiColorGammaUB(139, 0, 139),
  xiiColorGammaUB(153, 50, 204),
  xiiColorGammaUB(148, 0, 211),
  xiiColorGammaUB(218, 112, 214),
  xiiColorGammaUB(221, 160, 221),
  xiiColorGammaUB(128, 0, 128),
  xiiColorGammaUB(102, 51, 153),
  xiiColorGammaUB(106, 90, 205),
  xiiColorGammaUB(238, 130, 238),
};

xiiUuid xiiPropertyAnimAssetDocument::CreateTrack(
  const xiiDocumentObject*    pObject,
  const xiiAbstractProperty*  pProp,
  xiiVariant                  index,
  xiiPropertyAnimTarget::Enum target)
{
  xiiStringBuilder sObjectSearchSequence;
  xiiStringBuilder sComponentType;
  xiiStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);

  xiiObjectCommandAccessor accessor(GetCommandHistory());
  const xiiRTTI*           pTrackType = xiiGetStaticRTTI<xiiPropertyAnimationTrack>();
  xiiUuid                  newTrack;
  XII_VERIFY(
    accessor.AddObject(GetPropertyObject(), xiiGetStaticRTTI<xiiPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks"), -1, pTrackType, newTrack)
      .Succeeded(),
    "Adding track failed.");
  const xiiDocumentObject* pTrackObj = accessor.GetObject(newTrack);
  xiiVariant               value     = sObjectSearchSequence.GetData();
  XII_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("ObjectPath"), value).Succeeded(), "Adding track failed.");
  value = sComponentType.GetData();
  XII_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("ComponentType"), value).Succeeded(), "Adding track failed.");
  value = sPropertyPath.GetData();
  XII_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("Property"), value).Succeeded(), "Adding track failed.");
  value = (int)target;
  XII_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("Target"), value).Succeeded(), "Adding track failed.");

  {
    const xiiAbstractProperty* pFloatCurveProp   = pTrackType->FindPropertyByName("FloatCurve");
    xiiUuid                    floatCurveGuid    = accessor.Get<xiiUuid>(pTrackObj, pFloatCurveProp);
    const xiiDocumentObject*   pFloatCurveObject = GetObjectManager()->GetObject(floatCurveGuid);

    const xiiAbstractProperty* pColorProp = xiiGetStaticRTTI<xiiSingleCurveData>()->FindPropertyByName("Color");

    xiiColorGammaUB color = xiiColor::White;

    const xiiUInt32 uiNameHash = xiiHashingUtils::xxHash32(sObjectSearchSequence.GetData(), sObjectSearchSequence.GetElementCount());
    const xiiUInt32 uiColorIdx = uiNameHash % XII_ARRAY_SIZE(g_CurveColors);

    switch (target)
    {
      case xiiPropertyAnimTarget::Number:
        color = g_FloatColors[uiColorIdx];
        break;
      case xiiPropertyAnimTarget::VectorX:
      case xiiPropertyAnimTarget::RotationX:
        color = g_CurveColors[uiColorIdx][0];
        break;
      case xiiPropertyAnimTarget::VectorY:
      case xiiPropertyAnimTarget::RotationY:
        color = g_CurveColors[uiColorIdx][1];
        break;
      case xiiPropertyAnimTarget::VectorZ:
      case xiiPropertyAnimTarget::RotationZ:
        color = g_CurveColors[uiColorIdx][2];
        break;
      case xiiPropertyAnimTarget::VectorW:
        color = xiiColor::Beige;
        break;
      default:
        break;
    }

    accessor.SetValue(pFloatCurveObject, pColorProp, color).AssertSuccess();
  }

  return newTrack;
}

xiiUuid xiiPropertyAnimAssetDocument::FindCurveCp(const xiiUuid& trackGuid, xiiInt64 iTickX)
{
  auto     pTrack = GetTrack(trackGuid);
  xiiInt32 iIndex = -1;
  for (xiiUInt32 i = 0; i < pTrack->m_FloatCurve.m_ControlPoints.GetCount(); i++)
  {
    if (pTrack->m_FloatCurve.m_ControlPoints[i].m_iTick == iTickX)
    {
      iIndex = (xiiInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return xiiUuid();

  const xiiAbstractProperty* pCurveProp         = xiiGetStaticRTTI<xiiPropertyAnimationTrack>()->FindPropertyByName("FloatCurve");
  const xiiDocumentObject*   trackObject        = GetObjectManager()->GetObject(trackGuid);
  xiiUuid                    curveGuid          = m_pObjectAccessor->Get<xiiUuid>(trackObject, pCurveProp);
  const xiiAbstractProperty* pControlPointsProp = xiiGetStaticRTTI<xiiSingleCurveData>()->FindPropertyByName("ControlPoints");
  const xiiDocumentObject*   curveObject        = GetObjectManager()->GetObject(curveGuid);
  xiiUuid                    cpGuid             = m_pObjectAccessor->Get<xiiUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

xiiUuid xiiPropertyAnimAssetDocument::InsertCurveCpAt(const xiiUuid& track, xiiInt64 iTickX, double fNewPosY)
{
  xiiObjectCommandAccessor accessor(GetCommandHistory());
  xiiObjectAccessorBase&   acc = accessor;
  acc.StartTransaction("Insert Control Point");

  const xiiDocumentObject* trackObject = GetObjectManager()->GetObject(track);
  const xiiVariant         curveGuid   = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  xiiUuid newObjectGuid;
  XII_VERIFY(acc.AddObject(accessor.GetObject(curveGuid.Get<xiiUuid>()), "ControlPoints", -1, xiiGetStaticRTTI<xiiCurveControlPointData>(), newObjectGuid)
               .Succeeded(),
             "");
  auto curveCPObj = accessor.GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValue(curveCPObj, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValue(curveCPObj, "Value", fNewPosY).Succeeded(), "");
  XII_VERIFY(acc.SetValue(curveCPObj, "LeftTangent", xiiVec2(-0.1f, 0.0f)).Succeeded(), "");
  XII_VERIFY(acc.SetValue(curveCPObj, "RightTangent", xiiVec2(+0.1f, 0.0f)).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

xiiUuid xiiPropertyAnimAssetDocument::FindGradientColorCp(const xiiUuid& trackGuid, xiiInt64 iTickX)
{
  auto     pTrack = GetTrack(trackGuid);
  xiiInt32 iIndex = -1;
  for (xiiUInt32 i = 0; i < pTrack->m_ColorGradient.m_ColorCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_ColorCPs[i].m_iTick == iTickX)
    {
      iIndex = (xiiInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return xiiUuid();

  const xiiAbstractProperty* pCurveProp         = xiiGetStaticRTTI<xiiPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const xiiDocumentObject*   trackObject        = GetObjectManager()->GetObject(trackGuid);
  xiiUuid                    curveGuid          = m_pObjectAccessor->Get<xiiUuid>(trackObject, pCurveProp);
  const xiiAbstractProperty* pControlPointsProp = xiiGetStaticRTTI<xiiColorGradientAssetData>()->FindPropertyByName("ColorCPs");
  const xiiDocumentObject*   curveObject        = GetObjectManager()->GetObject(curveGuid);
  xiiUuid                    cpGuid             = m_pObjectAccessor->Get<xiiUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

xiiUuid xiiPropertyAnimAssetDocument::InsertGradientColorCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, const xiiColorGammaUB& color)
{
  xiiObjectCommandAccessor accessor(GetCommandHistory());
  xiiObjectAccessorBase&   acc = accessor;

  const xiiDocumentObject* trackObject    = GetObjectManager()->GetObject(trackGuid);
  const xiiUuid            gradientGuid   = trackObject->GetTypeAccessor().GetValue("Gradient").Get<xiiUuid>();
  const xiiDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Color Control Point");
  xiiUuid newObjectGuid;
  XII_VERIFY(acc.AddObject(gradientObject, "ColorCPs", -1, xiiGetStaticRTTI<xiiColorControlPoint>(), newObjectGuid).Succeeded(), "");
  const xiiDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValue(cpObject, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValue(cpObject, "Red", color.r).Succeeded(), "");
  XII_VERIFY(acc.SetValue(cpObject, "Green", color.g).Succeeded(), "");
  XII_VERIFY(acc.SetValue(cpObject, "Blue", color.b).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

xiiUuid xiiPropertyAnimAssetDocument::FindGradientAlphaCp(const xiiUuid& trackGuid, xiiInt64 iTickX)
{
  auto     pTrack = GetTrack(trackGuid);
  xiiInt32 iIndex = -1;
  for (xiiUInt32 i = 0; i < pTrack->m_ColorGradient.m_AlphaCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_AlphaCPs[i].m_iTick == iTickX)
    {
      iIndex = (xiiInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return xiiUuid();

  const xiiAbstractProperty* pCurveProp         = xiiGetStaticRTTI<xiiPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const xiiDocumentObject*   trackObject        = GetObjectManager()->GetObject(trackGuid);
  xiiUuid                    curveGuid          = m_pObjectAccessor->Get<xiiUuid>(trackObject, pCurveProp);
  const xiiAbstractProperty* pControlPointsProp = xiiGetStaticRTTI<xiiColorGradientAssetData>()->FindPropertyByName("AlphaCPs");
  const xiiDocumentObject*   curveObject        = GetObjectManager()->GetObject(curveGuid);
  xiiUuid                    cpGuid             = m_pObjectAccessor->Get<xiiUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

xiiUuid xiiPropertyAnimAssetDocument::InsertGradientAlphaCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, xiiUInt8 uiAlpha)
{
  xiiObjectCommandAccessor accessor(GetCommandHistory());
  xiiObjectAccessorBase&   acc = accessor;

  const xiiDocumentObject* trackObject    = GetObjectManager()->GetObject(trackGuid);
  const xiiUuid            gradientGuid   = trackObject->GetTypeAccessor().GetValue("Gradient").Get<xiiUuid>();
  const xiiDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Alpha Control Point");
  xiiUuid newObjectGuid;
  XII_VERIFY(acc.AddObject(gradientObject, "AlphaCPs", -1, xiiGetStaticRTTI<xiiAlphaControlPoint>(), newObjectGuid).Succeeded(), "");
  const xiiDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValue(cpObject, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValue(cpObject, "Alpha", uiAlpha).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

xiiUuid xiiPropertyAnimAssetDocument::FindGradientIntensityCp(const xiiUuid& trackGuid, xiiInt64 iTickX)
{
  auto     pTrack = GetTrack(trackGuid);
  xiiInt32 iIndex = -1;
  for (xiiUInt32 i = 0; i < pTrack->m_ColorGradient.m_IntensityCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_IntensityCPs[i].m_iTick == iTickX)
    {
      iIndex = (xiiInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return xiiUuid();

  const xiiAbstractProperty* pCurveProp         = xiiGetStaticRTTI<xiiPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const xiiDocumentObject*   trackObject        = GetObjectManager()->GetObject(trackGuid);
  xiiUuid                    curveGuid          = m_pObjectAccessor->Get<xiiUuid>(trackObject, pCurveProp);
  const xiiAbstractProperty* pControlPointsProp = xiiGetStaticRTTI<xiiColorGradientAssetData>()->FindPropertyByName("IntensityCPs");
  const xiiDocumentObject*   curveObject        = GetObjectManager()->GetObject(curveGuid);
  xiiUuid                    cpGuid             = m_pObjectAccessor->Get<xiiUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

xiiUuid xiiPropertyAnimAssetDocument::InsertGradientIntensityCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, float fIntensity)
{
  xiiObjectCommandAccessor accessor(GetCommandHistory());
  xiiObjectAccessorBase&   acc = accessor;

  const xiiDocumentObject* trackObject    = GetObjectManager()->GetObject(trackGuid);
  const xiiUuid            gradientGuid   = trackObject->GetTypeAccessor().GetValue("Gradient").Get<xiiUuid>();
  const xiiDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Intensity Control Point");
  xiiUuid newObjectGuid;
  XII_VERIFY(acc.AddObject(gradientObject, "IntensityCPs", -1, xiiGetStaticRTTI<xiiIntensityControlPoint>(), newObjectGuid).Succeeded(), "");
  const xiiDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValue(cpObject, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValue(cpObject, "Intensity", fIntensity).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

xiiUuid xiiPropertyAnimAssetDocument::InsertEventTrackCpAt(xiiInt64 iTickX, const char* szValue)
{
  xiiObjectCommandAccessor accessor(GetCommandHistory());
  xiiObjectAccessorBase&   acc = accessor;
  acc.StartTransaction("Insert Event");

  const xiiAbstractProperty* pTrackProp = xiiGetStaticRTTI<xiiPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  xiiUuid                    trackGuid  = accessor.Get<xiiUuid>(GetPropertyObject(), pTrackProp);

  xiiUuid newObjectGuid;
  XII_VERIFY(
    acc.AddObject(accessor.GetObject(trackGuid), "ControlPoints", -1, xiiGetStaticRTTI<xiiEventTrackControlPointData>(), newObjectGuid).Succeeded(),
    "");
  const xiiDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValue(pCPObj, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValue(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}
