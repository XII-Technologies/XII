#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/HistorySourcePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistorySourcePassTextureDataProvider, 1, xiiRTTIDefaultAllocator<xiiHistorySourcePassTextureDataProvider>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistorySourcePass, 1, xiiRTTIDefaultAllocator<xiiHistorySourcePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format),
    XII_ENUM_MEMBER_PROPERTY("MSAA_Mode", xiiGALMSAASampleCount, m_MsaaMode),
    XII_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new xiiExposeColorAlphaAttribute())
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHistorySourcePassTextureDataProvider::xiiHistorySourcePassTextureDataProvider() = default;
xiiHistorySourcePassTextureDataProvider::~xiiHistorySourcePassTextureDataProvider()
{
  while (!m_Data.IsEmpty())
  {
    ResetTexture(m_Data.GetIterator().Key());
  }
}

void xiiHistorySourcePassTextureDataProvider::ResetTexture(xiiStringView sSourcePassName)
{
  if (xiiGALTextureHandle* pHandle = m_Data.GetValue(sSourcePassName))
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
    pDevice->DestroyTexture(*pHandle);
    m_Data.Remove(sSourcePassName);
  }
}

xiiGALTextureHandle xiiHistorySourcePassTextureDataProvider::GetOrCreateTexture(xiiStringView sSourcePassName, const xiiGALTextureCreationDescription& desc)
{
  bool                 bExisted;
  xiiGALTextureHandle& hTexture = m_Data.FindOrAdd(sSourcePassName, &bExisted);
  if (!bExisted)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
    hTexture                           = pDevice->CreateTexture(desc);
    if (hTexture.IsInvalidated())
    {
      xiiLog::Error("Failed to create history source pass texture.");
    }
  }
  return hTexture;
}


xiiHistorySourcePass::xiiHistorySourcePass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true)
{
}

xiiHistorySourcePass::~xiiHistorySourcePass()
{
  FreeCachedRenderPasses();
}

bool xiiHistorySourcePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pData = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassTextureDataProvider>();
  pData->ResetTexture(GetName());

  m_bFirstExecute                      = true;
  outputs[m_PinOutput.m_uiOutputIndex] = xiiSourcePass::GetOutputDescription(view, m_Format, m_MsaaMode);
  return true;
}

xiiGALTextureViewHandle xiiHistorySourcePass::QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc)
{
  auto pData = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassTextureDataProvider>();
  return xiiGALDevice::GetDefaultDevice()->GetTexture(pData->GetOrCreateTexture(GetName(), desc))->GetDefaultView(xiiGALTextureViewType::RenderTarget);
}

void xiiHistorySourcePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr || !m_bFirstExecute)
    return;

  m_bFirstExecute = false;

  xiiSharedPtr<xiiGALDevice> pDevice              = xiiGALDevice::GetDefaultDevice();
  bool                       bRecreateRenderPass  = true;
  bool                       bRecreateFramebuffer = true;
  const bool                 bIsDepthAttachment   = xiiGALResourceFormat::IsDepthFormat(pOutput->m_TextureDescription.m_Format);

  if (!m_hRenderPass.IsInvalidated())
  {
    xiiGALRenderPass* pRenderPass           = pDevice->GetRenderPass(m_hRenderPass);
    const auto&       attachmentDescription = pRenderPass->GetDescription().m_Attachments.PeekBack();

    if (attachmentDescription.m_Format == pOutput->m_TextureDescription.m_Format && attachmentDescription.m_uiSampleCount == m_MsaaMode.GetValue())
    {
      bRecreateRenderPass = false;
    }
  }

  if (!m_hFramebuffer.IsInvalidated())
  {
    xiiGALFramebuffer* pFramebuffer    = pDevice->GetFramebuffer(m_hFramebuffer);
    const auto&        hAttachmentView = pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(bIsDepthAttachment ? xiiGALTextureViewType::DepthStencil : xiiGALTextureViewType::RenderTarget);

    if (pDevice->GetFramebuffer(m_hFramebuffer)->GetDescription().m_Attachments.PeekBack() == hAttachmentView)
    {
      bRecreateFramebuffer = false;
    }
  }

  if (bRecreateRenderPass)
  {
    bRecreateFramebuffer = true;

    FreeCachedRenderPasses();

    const auto& attachmentDescription = pDevice->GetTexture(pOutput->m_TextureHandle)->GetDescription();

    xiiGALRenderPassCreationDescription renderPassDescription;
    auto&                               subpassDescription    = renderPassDescription.m_SubPasses.ExpandAndGetRef();
    auto&                               dependencyDescription = renderPassDescription.m_Dependencies.ExpandAndGetRef();

    dependencyDescription.m_uiSourceSubPass      = XII_GAL_SUBPASS_EXTERNAL;
    dependencyDescription.m_uiDestinationSubPass = 0U;

    if (bIsDepthAttachment)
    {
      auto& depthAttachmentDescription                   = renderPassDescription.m_Attachments.ExpandAndGetRef();
      depthAttachmentDescription.m_Format                = attachmentDescription.m_Format;
      depthAttachmentDescription.m_uiSampleCount         = static_cast<xiiUInt8>(attachmentDescription.m_uiSampleCount);
      depthAttachmentDescription.m_InitialStateFlags     = xiiGALResourceStateFlags::DepthWrite;
      depthAttachmentDescription.m_FinalStateFlags       = xiiGALResourceStateFlags::DepthWrite;
      depthAttachmentDescription.m_LoadOperation         = xiiGALAttachmentLoadOperation::Clear;
      depthAttachmentDescription.m_StoreOperation        = xiiGALAttachmentStoreOperation::Store;
      depthAttachmentDescription.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Clear;
      depthAttachmentDescription.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Store;

      auto& depthAttachmentReference                = subpassDescription.m_DepthStencilAttachment.ExpandAndGetRef();
      depthAttachmentReference.m_ResourceStateFlags = xiiGALResourceStateFlags::DepthWrite;
      depthAttachmentReference.m_uiAttachmentIndex  = 0U;

      dependencyDescription.m_SourceStageFlags       = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
      dependencyDescription.m_DestinationStageFlags  = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
      dependencyDescription.m_SourceAccessFlags      = xiiGALAccessFlags::DepthStencilWrite;
      dependencyDescription.m_DestinationAccessFlags = xiiGALAccessFlags::DepthStencilWrite;
    }
    else
    {
      auto& colorAttachmentDescription                   = renderPassDescription.m_Attachments.ExpandAndGetRef();
      colorAttachmentDescription.m_Format                = attachmentDescription.m_Format;
      colorAttachmentDescription.m_uiSampleCount         = attachmentDescription.m_uiSampleCount;
      colorAttachmentDescription.m_InitialStateFlags     = xiiGALResourceStateFlags::RenderTarget;
      colorAttachmentDescription.m_FinalStateFlags       = xiiGALResourceStateFlags::RenderTarget;
      colorAttachmentDescription.m_LoadOperation         = xiiGALAttachmentLoadOperation::Clear;
      colorAttachmentDescription.m_StoreOperation        = xiiGALAttachmentStoreOperation::Store;
      colorAttachmentDescription.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Discard;
      colorAttachmentDescription.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Discard;

      auto& colorAttachmentReference                = subpassDescription.m_RenderTargetAttachments.ExpandAndGetRef();
      colorAttachmentReference.m_ResourceStateFlags = xiiGALResourceStateFlags::RenderTarget;
      colorAttachmentReference.m_uiAttachmentIndex  = 0U;

      dependencyDescription.m_SourceStageFlags       = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
      dependencyDescription.m_DestinationStageFlags  = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
      dependencyDescription.m_SourceAccessFlags      = xiiGALAccessFlags::RenderTargetWrite;
      dependencyDescription.m_DestinationAccessFlags = xiiGALAccessFlags::RenderTargetWrite;
    }

    m_hRenderPass = pDevice->CreateRenderPass(renderPassDescription);
    XII_ASSERT_DEV(!m_hRenderPass.IsInvalidated(), "Failed to create render pass.");
  }

  if (bRecreateFramebuffer)
  {
    const auto& attachmentDescription     = pDevice->GetTexture(pOutput->m_TextureHandle)->GetDescription();
    const auto& hAttachmentView           = pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(bIsDepthAttachment ? xiiGALTextureViewType::DepthStencil : xiiGALTextureViewType::RenderTarget);
    const auto& attachmentViewDescription = pDevice->GetTextureView(hAttachmentView)->GetDescription();
    xiiVec3U32  vSize                     = xiiGALTextureUtilities::GetMipLevelSize(attachmentViewDescription.m_uiMostDetailedMip, attachmentDescription);

    xiiGALFramebufferCreationDescription framebufferDescription;
    framebufferDescription.m_hRenderPass       = m_hRenderPass;
    framebufferDescription.m_FramebufferSize   = {vSize.x, vSize.y};
    framebufferDescription.m_uiArraySliceCount = attachmentDescription.GetArraySize();
    framebufferDescription.m_Attachments.PushBack(hAttachmentView);

    m_hFramebuffer = pDevice->CreateFramebuffer(framebufferDescription);
    XII_ASSERT_DEV(!m_hFramebuffer.IsInvalidated(), "Failed to create frame buffer.");
  }

  xiiGALBeginRenderPassDescription renderPassDescription = {.m_hRenderPass = m_hRenderPass, .m_hFramebuffer = m_hFramebuffer};

  if (bIsDepthAttachment)
  {
    auto& clearValue                      = renderPassDescription.m_ClearValues.ExpandAndGetRef();
    clearValue.m_ResourceFormat           = pOutput->m_TextureDescription.m_Format;
    clearValue.m_DepthStencil.m_fDepth    = 1.0f;
    clearValue.m_DepthStencil.m_uiStencil = 0U;
  }
  else
  {
    auto& clearValue            = renderPassDescription.m_ClearValues.ExpandAndGetRef();
    clearValue.m_ResourceFormat = pOutput->m_TextureDescription.m_Format;
    clearValue.m_ClearColor     = m_ClearColor;
  }

  if (auto pGraphicsQueue = pDevice->GetDefaultCommandQueue())
  {
    auto pCommandList = pGraphicsQueue->BeginCommandList();

    pCommandList->BeginDebugGroup(GetName());
    {
      pCommandList->BeginRenderPass(renderPassDescription);
      pCommandList->EndRenderPass();
    }
    pCommandList->EndDebugGroup();
    pCommandList->Submit();
  }
}

xiiResult xiiHistorySourcePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Format;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;

  return XII_SUCCESS;
}

xiiResult xiiHistorySourcePass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_Format;
  inout_stream >> m_MsaaMode;
  inout_stream >> m_ClearColor;

  return XII_SUCCESS;
}

void xiiHistorySourcePass::FreeCachedRenderPasses()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_hRenderPass.IsInvalidated())
  {
    pDevice->DestroyRenderPass(m_hRenderPass);
    m_hRenderPass.Invalidate();
  }
  if (!m_hFramebuffer.IsInvalidated())
  {
    pDevice->DestroyFramebuffer(m_hFramebuffer);
    m_hFramebuffer.Invalidate();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_HistorySourcePass);
