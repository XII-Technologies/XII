/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/DiscAreaLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDiscAreaLightRenderData, 1, xiiRTTIDefaultAllocator<xiiDiscAreaLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDiscAreaLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiDefaultValueAttribute(0.5f), new xiiSuffixAttribute(" m")),
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
    new xiiDiscAreaLightVisualizerAttribute("Radius", "Range", "Intensity", "LightColor"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiDiscAreaLightComponent::xiiDiscAreaLightComponent()  = default;
xiiDiscAreaLightComponent::~xiiDiscAreaLightComponent() = default;

void xiiDiscAreaLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fRange;
  s << m_fShadowFadeOutRange;
}

void xiiDiscAreaLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fRange;
  s >> m_fShadowFadeOutRange;
}

xiiResult xiiDiscAreaLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);

  m_fEffectiveRange  = CalculateEffectiveRange(m_fRange, m_fIntensity);
  ref_bounds         = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fEffectiveRange + m_fRadius);
  ref_bAlwaysVisible = false;

  return XII_SUCCESS;
}

void xiiDiscAreaLightComponent::SetRadius(float fRadius)
{
  m_fRadius = xiiMath::Max(fRadius, 0.001f);

  TriggerLocalBoundsUpdate();
}

float xiiDiscAreaLightComponent::GetRadius() const
{
  return m_fRadius;
}

void xiiDiscAreaLightComponent::SetRange(float fRange)
{
  m_fRange = xiiMath::Max(fRange, 0.0f);

  TriggerLocalBoundsUpdate();
}

float xiiDiscAreaLightComponent::GetRange() const
{
  return m_fRange;
}

float xiiDiscAreaLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void xiiDiscAreaLightComponent::SetShadowFadeOutRange(float fRange)
{
  m_fShadowFadeOutRange = xiiMath::Max(fRange, 0.0f);

  InvalidateCachedRenderData();
}

float xiiDiscAreaLightComponent::GetShadowFadeOutRange() const
{
  return m_fShadowFadeOutRange;
}

void xiiDiscAreaLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  const float fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
  if (m_fIntensity <= 0.0f || fEffectiveRange <= 0.0f || m_fRadius <= 0.0f)
    return;

  auto                        pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  xiiDiscAreaLightRenderData* pRenderData  = pWorldModule->CreateRenderDataForThisFrame<xiiDiscAreaLightRenderData>(this);
  pRenderData->m_LightColor                = m_LightColor;
  pRenderData->m_uiTemperature             = m_uiTemperature;
  pRenderData->m_fIntensity                = m_fIntensity;
  pRenderData->m_bCastShadows              = m_bCastShadows;
  pRenderData->m_fRadius                   = m_fRadius;
  pRenderData->m_fRange                    = fEffectiveRange;
  pRenderData->m_fShadowFadeOutRange       = m_fShadowFadeOutRange;
  pRenderData->m_qGlobalRotation           = GetOwner()->GetGlobalRotation();
  pRenderData->m_uiSortingKey              = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, m_bCastShadows ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDiscAreaLightVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiDiscAreaLightVisualizerAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDiscAreaLightVisualizerAttribute::xiiDiscAreaLightVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiDiscAreaLightVisualizerAttribute::xiiDiscAreaLightVisualizerAttribute(xiiStringView sRadiusProperty, xiiStringView sRangeProperty, xiiStringView sIntensityProperty, xiiStringView sColorProperty) :
  xiiVisualizerAttribute(sRadiusProperty, sRangeProperty, sIntensityProperty, sColorProperty)
{
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_DiscAreaLightComponent);
