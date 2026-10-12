/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAnimAssetDocument, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiPropertyAnimAssetDocument::xiiPropertyAnimAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiPropertyAnimationTrackGroup>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiPropertyAnimAssetDocument::~xiiPropertyAnimAssetDocument() = default;

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
    const xiiUInt32 uiTicksPerFrame = 4800 / xiiMath::Clamp(GetProperties()->m_uiFramesPerSecond, 1U, 4800U);
    uiTick                          = (xiiUInt64)xiiMath::RoundToMultiple((double)uiTick, (double)uiTicksPerFrame);
  }
  uiTick = xiiMath::Clamp<xiiUInt64>(uiTick, 0, GetAnimationDurationTicks());

  if (m_uiScrubberTickPos == uiTick)
    return false;

  m_uiScrubberTickPos = uiTick;

  xiiPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged;
  m_PropertyAnimEvents.Broadcast(e);

  return true;
}

xiiTransformStatus xiiPropertyAnimAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  return xiiTransformStatus("Property animation is a source-only document. The Engine has no property-animation runtime resource.");
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
  const xiiTime   tDiff       = (currentTime - m_LastFrameTime);
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

xiiUuid xiiPropertyAnimAssetDocument::InsertCurveCpAt(const xiiUuid& track, xiiInt64 iTickX, double fNewPosY)
{
  xiiObjectCommandAccessor accessor(GetCommandHistory());
  xiiObjectAccessorBase&   acc = accessor;
  acc.StartTransaction("Insert Control Point");

  const xiiDocumentObject* trackObject = GetObjectManager()->GetObject(track);
  const xiiVariant         curveGuid   = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  xiiUuid newObjectGuid;
  XII_VERIFY(acc.AddObjectByName(accessor.GetObject(curveGuid.Get<xiiUuid>()), "ControlPoints", -1, xiiGetStaticRTTI<xiiCurveControlPointData>(), newObjectGuid).Succeeded(), "");

  auto curveCPObj = accessor.GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValueByName(curveCPObj, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(curveCPObj, "Value", fNewPosY).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(curveCPObj, "LeftTangent", xiiVec2(-0.1f, 0.0f)).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(curveCPObj, "RightTangent", xiiVec2(+0.1f, 0.0f)).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
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
  XII_VERIFY(acc.AddObjectByName(gradientObject, "ColorCPs", -1, xiiGetStaticRTTI<xiiColorControlPoint>(), newObjectGuid).Succeeded(), "");
  const xiiDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(cpObject, "Red", color.r).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(cpObject, "Green", color.g).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(cpObject, "Blue", color.b).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
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
  XII_VERIFY(acc.AddObjectByName(gradientObject, "AlphaCPs", -1, xiiGetStaticRTTI<xiiAlphaControlPoint>(), newObjectGuid).Succeeded(), "");
  const xiiDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(cpObject, "Alpha", uiAlpha).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
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
  XII_VERIFY(acc.AddObjectByName(gradientObject, "IntensityCPs", -1, xiiGetStaticRTTI<xiiIntensityControlPoint>(), newObjectGuid).Succeeded(), "");
  const xiiDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValueByName(cpObject, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(cpObject, "Intensity", fIntensity).Succeeded(), "");
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
  XII_VERIFY(acc.AddObjectByName(accessor.GetObject(trackGuid), "ControlPoints", -1, xiiGetStaticRTTI<xiiEventTrackControlPointData>(), newObjectGuid).Succeeded(), "");

  const xiiDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValueByName(pCPObj, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}
