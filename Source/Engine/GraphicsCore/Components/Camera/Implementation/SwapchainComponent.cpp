#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Camera/SwapchainComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiPresentMode, 1)
  XII_ENUM_CONSTANTS(xiiPresentMode::Immediate, xiiPresentMode::Fifo, xiiPresentMode::FifoRelaxed, xiiPresentMode::Mailbox)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSwapchainRenderData, 1, xiiRTTIDefaultAllocator<xiiSwapchainRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSwapchainComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("PresentMode", xiiPresentMode, GetPresentMode, SetPresentMode),
    XII_ACCESSOR_PROPERTY("BufferCount", GetBufferCount, SetBufferCount)->AddAttributes(new xiiDefaultValueAttribute(2u), new xiiClampValueAttribute(2u, 4u)),
    XII_ACCESSOR_PROPERTY("HDR", GetHDR, SetHDR),
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

xiiSwapchainComponent::xiiSwapchainComponent()  = default;
xiiSwapchainComponent::~xiiSwapchainComponent() = default;

void xiiSwapchainComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_PresentMode.GetValue() << m_uiBufferCount << m_bHDR;
}

void xiiSwapchainComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto&    s  = inout_stream.GetStream();
  xiiUInt8 pm = 0;
  s >> pm;
  m_PresentMode = static_cast<xiiPresentMode::Enum>(pm);
  s >> m_uiBufferCount >> m_bHDR;
}

void xiiSwapchainComponent::SetPresentMode(xiiEnum<xiiPresentMode> mode) { m_PresentMode = mode; }
void xiiSwapchainComponent::SetBufferCount(xiiUInt8 uiCount) { m_uiBufferCount = xiiMath::Clamp<xiiUInt8>(uiCount, 2, 4); }
void xiiSwapchainComponent::SetHDR(bool bHDR) { m_bHDR = bHDR; }

void xiiSwapchainComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiSwapchainRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiSwapchainRenderData>(this);
  pRenderData->m_GlobalTransform      = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds         = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject         = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent      = GetHandle();
  pRenderData->m_PresentMode          = m_PresentMode;
  pRenderData->m_uiBufferCount        = m_uiBufferCount;
  pRenderData->m_bHDR                 = m_bHDR;
  pRenderData->m_uiSortingKey         = GetUniqueIdForRendering(0);

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Camera_Implementation_SwapchainComponent);
