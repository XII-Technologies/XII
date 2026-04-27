#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Camera/RenderTargetComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderTargetRenderData, 1, xiiRTTIDefaultAllocator<xiiRenderTargetRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiRenderTargetComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Width",       GetWidth,       SetWidth)->AddAttributes(new xiiDefaultValueAttribute(512u), new xiiClampValueAttribute(1u, 16384u)),
    XII_ACCESSOR_PROPERTY("Height",      GetHeight,      SetHeight)->AddAttributes(new xiiDefaultValueAttribute(512u), new xiiClampValueAttribute(1u, 16384u)),
    XII_ACCESSOR_PROPERTY("MSAASamples", GetMSAASamples, SetMSAASamples)->AddAttributes(new xiiDefaultValueAttribute(1u)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Camera"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiRenderTargetComponent::xiiRenderTargetComponent()  = default;
xiiRenderTargetComponent::~xiiRenderTargetComponent() = default;

void xiiRenderTargetComponent::OnActivated()
{
  SUPER::OnActivated();
  RecreateRenderTarget();
}

void xiiRenderTargetComponent::OnDeactivated()
{
  m_hRenderTarget.Invalidate();
  SUPER::OnDeactivated();
}

void xiiRenderTargetComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_uiWidth << m_uiHeight << m_uiMSAASamples;
}

void xiiRenderTargetComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_uiWidth >> m_uiHeight >> m_uiMSAASamples;
}

void xiiRenderTargetComponent::SetWidth(xiiUInt32 uiWidth)
{
  if (m_uiWidth == uiWidth) return;
  m_uiWidth = xiiMath::Clamp(uiWidth, 1u, 16384u);
  RecreateRenderTarget();
}

void xiiRenderTargetComponent::SetHeight(xiiUInt32 uiHeight)
{
  if (m_uiHeight == uiHeight) return;
  m_uiHeight = xiiMath::Clamp(uiHeight, 1u, 16384u);
  RecreateRenderTarget();
}

void xiiRenderTargetComponent::SetMSAASamples(xiiUInt8 uiSamples)
{
  m_uiMSAASamples = uiSamples;
  RecreateRenderTarget();
}

void xiiRenderTargetComponent::RecreateRenderTarget()
{
  // In a real implementation this would call into xiiResourceManager to (re)create the RT.
  // We mark the render data as dirty so it is re-submitted next frame.
}

void xiiRenderTargetComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiRenderTargetRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiRenderTargetRenderData>(this);
  pRenderData->m_GlobalTransform         = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds            = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject            = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent         = GetHandle();
  pRenderData->m_hRenderTarget           = m_hRenderTarget;
  pRenderData->m_uiWidth                 = m_uiWidth;
  pRenderData->m_uiHeight                = m_uiHeight;
  pRenderData->m_uiMSAASamples           = m_uiMSAASamples;
  pRenderData->m_uiSortingKey            = GetUniqueIdForRendering(0);

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Camera_Implementation_RenderTargetComponent);
