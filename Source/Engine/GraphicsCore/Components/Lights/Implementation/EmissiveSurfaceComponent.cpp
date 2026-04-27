#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/EmissiveSurfaceComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEmissiveSurfaceRenderData, 1, xiiRTTIDefaultAllocator<xiiEmissiveSurfaceRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiEmissiveSurfaceComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("EmissiveColor",    GetEmissiveColor,    SetEmissiveColor),
    XII_ACCESSOR_PROPERTY("IntensityScale",   GetIntensityScale,   SetIntensityScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
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
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiEmissiveSurfaceComponent::xiiEmissiveSurfaceComponent()  = default;
xiiEmissiveSurfaceComponent::~xiiEmissiveSurfaceComponent() = default;

void xiiEmissiveSurfaceComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_EmissiveColor << m_fIntensityScale;
}

void xiiEmissiveSurfaceComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_EmissiveColor >> m_fIntensityScale;
}

xiiResult xiiEmissiveSurfaceComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bounds);
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);
  // Bounds come from the co-located mesh component; return failure so we don't override them.
  return XII_FAILURE;
}

void xiiEmissiveSurfaceComponent::SetEmissiveColor(const xiiColor& color)
{
  m_EmissiveColor = xiiColorLinearUB(color);
  InvalidateCachedRenderData();
}

xiiColor xiiEmissiveSurfaceComponent::GetEmissiveColor() const { return xiiColor(m_EmissiveColor); }

void xiiEmissiveSurfaceComponent::SetIntensityScale(float f)
{
  m_fIntensityScale = xiiMath::Max(f, 0.0f);
  InvalidateCachedRenderData();
}

void xiiEmissiveSurfaceComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiEmissiveSurfaceRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiEmissiveSurfaceRenderData>(this);
  pRenderData->m_GlobalTransform            = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds               = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject               = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent            = GetHandle();
  pRenderData->m_EmissiveColor              = m_EmissiveColor;
  pRenderData->m_fIntensityScale            = m_fIntensityScale;
  pRenderData->m_uiSortingKey               = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_EmissiveSurfaceComponent);
