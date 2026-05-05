/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

namespace
{
  constexpr xiiAngle c_MaxSpotAngle = xiiAngle::MakeFromDegree(160.0f);
} // namespace

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpotLightRenderData, 1, xiiRTTIDefaultAllocator<xiiSpotLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSpotLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(0.0f), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiClampValueAttribute(0.0f, 0.5f), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("InnerSpotAngle", GetInnerSpotAngle, SetInnerSpotAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::MakeZero(), c_MaxSpotAngle), new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(15.0f))),
    XII_ACCESSOR_PROPERTY("OuterSpotAngle", GetOuterSpotAngle, SetOuterSpotAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::MakeZero(), c_MaxSpotAngle), new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(30.0f))),
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
    new xiiSpotLightVisualizerAttribute("OuterSpotAngle", "Range", "Intensity", "LightColor", "Radius"),
    new xiiConeLengthManipulatorAttribute("Range"),
    new xiiConeAngleManipulatorAttribute("OuterSpotAngle", 1.5f),
    new xiiConeAngleManipulatorAttribute("InnerSpotAngle", 1.5f),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSpotLightComponent::xiiSpotLightComponent()  = default;
xiiSpotLightComponent::~xiiSpotLightComponent() = default;

void xiiSpotLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRange;
  s << m_fEffectiveRange;
  s << m_fShadowFadeOutRange;
  s << m_fRadius;
  s << m_InnerSpotAngle;
  s << m_OuterSpotAngle;
}

void xiiSpotLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fRange;
  s >> m_fEffectiveRange;
  s >> m_fShadowFadeOutRange;
  s >> m_fRadius;
  s >> m_InnerSpotAngle;
  s >> m_OuterSpotAngle;
}

xiiResult xiiSpotLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);

  m_fEffectiveRange  = CalculateEffectiveRange(m_fRange, m_fIntensity);
  ref_bounds         = CalculateBoundingSphere(xiiTransform::MakeIdentity(), m_fEffectiveRange);
  ref_bAlwaysVisible = false;

  return XII_SUCCESS;
}

void xiiSpotLightComponent::SetRange(float fRange)
{
  m_fRange = xiiMath::Max(fRange, 0.0f);

  TriggerLocalBoundsUpdate();
}

float xiiSpotLightComponent::GetRange() const
{
  return m_fRange;
}

float xiiSpotLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void xiiSpotLightComponent::SetRadius(float fRadius)
{
  m_fRadius = xiiMath::Max(fRadius, 0.0f);

  InvalidateCachedRenderData();
}

float xiiSpotLightComponent::GetRadius() const
{
  return m_fRadius;
}

void xiiSpotLightComponent::SetShadowFadeOutRange(float fRange)
{
  m_fShadowFadeOutRange = xiiMath::Max(fRange, 0.0f);

  InvalidateCachedRenderData();
}

float xiiSpotLightComponent::GetShadowFadeOutRange() const
{
  return m_fShadowFadeOutRange;
}

void xiiSpotLightComponent::SetInnerSpotAngle(xiiAngle spotAngle)
{
  m_InnerSpotAngle = xiiMath::Clamp(spotAngle, xiiAngle::MakeZero(), c_MaxSpotAngle);

  InvalidateCachedRenderData();
}

xiiAngle xiiSpotLightComponent::GetInnerSpotAngle() const
{
  return m_InnerSpotAngle;
}

void xiiSpotLightComponent::SetOuterSpotAngle(xiiAngle spotAngle)
{
  m_OuterSpotAngle = xiiMath::Clamp(spotAngle, xiiAngle::MakeZero(), c_MaxSpotAngle);

  TriggerLocalBoundsUpdate();
}

xiiAngle xiiSpotLightComponent::GetOuterSpotAngle() const
{
  return m_OuterSpotAngle;
}

void xiiSpotLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  const float fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  if (m_fIntensity <= 0.0f || fEffectiveRange <= 0.0f || m_OuterSpotAngle.GetRadian() <= 0.0f)
    return;

  auto                    pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  xiiSpotLightRenderData* pRenderData  = pWorldModule->CreateRenderDataForThisFrame<xiiSpotLightRenderData>(this);
  pRenderData->m_LightColor            = GetLightColor();
  pRenderData->m_fIntensity            = GetIntensity();
  pRenderData->m_uiTemperature         = GetTemperature();
  pRenderData->m_fRange                = fEffectiveRange;
  pRenderData->m_fRadius               = m_fRadius;
  pRenderData->m_fShadowFadeOutRange   = m_fShadowFadeOutRange;
  pRenderData->m_bCastShadows          = m_bCastShadows;
  pRenderData->m_qGlobalRotation       = GetOwner()->GetGlobalRotation();
  pRenderData->m_InnerSpotAngle        = m_InnerSpotAngle;
  pRenderData->m_OuterSpotAngle        = m_OuterSpotAngle;
  pRenderData->m_uiSortingKey          = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, m_bCastShadows ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never);
}

xiiBoundingSphere xiiSpotLightComponent::CalculateBoundingSphere(const xiiTransform& transform, float fRange) const
{
  xiiBoundingSphere boundingSphere;
  xiiAngle          halfAngle         = m_OuterSpotAngle / 2.0f;
  xiiVec3           vPosition         = transform.m_vPosition;
  xiiVec3           vForwardDirection = transform.m_qRotation * xiiVec3(1.0f, 0.0f, 0.0f);

  if (halfAngle > xiiAngle::MakeFromDegree(45.0f))
  {
    boundingSphere.m_vCenter = vPosition + xiiMath::Cos(halfAngle) * fRange * vForwardDirection;
    boundingSphere.m_fRadius = xiiMath::Sin(halfAngle) * fRange;
  }
  else
  {
    boundingSphere.m_fRadius = fRange / (2.0f * xiiMath::Cos(halfAngle));
    boundingSphere.m_vCenter = vPosition + vForwardDirection * boundingSphere.m_fRadius;
  }

  return boundingSphere;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpotLightVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiSpotLightVisualizerAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSpotLightVisualizerAttribute::xiiSpotLightVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiSpotLightVisualizerAttribute::xiiSpotLightVisualizerAttribute(xiiStringView sAngleProperty, xiiStringView sRangeProperty, xiiStringView sIntensityProperty, xiiStringView sColorProperty, xiiStringView sRadiusProperty) :
  xiiVisualizerAttribute(sAngleProperty, sRangeProperty, sIntensityProperty, sColorProperty, sRadiusProperty)
{
}
