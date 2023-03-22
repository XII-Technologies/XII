#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/CommandEncoder/CommandEncoderImplDiligent.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Device/PassDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
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
  xiiGALPass(device), m_GALDeviceDiligent(static_cast<xiiGALDeviceDiligent&>(device))
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderRenderState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderImplDiligent, m_GALDeviceDiligent);

  m_pRenderCommandEncoder  = XII_DEFAULT_NEW(xiiGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

xiiGALPassDiligent::~xiiGALPassDiligent()
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

xiiGALRenderCommandEncoder* xiiGALPassDiligent::BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  if (m_RenderingSetup != renderingSetup)
  {
    m_RenderingSetup = renderingSetup;
  }

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

xiiGALRenderCommandEncoder* xiiGALPassDiligent::BeginRenderPassPlatform()
{
  RenderPassWrapper renderPass;
  if (!m_RenderPasses.TryGetValue(m_RenderingSetup, renderPass))
  {
    xiiLog::Info("Creating Renderpass #{}", m_RenderPasses.GetCount());

    xiiStringBuilder sName;
    m_sName.GetData(sName);

    CreateRenderPass(m_RenderingSetup, sName.GetData());
  }
  XII_ASSERT_DEV(m_RenderPasses.TryGetValue(m_RenderingSetup, renderPass), "Failed to retrieve render pass, this should have been successful.");

  FramebufferWrapper frameBuffer;
  if (!m_Framebuffers.TryGetValue(m_RenderingSetup, frameBuffer))
  {
    xiiLog::Info("Creating Framebuffer #{}", m_Framebuffers.GetCount());

    xiiStringBuilder sName;
    m_sName.GetData(sName);

    CreateFramebuffer(m_RenderingSetup, sName.GetData());
  }
  XII_ASSERT_DEV(m_Framebuffers.TryGetValue(m_RenderingSetup, frameBuffer), "Failed to retrieve frame buffer, this should have been successful.");

  const Diligent::FramebufferDesc& framebufferDesc = frameBuffer.m_pFramebuffer->GetDesc();
  m_pRenderCommandEncoder->SetScissorRect(xiiRectU32(framebufferDesc.Width, framebufferDesc.Height));

  m_ClearValues.Clear();

  Diligent::BeginRenderPassAttribs renderPassBeginInfo = {};
  renderPassBeginInfo.pRenderPass                      = renderPass.m_pRenderPass;
  renderPassBeginInfo.pFramebuffer                     = frameBuffer.m_pFramebuffer;

  const bool      bHasDepth    = !m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt32 uiColorCount = m_RenderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

  if (bHasDepth)
  {
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent =
      static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format             = textureDescription.m_Format;
    const auto&                             formatInfo         = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(format);

    Diligent::OptimizedClearValue& depthClear = m_ClearValues.ExpandAndGetRef();
    depthClear.Format                         = formatInfo.m_eDepthStencilType;
    depthClear.DepthStencil.Depth             = 1.0f;
    depthClear.DepthStencil.Stencil           = 0;
  }

  for (xiiUInt32 i = 0; i < uiColorCount; ++i)
  {
    xiiGALRenderTargetViewHandle          hColorRenderTarget        = m_RenderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i));
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(hColorRenderTarget));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format             = textureDescription.m_Format;
    const auto&                             formatInfo         = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(format);

    Diligent::OptimizedClearValue& colorClear = m_ClearValues.ExpandAndGetRef();
    colorClear.Format                         = formatInfo.m_eRenderTarget;
    colorClear.Color[0]                       = m_RenderingSetup.m_ClearColor.r;
    colorClear.Color[1]                       = m_RenderingSetup.m_ClearColor.g;
    colorClear.Color[2]                       = m_RenderingSetup.m_ClearColor.b;
    colorClear.Color[3]                       = m_RenderingSetup.m_ClearColor.a;
  }

  renderPassBeginInfo.pClearValues        = m_ClearValues.GetData();
  renderPassBeginInfo.ClearValueCount     = m_ClearValues.GetCount();
  renderPassBeginInfo.StateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pCommandEncoderImpl->TransitionResourceStates();

  m_GALDeviceDiligent.GetImmediateContext()->BeginRenderPass(renderPassBeginInfo);

#if 0

  // Clear Render Target
  if (m_RenderingSetup.m_uiRenderTargetClearMask != 0)
  {
    for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
    {
      if (m_RenderingSetup.m_uiRenderTargetClearMask & (1u << i) && i < uiColorCount)
      {
        xiiGALRenderTargetViewHandle hColorRenderTarget = m_RenderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i));

        xiiGALRenderTargetView*         pGALRenderTargetView      = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(hColorRenderTarget));
        xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<xiiGALRenderTargetViewDiligent*>(pGALRenderTargetView);

        m_pCommandEncoderImpl->m_pContext->ClearRenderTarget(pRenderTargetViewDiligent->GetRenderTargetView(), m_RenderingSetup.m_ClearColor.GetData(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      }
    }
  }

  if (bHasDepth && (m_RenderingSetup.m_bClearDepth || m_RenderingSetup.m_bClearStencil))
  {
    xiiGALRenderTargetView*         pGALDepthStencilView      = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));
    xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<xiiGALRenderTargetViewDiligent*>(pGALDepthStencilView);

    Diligent::CLEAR_DEPTH_STENCIL_FLAGS flags = Diligent::CLEAR_DEPTH_FLAG_NONE;

    if (m_RenderingSetup.m_bClearDepth)
      flags |= Diligent::CLEAR_DEPTH_FLAG;

    if (m_RenderingSetup.m_bClearStencil)
      flags |= Diligent::CLEAR_STENCIL_FLAG;

    m_pCommandEncoderImpl->m_pContext->ClearDepthStencil(pRenderTargetViewDiligent->GetDepthStencilView(), flags, m_RenderingSetup.m_fDepthClear, m_RenderingSetup.m_uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }
#endif

  return m_pRenderCommandEncoder.Borrow();
}

void xiiGALPassDiligent::EndRenderPassPlatform()
{
  m_GALDeviceDiligent.GetImmediateContext()->EndRenderPass();
}

void xiiGALPassDiligent::MarkDirty()
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

  m_ClearValues.Clear();

  m_pCommandEncoderImpl->MarkDirty();
}

void xiiGALPassDiligent::Reset()
{
  m_pCommandEncoderImpl->Reset();
  m_pRenderCommandEncoder->InvalidateState();
  m_pComputeCommandEncoder->InvalidateState();
}

void xiiGALPassDiligent::CreateRenderPass(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  // Populate Render Pass Description
  const bool      bHasDepthTarget        = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt32 uiColorAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

  xiiHybridArray<Diligent::RenderPassAttachmentDesc, 2> Attachments;

  if (bHasDepthTarget)
  {
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent =
      static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format             = textureDescription.m_Format;
    const auto&                             formatInfo         = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(format);

    Diligent::RenderPassAttachmentDesc& depthAttachment = Attachments.ExpandAndGetRef();
    depthAttachment.Format                              = formatInfo.m_eDepthStencilType;
    // \todo Handle format overrides

    depthAttachment.SampleCount = xiiDiligentUtils::ToDiligentMSAACount(textureDescription.m_SampleCount);

    if (renderingSetup.m_bDiscardDepth)
    {
      depthAttachment.InitialState = Diligent::RESOURCE_STATE_DEPTH_WRITE;
      depthAttachment.LoadOp       = Diligent::ATTACHMENT_LOAD_OP_DISCARD;
    }
    else
    {
      depthAttachment.InitialState = Diligent::RESOURCE_STATE_DEPTH_WRITE;
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

    Diligent::RenderPassAttachmentDesc& colorAttachment = Attachments.ExpandAndGetRef();
    colorAttachment.Format                              = formatInfo.m_eRenderTarget;
    // \todo Handle format overrides

    colorAttachment.SampleCount = xiiDiligentUtils::ToDiligentMSAACount(textureDescription.m_SampleCount);

    if (renderingSetup.m_bDiscardColor)
    {
      colorAttachment.InitialState = Diligent::RESOURCE_STATE_RENDER_TARGET;
      colorAttachment.LoadOp       = Diligent::ATTACHMENT_LOAD_OP_DISCARD;
    }
    else
    {
      if (renderingSetup.m_uiRenderTargetClearMask & (1u << i))
      {
        colorAttachment.InitialState = Diligent::RESOURCE_STATE_RENDER_TARGET;
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

  // Create Render Pass References
  xiiHybridArray<Diligent::AttachmentReference, 1> depthAttachmentReferences;
  xiiHybridArray<Diligent::AttachmentReference, 4> colorAttachmentReferences;

  const xiiUInt32 uiAttachmentCount = Attachments.GetCount();
  for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < uiAttachmentCount; ++uiAttachmentIndex)
  {
    Diligent::RenderPassAttachmentDesc& attachment = Attachments[uiAttachmentIndex];

    const bool bIsDepthFormat = xiiDiligentUtils::IsDepthFormat(attachment.Format);
    if (bIsDepthFormat)
    {
      attachment.FinalState = Diligent::RESOURCE_STATE_DEPTH_WRITE;

      Diligent::AttachmentReference& depthAttachmentRef = depthAttachmentReferences.ExpandAndGetRef();
      depthAttachmentRef.State                          = Diligent::RESOURCE_STATE_DEPTH_WRITE;
      depthAttachmentRef.AttachmentIndex                = uiAttachmentIndex;
    }
    else
    {
      attachment.FinalState = Diligent::RESOURCE_STATE_RENDER_TARGET;

      Diligent::AttachmentReference& colorAttachmentRef = colorAttachmentReferences.ExpandAndGetRef();
      colorAttachmentRef.State                          = Diligent::RESOURCE_STATE_RENDER_TARGET;
      colorAttachmentRef.AttachmentIndex                = uiAttachmentIndex;
    }
  }

  XII_ASSERT_DEV(depthAttachmentReferences.GetCount() <= 1u, "There must be at most one depth attachment.");

  const bool bHasColorAttachment = !colorAttachmentReferences.IsEmpty();
  const bool bHasDepthAttachment = !depthAttachmentReferences.IsEmpty();

  Diligent::SubpassDesc subpassDescription       = {};
  subpassDescription.pRenderTargetAttachments    = bHasColorAttachment ? colorAttachmentReferences.GetData() : nullptr;
  subpassDescription.pDepthStencilAttachment     = bHasDepthAttachment ? depthAttachmentReferences.GetData() : nullptr;
  subpassDescription.RenderTargetAttachmentCount = colorAttachmentReferences.GetCount();

  Diligent::SubpassDependencyDesc subpassDependency = {};
  subpassDependency.SrcSubpass                      = 0u;
  subpassDependency.DstSubpass                      = 0u;

  if (bHasColorAttachment)
    subpassDependency.DstAccessMask |= Diligent::ACCESS_FLAG_RENDER_TARGET_WRITE;

  if (bHasDepthAttachment)
    subpassDependency.DstAccessMask |= Diligent::ACCESS_FLAG_DEPTH_STENCIL_WRITE;

  subpassDependency.DstStageMask  = Diligent::PIPELINE_STAGE_FLAG_RENDER_TARGET | Diligent::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS;
  subpassDependency.SrcSubpass    = Diligent::SUBPASS_EXTERNAL;
  subpassDependency.SrcAccessMask = {};
  subpassDependency.SrcStageMask  = Diligent::PIPELINE_STAGE_FLAG_RENDER_TARGET | Diligent::PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS;

  Diligent::RenderPassDesc renderPassDesc = {};
  renderPassDesc.Name                     = szName;
  renderPassDesc.AttachmentCount          = Attachments.GetCount();
  renderPassDesc.pAttachments             = Attachments.GetData();
  renderPassDesc.SubpassCount             = 1u;
  renderPassDesc.pSubpasses               = &subpassDescription;
  renderPassDesc.DependencyCount          = 1u;
  renderPassDesc.pDependencies            = &subpassDependency;

  Diligent::IRenderPass* pRenderPass = nullptr;

  m_GALDeviceDiligent.GetDevice()->CreateRenderPass(renderPassDesc, &pRenderPass);

  RenderPassWrapper wrapper{pRenderPass};
  m_RenderPasses.Insert(renderingSetup, wrapper);

  XII_ASSERT_DEV(pRenderPass != nullptr, "Failed to create render pass for {0}", szName);

  if (pRenderPass == nullptr)
  {
    xiiLog::Error("Failed to create render pass for '{0}'", szName);
  }
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

  FramebufferWrapper wrapper{pFramebuffer};
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
