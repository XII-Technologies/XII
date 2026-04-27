#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPointLightRenderData, 1, xiiRTTIDefaultAllocator<xiiPointLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiPointLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("AttenuationRadius", GetAttenuationRadius, SetAttenuationRadius)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.01f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("FalloffExponent",   GetFalloffExponent,   SetFalloffExponent)->AddAttributes(new xiiDefaultValueAttribute(2.0f), new xiiClampValueAttribute(0.1f, 8.0f)),
    XII_ACCESSOR_PROPERTY("VolumetricIntensity", GetVolumetricIntensity, SetVolumetricIntensity)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lights"),
    new xiiSphereVisualizerAttribute("AttenuationRadius", nullptr, xiiVisualizerAnchor::Center, xiiVec3(0), "LightColor"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiPointLightComponent::xiiPointLightComponent()  = default;
xiiPointLightComponent::~xiiPointLightComponent() = default;

void xiiPointLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_fAttenuationRadius << m_fFalloffExponent << m_fVolumetricIntensity;
}

void xiiPointLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_fAttenuationRadius >> m_fFalloffExponent >> m_fVolumetricIntensity;
}

xiiResult xiiPointLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);

  ref_bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fAttenuationRadius));
  return XII_SUCCESS;
}

void xiiPointLightComponent::SetAttenuationRadius(float fRadius)
{
  m_fAttenuationRadius = xiiMath::Max(fRadius, 0.01f);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiPointLightComponent::SetFalloffExponent(float fExp)
{
  m_fFalloffExponent = xiiMath::Clamp(fExp, 0.1f, 8.0f);
  InvalidateCachedRenderData();
}

void xiiPointLightComponent::SetVolumetricIntensity(float f)
{
  m_fVolumetricIntensity = xiiMath::Max(f, 0.0f);
  InvalidateCachedRenderData();
}

void xiiPointLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (m_fIntensity <= 0.0f || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiPointLightRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiPointLightRenderData>(this);
  pRenderData->m_GlobalTransform       = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds          = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject          = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent       = GetHandle();
  pRenderData->m_LightColor            = m_LightColor;
  pRenderData->m_uiTemperature         = m_uiTemperature;
  pRenderData->m_fIntensity            = m_fIntensity;
  pRenderData->m_fRadius               = m_fAttenuationRadius;
  pRenderData->m_bCastShadows          = m_bCastShadows;
  pRenderData->m_fAttenuationRadius    = m_fAttenuationRadius;
  pRenderData->m_fFalloffExponent      = m_fFalloffExponent;
  pRenderData->m_fVolumetricIntensity  = m_fVolumetricIntensity;
  pRenderData->m_uiSortingKey          = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_PointLightComponent);
