/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/SphereAreaLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSphereAreaLightRenderData, 1, xiiRTTIDefaultAllocator<xiiSphereAreaLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSphereAreaLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiDefaultValueAttribute(0.25f), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
    XII_ACCESSOR_PROPERTY("ShadowFadeOutRange", GetShadowFadeOutRange, SetShadowFadeOutRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSphereManipulatorAttribute("Radius"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSphereAreaLightComponent::xiiSphereAreaLightComponent()
{
  m_IntensityUnit = xiiPhotometricUnit::Nit;
}

xiiSphereAreaLightComponent::~xiiSphereAreaLightComponent() = default;

void xiiSphereAreaLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& stream = inout_stream.GetStream();
  stream << m_fRadius;
  stream << m_fRange;
  stream << m_fShadowFadeOutRange;
}

void xiiSphereAreaLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& stream = inout_stream.GetStream();
  stream >> m_fRadius;
  stream >> m_fRange;
  stream >> m_fShadowFadeOutRange;
}

xiiResult xiiSphereAreaLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);

  m_fEffectiveRange  = CalculateEffectiveRange(m_fRange, GetOnAxisCandela());
  ref_bounds         = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fEffectiveRange + m_fRadius);
  ref_bAlwaysVisible = false;
  return XII_SUCCESS;
}

void xiiSphereAreaLightComponent::SetRadius(float fRadius)
{
  fRadius = xiiMath::Max(fRadius, 0.001f);
  if (m_fRadius == fRadius)
    return;

  m_fRadius = fRadius;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

float xiiSphereAreaLightComponent::GetRadius() const
{
  return m_fRadius;
}

void xiiSphereAreaLightComponent::SetRange(float fRange)
{
  fRange = xiiMath::Max(fRange, 0.0f);
  if (m_fRange == fRange)
    return;

  m_fRange = fRange;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

float xiiSphereAreaLightComponent::GetRange() const
{
  return m_fRange;
}

float xiiSphereAreaLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void xiiSphereAreaLightComponent::SetShadowFadeOutRange(float fRange)
{
  fRange = xiiMath::Max(fRange, 0.0f);
  if (m_fShadowFadeOutRange == fRange)
    return;

  m_fShadowFadeOutRange = fRange;
  InvalidateCachedRenderData();
}

float xiiSphereAreaLightComponent::GetShadowFadeOutRange() const
{
  return m_fShadowFadeOutRange;
}

float xiiSphereAreaLightComponent::GetLuminanceNits() const
{
  const float fEmittingArea  = 4.0f * xiiMath::Pi<float>() * m_fRadius * m_fRadius;
  const float fProjectedArea = xiiMath::Pi<float>() * m_fRadius * m_fRadius;
  return GetLuminance(fEmittingArea, fProjectedArea);
}

float xiiSphereAreaLightComponent::GetOnAxisCandela() const
{
  const float fProjectedArea = xiiMath::Pi<float>() * m_fRadius * m_fRadius;
  return xiiPhotometricUtils::LuminanceToLuminousIntensity(GetLuminanceNits(), fProjectedArea);
}

void xiiSphereAreaLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  const float fNits           = GetLuminanceNits();
  const float fEffectiveRange = CalculateEffectiveRange(m_fRange, GetOnAxisCandela());
  if (fNits <= 0.0f || fEffectiveRange <= 0.0f)
    return;

  auto                          pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  xiiSphereAreaLightRenderData* pRenderData  = pWorldModule->CreateRenderDataForThisFrame<xiiSphereAreaLightRenderData>(this);
  pRenderData->m_LightColor                  = m_LightColor;
  pRenderData->m_uiTemperature               = m_uiTemperature;
  pRenderData->m_fPhotometricIntensity       = fNits;
  pRenderData->m_bCastShadows                = m_bCastShadows;
  pRenderData->m_fRadius                     = m_fRadius;
  pRenderData->m_fRange                      = fEffectiveRange;
  pRenderData->m_fShadowFadeOutRange         = m_fShadowFadeOutRange;
  pRenderData->m_uiSortingKey                = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, m_bCastShadows ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_SphereAreaLightComponent);
