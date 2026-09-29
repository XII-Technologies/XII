/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/TubeAreaLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTubeAreaLightRenderData, 1, xiiRTTIDefaultAllocator<xiiTubeAreaLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiTubeAreaLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiDefaultValueAttribute(1.0f), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiDefaultValueAttribute(0.05f), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
    XII_ACCESSOR_PROPERTY("ShadowFadeOutRange", GetShadowFadeOutRange, SetShadowFadeOutRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiTubeAreaLightComponent::xiiTubeAreaLightComponent()
{
  m_IntensityUnit = xiiPhotometricUnit::Nit;
}

xiiTubeAreaLightComponent::~xiiTubeAreaLightComponent() = default;

void xiiTubeAreaLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& stream = inout_stream.GetStream();
  stream << m_fLength;
  stream << m_fRadius;
  stream << m_fRange;
  stream << m_fShadowFadeOutRange;
}

void xiiTubeAreaLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& stream = inout_stream.GetStream();
  stream >> m_fLength;
  stream >> m_fRadius;
  stream >> m_fRange;
  stream >> m_fShadowFadeOutRange;
}

xiiResult xiiTubeAreaLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);

  m_fEffectiveRange  = CalculateEffectiveRange(m_fRange, GetMaximumCandela());
  ref_bounds         = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fEffectiveRange + m_fLength * 0.5f + m_fRadius);
  ref_bAlwaysVisible = false;
  return XII_SUCCESS;
}

void xiiTubeAreaLightComponent::SetLength(float fLength)
{
  fLength = xiiMath::Max(fLength, 0.001f);
  if (m_fLength == fLength)
    return;

  m_fLength = fLength;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

float xiiTubeAreaLightComponent::GetLength() const
{
  return m_fLength;
}

void xiiTubeAreaLightComponent::SetRadius(float fRadius)
{
  fRadius = xiiMath::Max(fRadius, 0.001f);
  if (m_fRadius == fRadius)
    return;

  m_fRadius = fRadius;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

float xiiTubeAreaLightComponent::GetRadius() const
{
  return m_fRadius;
}

void xiiTubeAreaLightComponent::SetRange(float fRange)
{
  fRange = xiiMath::Max(fRange, 0.0f);
  if (m_fRange == fRange)
    return;

  m_fRange = fRange;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

float xiiTubeAreaLightComponent::GetRange() const
{
  return m_fRange;
}

float xiiTubeAreaLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void xiiTubeAreaLightComponent::SetShadowFadeOutRange(float fRange)
{
  fRange = xiiMath::Max(fRange, 0.0f);
  if (m_fShadowFadeOutRange == fRange)
    return;

  m_fShadowFadeOutRange = fRange;
  InvalidateCachedRenderData();
}

float xiiTubeAreaLightComponent::GetShadowFadeOutRange() const
{
  return m_fShadowFadeOutRange;
}

float xiiTubeAreaLightComponent::GetEmittingArea() const
{
  return 2.0f * xiiMath::Pi<float>() * m_fRadius * m_fLength + 4.0f * xiiMath::Pi<float>() * m_fRadius * m_fRadius;
}

float xiiTubeAreaLightComponent::GetMaximumProjectedArea() const
{
  return 2.0f * m_fRadius * m_fLength + xiiMath::Pi<float>() * m_fRadius * m_fRadius;
}

float xiiTubeAreaLightComponent::GetLuminanceNits() const
{
  return GetLuminance(GetEmittingArea(), GetMaximumProjectedArea());
}

float xiiTubeAreaLightComponent::GetMaximumCandela() const
{
  return xiiPhotometricUtils::LuminanceToLuminousIntensity(GetLuminanceNits(), GetMaximumProjectedArea());
}

void xiiTubeAreaLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  const float fNits           = GetLuminanceNits();
  const float fEffectiveRange = CalculateEffectiveRange(m_fRange, GetMaximumCandela());
  if (fNits <= 0.0f || fEffectiveRange <= 0.0f)
    return;

  auto                        pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  xiiTubeAreaLightRenderData* pRenderData  = pWorldModule->CreateRenderDataForThisFrame<xiiTubeAreaLightRenderData>(this);
  pRenderData->m_LightColor                = m_LightColor;
  pRenderData->m_uiTemperature             = m_uiTemperature;
  pRenderData->m_fPhotometricIntensity     = fNits;
  pRenderData->m_bCastShadows              = m_bCastShadows;
  pRenderData->m_fRadius                   = m_fRadius;
  pRenderData->m_fRange                    = fEffectiveRange;
  pRenderData->m_fLength                   = m_fLength;
  pRenderData->m_fShadowFadeOutRange       = m_fShadowFadeOutRange;
  pRenderData->m_qGlobalRotation           = GetOwner()->GetGlobalRotation();
  pRenderData->m_uiSortingKey              = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, m_bCastShadows ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_TubeAreaLightComponent);
