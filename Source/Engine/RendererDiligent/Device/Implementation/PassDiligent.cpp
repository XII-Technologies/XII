#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/CommandEncoder/CommandEncoderImplDiligent.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Device/PassDiligent.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

XII_CHECK_AT_COMPILETIME(sizeof(xiiUInt32) == sizeof(xiiGALRenderTargetViewHandle));
namespace
{
  XII_ALWAYS_INLINE xiiStreamWriter& operator<<(xiiStreamWriter& Stream, const xiiGALRenderTargetViewHandle& Value)
  {
    Stream << reinterpret_cast<const xiiUInt32&>(Value);
    return Stream;
  }
} // namespace

xiiGALPassDiligent::xiiGALPassDiligent(xiiGALDevice& device) :
  xiiGALPass(device),
  m_GALDeviceDiligent(static_cast<xiiGALDeviceDiligent&>(device))
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderRenderState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderImplDiligent, static_cast<xiiGALDeviceDiligent&>(device));

  m_pRenderCommandEncoder  = XII_DEFAULT_NEW(xiiGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

xiiGALPassDiligent::~xiiGALPassDiligent()
{
  ReleaseRenderPassResources();
}

xiiGALRenderCommandEncoder* xiiGALPassDiligent::BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void xiiGALPassDiligent::EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndRendering();
}

xiiGALComputeCommandEncoder* xiiGALPassDiligent::BeginComputePlatform(const char* szName)
{
  m_pCommandEncoderImpl->BeginCompute();

  return m_pComputeCommandEncoder.Borrow();
}

void xiiGALPassDiligent::EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndCompute();
}

void xiiGALPassDiligent::ReleaseRenderPassResources()
{
  for (auto iter : m_RenderPasses)
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pRenderPass);
  }
  m_RenderPasses.Clear();
  m_RenderPasses.Compact();

  for (auto iter : m_Framebuffers)
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pFramebuffer);
  }
  m_Framebuffers.Clear();
  m_Framebuffers.Compact();
}

Diligent::IRenderPass* xiiGALPassDiligent::RequestRenderPass(const xiiGALRenderingSetup& renderingSetup)
{
  RenderPassDesc renderPassDesc;
  GetRenderPassDesc(renderingSetup, renderPassDesc);

  Diligent::IRenderPass* pRenderPass = RequestRenderPassInternal(renderingSetup, renderPassDesc);

  return pRenderPass;
}

Diligent::IFramebuffer* xiiGALPassDiligent::RequestFrameBuffer(Diligent::IRenderPass* pRenderPass, const xiiGALRenderTargetSetup& renderTargetSetup, xiiVec2U32 out_Size, xiiEnum<xiiGALMSAASampleCount> out_MSAA)
{
  return nullptr;
}

Diligent::IRenderPass* xiiGALPassDiligent::RequestRenderPassInternal(const xiiGALRenderingSetup& renderingSetup, RenderPassDesc& desc)
{
  if (const RenderPassInfo* pRenderPassInfo = m_RenderPasses.GetValue(renderingSetup))
  {
    return pRenderPassInfo->m_pRenderPass;
  }

  xiiLog::Dev("Creating RenderPass #{}", m_RenderPasses.GetCount());

  xiiHybridArray<Diligent::AttachmentReference, 1> depthAttachmentRefs;
  xiiHybridArray<Diligent::AttachmentReference, 4> colorAttachmentRefs;

  const xiiUInt32 uiAttachmentCount = desc.m_Attachments.GetCount();
  for (xiiUInt32 i = 0; uiAttachmentCount; ++i)
  {
    auto& attachment = desc.m_Attachments[i];

    const bool bIsDepthAttachment = xiiDiligentUtils::IsDepthFormat(attachment.Format);
    if (bIsDepthAttachment)
    {
      attachment.FinalState = Diligent::RESOURCE_STATE_DEPTH_WRITE | Diligent::RESOURCE_STATE_DEPTH_READ;

      Diligent::AttachmentReference& attachmentRef = depthAttachmentRefs.ExpandAndGetRef();
      attachmentRef.AttachmentIndex                = i;
      attachmentRef.State                          = Diligent::RESOURCE_STATE_DEPTH_WRITE | Diligent::RESOURCE_STATE_DEPTH_READ;
    }
    else
    {
      attachment.FinalState = Diligent::RESOURCE_STATE_RENDER_TARGET;

      Diligent::AttachmentReference& attachmentRef = colorAttachmentRefs.ExpandAndGetRef();
      attachmentRef.AttachmentIndex                = i;
      attachmentRef.State                          = Diligent::RESOURCE_STATE_RENDER_TARGET;
    }
  }

  XII_ASSERT_DEV(depthAttachmentRefs.GetCount() <= 1u, "There can only be a maximum of 1 bound depth attachment.");

  const bool bHasDepthAttachment = !depthAttachmentRefs.IsEmpty();
  const bool bHasColorAttachment = !colorAttachmentRefs.IsEmpty();

  Diligent::SubpassDesc subpassDesc;
  subpassDesc.pDepthStencilAttachment     = bHasDepthAttachment ? depthAttachmentRefs.GetData() : nullptr;
  subpassDesc.pRenderTargetAttachments    = bHasColorAttachment ? colorAttachmentRefs.GetData() : nullptr;
  subpassDesc.RenderTargetAttachmentCount = colorAttachmentRefs.GetCount();

  Diligent::SubpassDependencyDesc subpassDependencyDesc;
  subpassDependencyDesc.DstSubpass = 0u;

  if (bHasColorAttachment)
    subpassDependencyDesc.DstAccessMask |= Diligent::ACCESS_FLAG_RENDER_TARGET_WRITE;

  if (bHasDepthAttachment)
    subpassDependencyDesc.DstAccessMask |= Diligent::ACCESS_FLAG_DEPTH_STENCIL_WRITE;

  subpassDependencyDesc.DstStageMask  = Diligent::PIPELINE_STAGE_FLAG_RENDER_TARGET | Diligent::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS;
  subpassDependencyDesc.SrcSubpass    = Diligent::SUBPASS_EXTERNAL;
  subpassDependencyDesc.SrcAccessMask = Diligent::ACCESS_FLAG_NONE;
  subpassDependencyDesc.SrcStageMask  = Diligent::PIPELINE_STAGE_FLAG_RENDER_TARGET | Diligent::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS;

  Diligent::RenderPassDesc renderPassDescription;
  renderPassDescription.pAttachments    = desc.m_Attachments.GetData();
  renderPassDescription.AttachmentCount = desc.m_Attachments.GetCount();
  renderPassDescription.pSubpasses      = &subpassDesc;
  renderPassDescription.SubpassCount    = 1u;
  renderPassDescription.pDependencies   = &subpassDependencyDesc;
  renderPassDescription.DependencyCount = 1u;

  Diligent::IRenderPass* pRenderPass = nullptr;
  m_GALDeviceDiligent.GetDevice()->CreateRenderPass(renderPassDescription, &pRenderPass);

  if (pRenderPass == nullptr)
  {
    xiiLog::Error("Failed to create RenderPass.");
  }

  RenderPassInfo renderPassInfo{pRenderPass};
  m_RenderPasses.Insert(renderingSetup, renderPassInfo);

  return pRenderPass;
}

void xiiGALPassDiligent::GetRenderPassDesc(const xiiGALRenderingSetup& renderingSetup, RenderPassDesc& out_Desc)
{
  const bool      bHasDepthTarget        = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt32 uiColorAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  out_Desc.m_Attachments.Clear();

  if (bHasDepthTarget)
  {
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format             = textureDescription.m_Format;
    const auto&                             formatInfo         = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(format);

    Diligent::RenderPassAttachmentDesc& depthAttachment = out_Desc.m_Attachments.ExpandAndGetRef();
    depthAttachment.Format                              = formatInfo.m_eRenderTarget;
    depthAttachment.SampleCount                         = xiiDiligentUtils::ToDiligentMSAACount(textureDescription.m_SampleCount);

    if (renderingSetup.m_bDiscardDepth)
    {
      depthAttachment.InitialState = Diligent::RESOURCE_STATE_UNDEFINED;
      depthAttachment.LoadOp       = Diligent::ATTACHMENT_LOAD_OP_DISCARD;
    }
    else
    {
      depthAttachment.InitialState = renderingSetup.m_bClearDepth ? Diligent::RESOURCE_STATE_UNDEFINED : Diligent::RESOURCE_STATE_RENDER_TARGET;
      depthAttachment.LoadOp       = renderingSetup.m_bClearDepth ? Diligent::ATTACHMENT_LOAD_OP_CLEAR : Diligent::ATTACHMENT_LOAD_OP_LOAD;
    }
    depthAttachment.StoreOp = Diligent::ATTACHMENT_STORE_OP_STORE;

    if (format == xiiGALResourceFormat::D24S8)
    {
      depthAttachment.StencilLoadOp  = renderingSetup.m_bClearStencil ? Diligent::ATTACHMENT_LOAD_OP_CLEAR : Diligent::ATTACHMENT_LOAD_OP_LOAD;
      depthAttachment.StencilStoreOp = Diligent::ATTACHMENT_STORE_OP_STORE;
    }
    else
    {
      depthAttachment.StencilLoadOp  = Diligent::ATTACHMENT_LOAD_OP_DISCARD;
      depthAttachment.StencilStoreOp = Diligent::ATTACHMENT_STORE_OP_DISCARD;
    }
  }

  for (xiiUInt32 i = 0; i < uiColorAttachmentCount; ++i)
  {
    xiiGALRenderTargetViewHandle          hColorRenderTarget        = renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i));
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(hColorRenderTarget));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format             = textureDescription.m_Format;
    const auto&                             formatInfo         = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(format);

    Diligent::RenderPassAttachmentDesc& colorAttachment = out_Desc.m_Attachments.ExpandAndGetRef();
    colorAttachment.Format                              = formatInfo.m_eRenderTarget;
    colorAttachment.SampleCount                         = xiiDiligentUtils::ToDiligentMSAACount(textureDescription.m_SampleCount);

    if (renderingSetup.m_bDiscardColor)
    {
      colorAttachment.InitialState = Diligent::RESOURCE_STATE_UNDEFINED;
      colorAttachment.LoadOp       = Diligent::ATTACHMENT_LOAD_OP_DISCARD;
    }
    else
    {
      if (renderingSetup.m_uiRenderTargetClearMask & XII_BIT(i))
      {
        colorAttachment.InitialState = Diligent::RESOURCE_STATE_UNDEFINED;
        colorAttachment.LoadOp       = Diligent::ATTACHMENT_LOAD_OP_CLEAR;
      }
      else
      {
        colorAttachment.InitialState = Diligent::RESOURCE_STATE_RENDER_TARGET;
        colorAttachment.LoadOp       = Diligent::ATTACHMENT_LOAD_OP_LOAD;
      }
    }

    colorAttachment.StoreOp        = Diligent::ATTACHMENT_STORE_OP_STORE;
    colorAttachment.StencilLoadOp  = Diligent::ATTACHMENT_LOAD_OP_DISCARD;
    colorAttachment.StencilStoreOp = Diligent::ATTACHMENT_STORE_OP_DISCARD;
  }
}

void xiiGALPassDiligent::GetFrameBufferDesc(Diligent::IRenderPass* pRenderPass, const xiiGALRenderTargetSetup, Diligent::FramebufferDesc desc)
{
}

void xiiGALPassDiligent::CreateFramebuffer(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  Diligent::IRenderPass*          pRenderPass    = GetRenderPass(renderingSetup);
  const Diligent::RenderPassDesc& renderPassDesc = pRenderPass->GetDesc();

  Diligent::FramebufferDesc framebufferDesc = {};
  framebufferDesc.Name                      = szName;
  framebufferDesc.pRenderPass               = pRenderPass;
  framebufferDesc.AttachmentCount           = renderPassDesc.AttachmentCount;

  const bool      bHasDepth              = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt32 uiColorAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

  xiiHybridArray<Diligent::ITextureView*, 2> framebufferAttachments;

  if (bHasDepth)
  {
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent =
      static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();

    xiiVec3U32 size                = pTextureDiligent->GetMipLevelSize(pRenderTargetViewDiligent->GetDescription().m_uiMipLevel);
    framebufferDesc.Width          = size.x;
    framebufferDesc.Height         = size.y;
    framebufferDesc.NumArraySlices = textureDescription.m_uiArraySize;

    framebufferAttachments.PushBack(const_cast<xiiGALRenderTargetViewDiligent*>(pRenderTargetViewDiligent)->GetDepthStencilView());
  }

  for (xiiUInt32 uiColorAttachmentIndex = 0; uiColorAttachmentIndex < uiColorAttachmentCount; ++uiColorAttachmentIndex)
  {
    xiiGALRenderTargetViewHandle          hColorRenderTarget        = renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(uiColorAttachmentIndex));
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(hColorRenderTarget));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();

    xiiVec3U32 size                = pTextureDiligent->GetMipLevelSize(pRenderTargetViewDiligent->GetDescription().m_uiMipLevel);
    framebufferDesc.Width          = size.x;
    framebufferDesc.Height         = size.y;
    framebufferDesc.NumArraySlices = textureDescription.m_uiArraySize;

    framebufferAttachments.PushBack(const_cast<xiiGALRenderTargetViewDiligent*>(pRenderTargetViewDiligent)->GetRenderTargetView());
  }

  // In some places rendering is started with an empty xiiGALRenderTargetSetup just to be able to run GPU commands.
  // An empty size is invalid in Vulkan so we just set it so 1,1.
  if (xiiVec2U32(framebufferDesc.Width, framebufferDesc.Height) == xiiVec2U32(0, 0))
  {
    framebufferDesc.Width          = 1;
    framebufferDesc.Height         = 1;
    framebufferDesc.NumArraySlices = 1;
  }

  framebufferDesc.ppAttachments   = framebufferAttachments.GetData();
  framebufferDesc.AttachmentCount = framebufferAttachments.GetCount();

  Diligent::IFramebuffer* pFramebuffer = nullptr;

  m_GALDeviceDiligent.GetDevice()->CreateFramebuffer(framebufferDesc, &pFramebuffer);

  FramebufferInfo wrapper{pFramebuffer};
  m_Framebuffers.Insert(renderingSetup, wrapper);

  XII_ASSERT_DEV(pFramebuffer != nullptr, "Failed to create frame buffer for {0}", szName);

  if (pFramebuffer == nullptr)
  {
    xiiLog::Error("Failed to create frame buffer for '{0}'", szName);
  }
}

xiiUInt32 xiiGALPassDiligent::ResourceCacheHash::Hash(const xiiGALRenderingSetup& renderingSetup)
{
  xiiHashStreamWriter32 writer;
  writer << renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();

  const xiiUInt8 uiCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  for (xiiUInt8 i = 0; i < uiCount; ++i)
  {
    writer << renderingSetup.m_RenderTargetSetup.GetRenderTarget(i);
  }

  writer << renderingSetup.m_ClearColor;
  writer << renderingSetup.m_uiRenderTargetClearMask;
  writer << renderingSetup.m_fDepthClear;
  writer << renderingSetup.m_uiStencilClear;
  writer << renderingSetup.m_bClearDepth;
  writer << renderingSetup.m_bClearStencil;
  writer << renderingSetup.m_bDiscardColor;
  writer << renderingSetup.m_bDiscardDepth;

  return writer.GetHashValue();
}

bool xiiGALPassDiligent::ResourceCacheHash::Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b)
{
  return a == b;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Device_Implementation_PassDiligent);
