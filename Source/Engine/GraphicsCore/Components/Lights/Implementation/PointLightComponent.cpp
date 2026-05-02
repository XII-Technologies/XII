/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPointLightRenderData, 1, xiiRTTIDefaultAllocator<xiiPointLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiPointLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiClampValueAttribute(0.0f, 0.5f), new xiiSuffixAttribute(" m")),
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
    new xiiSphereManipulatorAttribute("Range"),
    new xiiPointLightVisualizerAttribute("Length", "Radius", "Range", "Intensity", "LightColor"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiPointLightComponent::xiiPointLightComponent()  = default;
xiiPointLightComponent::~xiiPointLightComponent() = default;

void xiiPointLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fLength;
  s << m_fRadius;
  s << m_fRange;
  s << m_fShadowFadeOutRange;
}

void xiiPointLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fLength;
  s >> m_fRadius;
  s >> m_fRange;
  s >> m_fShadowFadeOutRange;
}

xiiResult xiiPointLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  const float fBoundingRadius = m_fEffectiveRange + m_fLength * 0.5f;
  ref_bounds                  = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), fBoundingRadius);

  return XII_SUCCESS;
}

void xiiPointLightComponent::SetRange(float fRange)
{
  m_fRange = xiiMath::Max(fRange, 0.0f);

  TriggerLocalBoundsUpdate();
}

float xiiPointLightComponent::GetRange() const
{
  return m_fRange;
}

float xiiPointLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void xiiPointLightComponent::SetLength(float fLength)
{
  m_fLength = xiiMath::Max(fLength, 0.0f);

  TriggerLocalBoundsUpdate();
}

float xiiPointLightComponent::GetLength() const
{
  return m_fLength;
}

void xiiPointLightComponent::SetRadius(float fRadius)
{
  m_fRadius = xiiMath::Max(fRadius, 0.0f);

  InvalidateCachedRenderData();
}

float xiiPointLightComponent::GetRadius() const
{
  return m_fRadius;
}

void xiiPointLightComponent::SetShadowFadeOutRange(float fRange)
{
  m_fShadowFadeOutRange = xiiMath::Max(fRange, 0.0f);

  InvalidateCachedRenderData();
}

float xiiPointLightComponent::GetShadowFadeOutRange() const
{
  return m_fShadowFadeOutRange;
}

void xiiPointLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;
  if (m_fIntensity <= 0.0f)
    return;

  auto                     pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  xiiPointLightRenderData* pRenderData  = pWorldModule->CreateRenderDataForThisFrame<xiiPointLightRenderData>(this);
  pRenderData->m_LightColor             = m_LightColor;
  pRenderData->m_uiTemperature          = m_uiTemperature;
  pRenderData->m_fIntensity             = m_fIntensity;
  pRenderData->m_bCastShadows           = m_bCastShadows;
  pRenderData->m_fRadius                = GetEffectiveRange();
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPointLightVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiPointLightVisualizerAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiPointLightVisualizerAttribute::xiiPointLightVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiPointLightVisualizerAttribute::xiiPointLightVisualizerAttribute(xiiStringView sLengthProperty, xiiStringView sRadiusProperty, xiiStringView sRangeProperty, xiiStringView sIntensityProperty, xiiStringView sColorProperty) :
  xiiVisualizerAttribute(sLengthProperty, sRadiusProperty, sRangeProperty, sIntensityProperty, sColorProperty)
{
}
