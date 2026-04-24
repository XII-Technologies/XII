#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalLightRenderData, 1, xiiRTTIDefaultAllocator<xiiDirectionalLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDirectionalLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("SourceAngle", GetSourceAngle, SetSourceAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::MakeZero(), xiiAngle::MakeFromDegree(10.0f))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 1.0f, xiiColor::White, "LightColor"),
    new xiiDirectionalLightVisualizerAttribute("SourceAngle", "LightColor"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

void xiiDirectionalLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_SourceAngle;
}

void xiiDirectionalLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_SourceAngle;
}

xiiResult xiiDirectionalLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bounds);
  XII_IGNORE_UNUSED(ref_msg);

  ref_bAlwaysVisible = true;

  return XII_SUCCESS;
}

void xiiDirectionalLightComponent::SetSourceAngle(xiiAngle sourceAngle)
{
  m_SourceAngle = xiiMath::Clamp(sourceAngle, xiiAngle::MakeZero(), xiiAngle::MakeFromDegree(10.0f));

  InvalidateCachedRenderData();
}

xiiAngle xiiDirectionalLightComponent::GetSourceAngle() const
{
  return m_SourceAngle;
}

void xiiDirectionalLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  xiiDirectionalLightRenderData* pRenderData = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), xiiDirectionalLightRenderData);
  pRenderData->m_LightColor                  = m_LightColor;
  pRenderData->m_uiTemperature               = m_uiTemperature;
  pRenderData->m_fIntensity                  = m_fIntensity;
  pRenderData->m_bCastShadows                = m_bCastShadows;
  pRenderData->m_vDirection                  = GetOwner()->GetGlobalRotation() * xiiVec3(-1.0f, 0.0f, 0.0f);
  pRenderData->m_fRadius                     = xiiMath::Sin(m_SourceAngle * 0.5f); // This is interpreted as the sin(halfangle) of the emitter disc.
  pRenderData->m_uiSortingKey                = GetUniqueIdForRendering();

  xiiEnum<xiiRenderData::Caching> caching = m_bCastShadows ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never;
  ref_msg.AddRenderData(pRenderData, caching);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalLightVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiDirectionalLightVisualizerAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDirectionalLightVisualizerAttribute::xiiDirectionalLightVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiDirectionalLightVisualizerAttribute::xiiDirectionalLightVisualizerAttribute(xiiStringView sAngleProperty, xiiStringView sColorProperty) :
  xiiVisualizerAttribute(sAngleProperty, sColorProperty)
{
}
