/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Fog/VolumetricCloudComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVolumetricCloudSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVolumetricCloudSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LayerAltitude", m_fLayerAltitudeMeters)->AddAttributes(new xiiDefaultValueAttribute(2000.0f), new xiiClampValueAttribute(0.0f, 20000.0f), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("ShadowScale", m_fShadowScaleMeters)->AddAttributes(new xiiDefaultValueAttribute(4000.0f), new xiiClampValueAttribute(10.0f, 100000.0f), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("DetailScale", m_fDetailScale)->AddAttributes(new xiiDefaultValueAttribute(4.0f), new xiiClampValueAttribute(1.0f, 16.0f)),
    XII_MEMBER_PROPERTY("Coverage", m_fCoverage)->AddAttributes(new xiiDefaultValueAttribute(0.55f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("OpticalDepth", m_fOpticalDepth)->AddAttributes(new xiiDefaultValueAttribute(2.0f), new xiiClampValueAttribute(0.0f, 20.0f)),
    XII_MEMBER_PROPERTY("ShadowStrength", m_fShadowStrength)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("WindVelocity", m_vWindVelocityMetersPerSecond)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(12.0f, 4.0f)), new xiiSuffixAttribute(" m/s")),
    XII_MEMBER_PROPERTY("CastShadows", m_bCastShadows)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVolumetricCloudRenderData, 1, xiiRTTIDefaultAllocator<xiiVolumetricCloudRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiVolumetricCloudComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Cloud", GetCloudSettings, SetCloudSettings),
    XII_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lighting"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiVolumetricCloudComponent::xiiVolumetricCloudComponent()  = default;
xiiVolumetricCloudComponent::~xiiVolumetricCloudComponent() = default;

void xiiVolumetricCloudComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& stream = inout_stream.GetStream();

  stream << m_Settings.m_fLayerAltitudeMeters;
  stream << m_Settings.m_fShadowScaleMeters;
  stream << m_Settings.m_fDetailScale;
  stream << m_Settings.m_fCoverage;
  stream << m_Settings.m_fOpticalDepth;
  stream << m_Settings.m_fShadowStrength;
  stream << m_Settings.m_vWindVelocityMetersPerSecond;
  stream << m_Settings.m_bCastShadows;
  stream << m_iPriority;
}

void xiiVolumetricCloudComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& stream = inout_stream.GetStream();

  stream >> m_Settings.m_fLayerAltitudeMeters;
  stream >> m_Settings.m_fShadowScaleMeters;
  stream >> m_Settings.m_fDetailScale;
  stream >> m_Settings.m_fCoverage;
  stream >> m_Settings.m_fOpticalDepth;
  stream >> m_Settings.m_fShadowStrength;
  stream >> m_Settings.m_vWindVelocityMetersPerSecond;
  stream >> m_Settings.m_bCastShadows;
  stream >> m_iPriority;
}

xiiResult xiiVolumetricCloudComponent::GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(out_bounds);
  XII_IGNORE_UNUSED(ref_msg);
  out_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiVolumetricCloudComponent::SetCloudSettings(const xiiVolumetricCloudSettings& settings)
{
  m_Settings = settings;
  InvalidateCachedRenderData();
}

const xiiVolumetricCloudSettings& xiiVolumetricCloudComponent::GetCloudSettings() const
{
  return m_Settings;
}

void xiiVolumetricCloudComponent::SetPriority(xiiInt32 iPriority)
{
  if (m_iPriority == iPriority)
    return;

  m_iPriority = iPriority;
  InvalidateCachedRenderData();
}

xiiInt32 xiiVolumetricCloudComponent::GetPriority() const
{
  return m_iPriority;
}

void xiiVolumetricCloudComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  const xiiRenderWorldModule* pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  const xiiQuat                 rotation    = GetOwner()->GetGlobalRotation();
  xiiVolumetricCloudRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiVolumetricCloudRenderData>(this);
  pRenderData->m_Settings                   = m_Settings;
  pRenderData->m_vLayerNormal               = rotation * xiiVec3(0.0f, 0.0f, 1.0f);
  pRenderData->m_vProjectionAxisU           = rotation * xiiVec3(1.0f, 0.0f, 0.0f);
  pRenderData->m_vProjectionAxisV           = rotation * xiiVec3(0.0f, 1.0f, 0.0f);
  pRenderData->m_vLayerOrigin               = GetOwner()->GetGlobalPosition() + pRenderData->m_vLayerNormal * m_Settings.m_fLayerAltitudeMeters;
  pRenderData->m_iPriority                  = m_iPriority;
  pRenderData->m_uiSortingKey               = GetUniqueIdForRendering();
  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Fog_Implementation_VolumetricCloudComponent);
