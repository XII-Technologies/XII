/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/RectangleAreaLightComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRectangleAreaLightRenderData, 1, xiiRTTIDefaultAllocator<xiiRectangleAreaLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiRectangleAreaLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(1.0f, 1.0f)), new xiiSuffixAttribute(" m")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiRectangleAreaLightVisualizerAttribute("Extents", "LightColor", "Intensity"),
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiRectangleAreaLightComponent::xiiRectangleAreaLightComponent()  = default;
xiiRectangleAreaLightComponent::~xiiRectangleAreaLightComponent() = default;

void xiiRectangleAreaLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vExtents;
}

void xiiRectangleAreaLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_vExtents;
}

xiiResult xiiRectangleAreaLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);

  ref_bounds         = CalculateBoundingSphere(xiiTransform::MakeIdentity(), m_vExtents);
  ref_bAlwaysVisible = false;

  return XII_SUCCESS;
}

void xiiRectangleAreaLightComponent::SetExtents(xiiVec2 vExtents)
{
  if (m_vExtents == vExtents)
    return;

  m_vExtents = vExtents;

  TriggerLocalBoundsUpdate();
}

xiiVec2 xiiRectangleAreaLightComponent::GetExtents() const
{
  return m_vExtents;
}

void xiiRectangleAreaLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  if (m_fIntensity <= 0.0f || m_vExtents.IsZero())
    return;

  auto                             pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  xiiRectangleAreaLightRenderData* pRenderData  = pWorldModule->CreateRenderDataForThisFrame<xiiRectangleAreaLightRenderData>(this);
  pRenderData->m_LightColor                     = m_LightColor;
  pRenderData->m_uiTemperature                  = m_uiTemperature;
  pRenderData->m_fIntensity                     = m_fIntensity;
  pRenderData->m_bCastShadows                   = m_bCastShadows;
  pRenderData->m_vExtents                       = m_vExtents;
  pRenderData->m_uiSortingKey                   = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, m_bCastShadows ? xiiRenderData::Caching::IfStatic : xiiRenderData::Caching::Never);
}

xiiBoundingSphere xiiRectangleAreaLightComponent::CalculateBoundingSphere(const xiiTransform& transform, const xiiVec2& vExtents) const
{
  const float fRadius = vExtents.GetLength() * 0.5f;

  return xiiBoundingSphere::MakeFromCenterAndRadius(transform.m_vPosition, fRadius);
}

////////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRectangleAreaLightVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiRectangleAreaLightVisualizerAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRectangleAreaLightVisualizerAttribute::xiiRectangleAreaLightVisualizerAttribute() :
  xiiVisualizerAttribute({})
{
}

xiiRectangleAreaLightVisualizerAttribute::xiiRectangleAreaLightVisualizerAttribute(xiiStringView sExtentsProperty, xiiStringView sColorProperty, xiiStringView sIntensityProperty) :
  xiiVisualizerAttribute(sExtentsProperty, sColorProperty, sIntensityProperty)
{
}
