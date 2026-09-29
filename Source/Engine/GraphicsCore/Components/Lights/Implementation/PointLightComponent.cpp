/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

namespace
{
  float GetPointEmitterProjectedArea(float fRadius, float fLength)
  {
    const float fSafeRadius = xiiMath::Max(fRadius, 0.0f);
    return xiiMath::Pi<float>() * fSafeRadius * fSafeRadius + 2.0f * fSafeRadius * xiiMath::Max(fLength, 0.0f);
  }
} // namespace

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPointLightRenderData, 1, xiiRTTIDefaultAllocator<xiiPointLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiPointLightComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiClampValueAttribute(0.0f, 0.5f), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
    XII_ACCESSOR_PROPERTY("ShadowFadeOutRange", GetShadowFadeOutRange, SetShadowFadeOutRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
    XII_RESOURCE_ACCESSOR_PROPERTY("IESProfile", GetIESProfile, SetIESProfile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_IESProfile", xiiDependencyFlags::Package)),
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
  s << m_hIESProfile;
}

void xiiPointLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fLength;
  s >> m_fRadius;
  s >> m_fRange;
  s >> m_fShadowFadeOutRange;

  if (inout_stream.GetComponentTypeVersion(GetStaticRTTI()) >= 2U)
    s >> m_hIESProfile;
}

xiiResult xiiPointLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  const float fCandela = GetLuminousIntensity(4.0f * xiiMath::Pi<float>(), GetPointEmitterProjectedArea(m_fRadius, m_fLength));
  m_fEffectiveRange    = CalculateEffectiveRange(m_fRange, fCandela);

  const float fBoundingRadius = m_fEffectiveRange + m_fLength * 0.5f;
  ref_bounds                  = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), fBoundingRadius);

  return XII_SUCCESS;
}

void xiiPointLightComponent::SetRange(float fRange)
{
  m_fRange = xiiMath::Max(fRange, 0.0f);

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
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
  InvalidateCachedRenderData();
}

float xiiPointLightComponent::GetLength() const
{
  return m_fLength;
}

void xiiPointLightComponent::SetRadius(float fRadius)
{
  m_fRadius = xiiMath::Max(fRadius, 0.0f);

  TriggerLocalBoundsUpdate();
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

void xiiPointLightComponent::SetIESProfile(const xiiIESProfileResourceHandle& hProfile)
{
  if (m_hIESProfile != hProfile)
  {
    m_hIESProfile = hProfile;
    InvalidateCachedRenderData();
  }
}

const xiiIESProfileResourceHandle& xiiPointLightComponent::GetIESProfile() const
{
  return m_hIESProfile;
}

void xiiPointLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;
  const float fCandela = GetLuminousIntensity(4.0f * xiiMath::Pi<float>(), GetPointEmitterProjectedArea(m_fRadius, m_fLength));
  if (fCandela <= 0.0f)
    return;

  auto                     pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  xiiPointLightRenderData* pRenderData  = pWorldModule->CreateRenderDataForThisFrame<xiiPointLightRenderData>(this);
  pRenderData->m_LightColor             = m_LightColor;
  pRenderData->m_uiTemperature          = m_uiTemperature;
  pRenderData->m_fPhotometricIntensity  = fCandela;
  pRenderData->m_bCastShadows           = m_bCastShadows;
  pRenderData->m_fRange                 = CalculateEffectiveRange(m_fRange, fCandela);
  pRenderData->m_fRadius                = m_fRadius;
  pRenderData->m_fLength                = m_fLength;
  pRenderData->m_fShadowFadeOutRange    = m_fShadowFadeOutRange;
  pRenderData->m_qGlobalRotation        = GetOwner()->GetGlobalRotation();
  pRenderData->m_hIESProfile            = m_hIESProfile;
  pRenderData->m_uiSortingKey           = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, m_bCastShadows ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never);
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
