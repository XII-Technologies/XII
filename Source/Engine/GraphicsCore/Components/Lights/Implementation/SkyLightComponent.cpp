/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/SkyLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkyLightRenderData, 1, xiiRTTIDefaultAllocator<xiiSkyLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSkyLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("EnvironmentMap", GetEnvironmentMap, SetEnvironmentMap)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Cube", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new xiiDefaultValueAttribute(0)),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Graphics/Lighting/Reflections"), } XII_END_ATTRIBUTES;
} XII_END_COMPONENT_TYPE
// clang-format on

xiiSkyLightComponent::xiiSkyLightComponent()  = default;
xiiSkyLightComponent::~xiiSkyLightComponent() = default;

void xiiSkyLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& stream = inout_stream.GetStream();
  stream << m_hEnvironmentMap;
  stream << m_fIntensity;
  stream << m_fSaturation;
  stream << m_iPriority;
}

void xiiSkyLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& stream = inout_stream.GetStream();
  stream >> m_hEnvironmentMap;
  stream >> m_fIntensity;
  stream >> m_fSaturation;
  stream >> m_iPriority;
}

xiiResult xiiSkyLightComponent::GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(out_bounds);
  XII_IGNORE_UNUSED(ref_msg);
  out_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiSkyLightComponent::SetEnvironmentMap(const xiiTextureCubeResourceHandle& hEnvironmentMap)
{
  if (m_hEnvironmentMap == hEnvironmentMap)
    return;
  m_hEnvironmentMap = hEnvironmentMap;
  InvalidateCachedRenderData();
}

void xiiSkyLightComponent::SetIntensity(float fIntensity)
{
  fIntensity = xiiMath::Max(fIntensity, 0.0f);
  if (m_fIntensity == fIntensity)
    return;
  m_fIntensity = fIntensity;
  InvalidateCachedRenderData();
}

void xiiSkyLightComponent::SetSaturation(float fSaturation)
{
  fSaturation = xiiMath::Saturate(fSaturation);
  if (m_fSaturation == fSaturation)
    return;
  m_fSaturation = fSaturation;
  InvalidateCachedRenderData();
}

void xiiSkyLightComponent::SetPriority(xiiInt32 iPriority)
{
  if (m_iPriority == iPriority)
    return;
  m_iPriority = iPriority;
  InvalidateCachedRenderData();
}

void xiiSkyLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!ref_msg.m_pView || !ref_msg.m_pExtractedRenderData || !m_hEnvironmentMap.IsValid() || m_fIntensity <= 0)
    return;
  const auto* renderer = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!renderer)
    return;
  auto* packet              = renderer->CreateRenderDataForThisFrame<xiiSkyLightRenderData>(this);
  packet->m_hEnvironmentMap = m_hEnvironmentMap;
  packet->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  packet->m_fIntensity      = m_fIntensity;
  packet->m_fSaturation     = m_fSaturation;
  packet->m_iPriority       = m_iPriority;
  packet->m_uiSortingKey    = GetUniqueIdForRendering();
  ref_msg.AddRenderData(packet, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_SkyLightComponent);
