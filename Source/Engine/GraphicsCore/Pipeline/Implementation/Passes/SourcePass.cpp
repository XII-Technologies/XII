#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/SourcePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSourcePass, 1, xiiRTTIDefaultAllocator<xiiSourcePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiGALTextureFormat, m_Format),
    XII_ENUM_MEMBER_PROPERTY("SampleCount", xiiGALMSAASampleCount, m_SampleCount),
    XII_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_MEMBER_PROPERTY("Clear", m_bClear),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSourcePass::xiiSourcePass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Vulkan)
  {
    m_Format = xiiGALTextureFormat::BGRA8UNormalizedSRGB;
  }
  else
  {
    m_Format = xiiGALTextureFormat::RGBA8UNormalizedSRGB;
  }

  m_SampleCount = xiiGALMSAASampleCount::OneSample;
  m_bClear      = true;
  m_ClearColor  = xiiColor::Black;
}

xiiSourcePass::~xiiSourcePass()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

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

bool xiiSourcePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  xiiUInt32 uiWidth  = static_cast<xiiUInt32>(view.GetViewport().width);
  xiiUInt32 uiHeight = static_cast<xiiUInt32>(view.GetViewport().height);

  xiiGALTextureCreationDescription desc;
  desc.m_Type               = xiiGALResourceDimension::Texture2D;
  desc.m_Format             = m_Format;
  desc.m_Size.width         = uiWidth;
  desc.m_Size.height        = uiHeight;
  desc.m_uiSampleCount      = m_SampleCount;
  desc.m_uiArraySizeOrDepth = view.GetCamera()->IsStereoscopic() ? 2 : 1;
  desc.m_BindFlags          = ((!xiiGALTextureFormat::IsDepthFormat(m_Format) ? xiiGALBindFlags::RenderTarget : xiiGALBindFlags::DepthStencil) | xiiGALBindFlags::ShaderResource);

  if (desc.m_uiArraySizeOrDepth > 1 || desc.m_uiSampleCount > xiiGALMSAASampleCount::OneSample)
    desc.m_Type = xiiGALResourceDimension::Texture2DArray;

  outputs[m_PinOutput.m_uiOutputIndex] = desc;

  return true;
}

void xiiSourcePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  if (!m_bClear)
    return;

  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (m_hRenderPass.IsInvalidated())
  {
    const auto& attachmentDescription = pDevice->GetTexture(pOutput->m_TextureHandle)->GetDescription();

    xiiGALRenderPassCreationDescription renderPassDescription;

    auto& subpassDescription    = renderPassDescription.m_SubPasses.ExpandAndGetRef();
    auto& dependencyDescription = renderPassDescription.m_Dependencies.ExpandAndGetRef();

    dependencyDescription.m_uiSourceSubPass      = XII_GAL_SUBPASS_EXTERNAL;
    dependencyDescription.m_uiDestinationSubPass = 0U;

    if (xiiGALTextureFormat::IsDepthFormat(pOutput->m_Desc.m_Format))
    {
      auto& depthAttachmentDescription = renderPassDescription.m_Attachments.ExpandAndGetRef();

      depthAttachmentDescription.m_Format                = attachmentDescription.m_Format;
      depthAttachmentDescription.m_uiSampleCount         = static_cast<xiiUInt8>(attachmentDescription.m_uiSampleCount);
      depthAttachmentDescription.m_InitialStateFlags     = xiiGALResourceStateFlags::Unknown;
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
      auto& colorAttachmentDescription = renderPassDescription.m_Attachments.ExpandAndGetRef();

      colorAttachmentDescription.m_Format                = attachmentDescription.m_Format;
      colorAttachmentDescription.m_uiSampleCount         = attachmentDescription.m_uiSampleCount;
      colorAttachmentDescription.m_InitialStateFlags     = xiiGALResourceStateFlags::Unknown;
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

  if (auto pFramebuffer = pDevice->GetFramebuffer(m_hFramebuffer))
  {
    const bool  bIsDepthAttachment        = xiiGALTextureFormat::IsDepthFormat(pOutput->m_Desc.m_Format);
    const auto& hAttachmentView           = pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(bIsDepthAttachment ? xiiGALTextureViewType::DepthStencil : xiiGALTextureViewType::RenderTarget);

    if (pFramebuffer->GetDescription().m_Attachments.PeekBack() != hAttachmentView)
    {
      pDevice->DestroyFramebuffer(m_hFramebuffer);
      m_hFramebuffer.Invalidate();
    }
  }

  if (m_hFramebuffer.IsInvalidated())
  {
    const bool  bIsDepthAttachment        = xiiGALTextureFormat::IsDepthFormat(pOutput->m_Desc.m_Format);
    const auto& attachmentDescription     = pDevice->GetTexture(pOutput->m_TextureHandle)->GetDescription();
    const auto& hAttachmentView           = pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(bIsDepthAttachment ? xiiGALTextureViewType::DepthStencil : xiiGALTextureViewType::RenderTarget);
    const auto& attachmentViewDescription = pDevice->GetTextureView(hAttachmentView)->GetDescription();
    xiiVec3U32  vSize                     = xiiGALGraphicsUtilities::GetMipLevelSize(attachmentViewDescription.m_uiMostDetailedMip, attachmentDescription);

    xiiGALFramebufferCreationDescription framebufferDescription;

    framebufferDescription.m_hRenderPass       = m_hRenderPass;
    framebufferDescription.m_FramebufferSize   = {vSize.x, vSize.y};
    framebufferDescription.m_uiArraySliceCount = attachmentDescription.GetArraySize();
    framebufferDescription.m_Attachments.PushBack(hAttachmentView);

    m_hFramebuffer = pDevice->CreateFramebuffer(framebufferDescription);
    XII_ASSERT_DEV(!m_hFramebuffer.IsInvalidated(), "Failed to frame buffer.");
  }

  xiiGALBeginRenderPassDescription renderPassDescription = {.m_hRenderPass = m_hRenderPass, .m_hFramebuffer = m_hFramebuffer};

  if (xiiGALTextureFormat::IsDepthFormat(pOutput->m_Desc.m_Format))
  {
    auto& clearValue = renderPassDescription.m_ClearValues.ExpandAndGetRef();

    clearValue.m_TextureFormat            = pOutput->m_Desc.m_Format;
    clearValue.m_DepthStencil.m_fDepth    = 1.0f;
    clearValue.m_DepthStencil.m_uiStencil = 0U;
  }
  else
  {
    auto& clearValue = renderPassDescription.m_ClearValues.ExpandAndGetRef();

    clearValue.m_TextureFormat = pOutput->m_Desc.m_Format;
    clearValue.m_ClearColor    = m_ClearColor;
  }

  if (auto pGraphicsQueue = pDevice->GetGraphicsQueue())
  {
    auto pCommandList = pGraphicsQueue->BeginCommandList(GetName());
    pCommandList->BeginRenderPass(renderPassDescription);
    pCommandList->EndRenderPass();
    pGraphicsQueue->Submit(pCommandList);
  }
}

xiiResult xiiSourcePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Format;
  inout_stream << m_SampleCount;
  inout_stream << m_ClearColor;
  inout_stream << m_bClear;
  return XII_SUCCESS;
}

xiiResult xiiSourcePass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_Format;
  inout_stream >> m_SampleCount;
  inout_stream >> m_ClearColor;
  inout_stream >> m_bClear;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SourcePass);
