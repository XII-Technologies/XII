#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/RectangleAreaLightComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRectAreaLightRenderData, 1, xiiRTTIDefaultAllocator<xiiRectAreaLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiRectAreaLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Width",     GetWidth,     SetWidth)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.001f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Height",    GetHeight,    SetHeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.001f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("TwoSided",  GetTwoSided,  SetTwoSided),
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
    new xiiBoxVisualizerAttribute("Width", "Height", 0.001f, nullptr),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiRectAreaLightComponent::xiiRectAreaLightComponent()  = default;
xiiRectAreaLightComponent::~xiiRectAreaLightComponent() = default;

void xiiRectAreaLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_fWidth << m_fHeight << m_bTwoSided;
}

void xiiRectAreaLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_fWidth >> m_fHeight >> m_bTwoSided;
}

xiiResult xiiRectAreaLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);

  const float fR = xiiMath::Max(m_fWidth, m_fHeight) * 0.5f + m_fIntensity * 0.5f; // rough influence radius
  ref_bounds     = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), fR));
  return XII_SUCCESS;
}

void xiiRectAreaLightComponent::SetWidth(float f)
{
  m_fWidth = xiiMath::Max(f, 0.001f);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}
void xiiRectAreaLightComponent::SetHeight(float f)
{
  m_fHeight = xiiMath::Max(f, 0.001f);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}
void xiiRectAreaLightComponent::SetTwoSided(bool b)
{
  m_bTwoSided = b;
  InvalidateCachedRenderData();
}

void xiiRectAreaLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (m_fIntensity <= 0.0f || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiRectAreaLightRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiRectAreaLightRenderData>(this);
  pRenderData->m_GlobalTransform          = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds             = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject             = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent          = GetHandle();
  pRenderData->m_LightColor               = m_LightColor;
  pRenderData->m_uiTemperature            = m_uiTemperature;
  pRenderData->m_fIntensity               = m_fIntensity;
  pRenderData->m_bCastShadows             = m_bCastShadows;
  pRenderData->m_fRadius                  = xiiMath::Max(m_fWidth, m_fHeight) * 0.5f;
  pRenderData->m_fWidth                   = m_fWidth;
  pRenderData->m_fHeight                  = m_fHeight;
  pRenderData->m_bTwoSided                = m_bTwoSided;
  pRenderData->m_uiSortingKey             = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_RectangleAreaLightComponent);
