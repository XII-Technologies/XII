#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <GraphicsFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/Pass.h>

xiiVec3U32 GetMipLevelSize(xiiUInt32 uiMipLevelSize, const xiiGALTextureCreationDescription& textureDescription)
{
  xiiVec3U32 size = {textureDescription.m_Size.width, textureDescription.m_Size.height, textureDescription.m_uiArraySizeOrDepth};
  size.x          = xiiMath::Max(1U, size.x >> uiMipLevelSize);
  size.y          = xiiMath::Max(1U, size.y >> uiMipLevelSize);
  size.z          = xiiMath::Max(1U, size.z >> uiMipLevelSize);
  return size;
}

xiiGALPass::xiiGALPass(xiiGALDevice& device) :
  m_Device(device)
{
}

xiiGALPass::~xiiGALPass() = default;

xiiGALGraphicsCommandEncoder* xiiGALPass::BeginRendering(const xiiGALRenderingSetup& renderingSetup, xiiStringView sName /* = {} */)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "Command Encoder nesting is not permitted.");

  xiiGALRenderPass*  pRenderPass  = nullptr;
  xiiGALFramebuffer* pFramebuffer = nullptr;
  GetRenderPassAndFramebuffer(renderingSetup, pRenderPass, pFramebuffer);

  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Graphics;

  xiiGALGraphicsCommandEncoder* pCommandEncoder = BeginRenderingPlatform(pRenderPass, pFramebuffer, sName);

  if (!sName.IsEmpty())
  {
    pCommandEncoder->PushMarker(sName);

    m_bMarkerPushed = true;
  }

  return pCommandEncoder;
}

void xiiGALPass::EndRendering(xiiGALGraphicsCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "BeginRendering has not been called.");

  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Invalid;

  if (m_bMarkerPushed)
  {
    pCommandEncoder->PopMarker();

    m_bMarkerPushed = false;
  }

  EndRenderingPlatform(pCommandEncoder);
}

xiiGALComputeCommandEncoder* xiiGALPass::BeginCompute(xiiStringView sName)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "Command Encoder nesting is not permitted.");

  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Compute;

  xiiGALComputeCommandEncoder* pCommandEncoder = BeginComputePlatform(sName);

  if (!sName.IsEmpty())
  {
    pCommandEncoder->PushMarker(sName);

    m_bMarkerPushed = true;
  }

  return pCommandEncoder;
}

void xiiGALPass::EndCompute(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "BeginCompute has not been called.");

  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Invalid;

  if (m_bMarkerPushed)
  {
    pCommandEncoder->PopMarker();

    m_bMarkerPushed = false;
  }

  EndComputePlatform(pCommandEncoder);
}

void xiiGALPass::GetRenderPassAndFramebuffer(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPass* out_pRenderPass, xiiGALFramebuffer* out_pFramebuffer)
{
  RenderPassFrameBufferInfo frameBufferInfo;
  if (!m_FramebufferCache.TryGetValue(renderingSetup, frameBufferInfo))
  {
    // Retrieve an existing render pass handle if any, a new one is created otherwise.

    xiiGALRenderPassHandle hRenderPass;
    if (!m_RenderPassCache.TryGetValue(renderingSetup.m_RenderTargetSetup, hRenderPass))
    {
      xiiGALRenderPassCreationDescription renderPassDescription;

      const bool      bHasDepthAttachment    = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
      const xiiUInt32 uiColorAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

      // Build render pass description.
      {
        if (bHasDepthAttachment)
        {
          xiiGALTexture*                          pDepthTexture      = m_Device.GetTextureView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget())->GetTexture();
          const xiiGALTextureCreationDescription& textureDescription = pDepthTexture->GetDescription();

          xiiGALRenderPassAttachmentDescription& attachmentReference = renderPassDescription.m_Attachments.ExpandAndGetRef();
          attachmentReference.m_Format                               = textureDescription.m_Format;
          attachmentReference.m_uiSampleCount                        = static_cast<xiiUInt8>(textureDescription.m_uiSampleCount);
          attachmentReference.m_InitialStateFlags                    = xiiGALResourceStateFlags::DepthWrite;
          attachmentReference.m_FinalStateFlags                      = xiiGALResourceStateFlags::DepthWrite;

          if (renderingSetup.m_bDiscardDepth)
          {
            attachmentReference.m_LoadOperation = xiiGALAttachmentLoadOperation::Discard;
          }
          else
          {
            attachmentReference.m_LoadOperation = renderingSetup.m_bClearDepth ? xiiGALAttachmentLoadOperation::Clear : xiiGALAttachmentLoadOperation::Load;
          }
          attachmentReference.m_StoreOperation = xiiGALAttachmentStoreOperation::Store;

          if (textureDescription.m_Format.IsStencilFormat(textureDescription.m_Format))
          {
            attachmentReference.m_StencilLoadOperation  = renderingSetup.m_bClearStencil ? xiiGALAttachmentLoadOperation::Clear : xiiGALAttachmentLoadOperation::Load;
            attachmentReference.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Store;
          }
          else
          {
            attachmentReference.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Discard;
            attachmentReference.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Discard;
          }
        }

        for (xiiUInt32 i = 0; i < uiColorAttachmentCount; ++i)
        {
          xiiGALTexture*                          pColourTexture     = m_Device.GetTextureView(renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i)))->GetTexture();
          const xiiGALTextureCreationDescription& textureDescription = pColourTexture->GetDescription();

          xiiGALRenderPassAttachmentDescription& attachmentReference = renderPassDescription.m_Attachments.ExpandAndGetRef();
          attachmentReference.m_Format                               = textureDescription.m_Format;
          attachmentReference.m_uiSampleCount                        = static_cast<xiiUInt8>(textureDescription.m_uiSampleCount);
          attachmentReference.m_InitialStateFlags                    = xiiGALResourceStateFlags::RenderTarget;
          attachmentReference.m_FinalStateFlags                      = xiiGALResourceStateFlags::RenderTarget;

          if (renderingSetup.m_bDiscardColor)
          {
            attachmentReference.m_LoadOperation = xiiGALAttachmentLoadOperation::Discard;
          }
          else
          {
            if (renderingSetup.m_uiRenderTargetClearMask & XII_BIT(i))
            {
              attachmentReference.m_LoadOperation = xiiGALAttachmentLoadOperation::Clear;
            }
            else
            {
              attachmentReference.m_LoadOperation = xiiGALAttachmentLoadOperation::Load;
            }
          }

          attachmentReference.m_StoreOperation        = xiiGALAttachmentStoreOperation::Store;
          attachmentReference.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Discard;
          attachmentReference.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Discard;
        }
      }

      // Build render pass attachment description.
      {
        xiiHybridArray<xiiGALAttachmentReferenceDescription, 1U> depthAttachmentRefs;
        xiiHybridArray<xiiGALAttachmentReferenceDescription, 4U> colorAttachmentRefs;

        const xiiUInt32 uiAttachmentCount = renderPassDescription.m_Attachments.GetCount();
        for (xiiUInt32 i = 0; i < uiAttachmentCount; ++i)
        {
          auto& attachment = renderPassDescription.m_Attachments[i];

          const bool bIsDepthAttachment = xiiGALTextureFormat::IsDepthFormat(attachment.m_Format);
          if (bIsDepthAttachment)
          {
            attachment.m_FinalStateFlags = xiiGALResourceStateFlags::DepthWrite;

            xiiGALAttachmentReferenceDescription& attachmentRef = depthAttachmentRefs.ExpandAndGetRef();
            attachmentRef.m_uiAttachmentIndex                   = i;
            attachmentRef.m_ResourceStateFlags                  = xiiGALResourceStateFlags::DepthWrite;
          }
          else
          {
            attachment.m_FinalStateFlags = xiiGALResourceStateFlags::RenderTarget;

            xiiGALAttachmentReferenceDescription& attachmentRef = colorAttachmentRefs.ExpandAndGetRef();
            attachmentRef.m_uiAttachmentIndex                   = i;
            attachmentRef.m_ResourceStateFlags                  = xiiGALResourceStateFlags::RenderTarget;
          }
        }

        XII_ASSERT_DEV(depthAttachmentRefs.GetCount() <= 1U, "There can only be a maximum of 1 bound depth attachment.");

        xiiGALSubPassDescription& subpassDescription = renderPassDescription.m_SubPasses.ExpandAndGetRef();
        subpassDescription.m_RenderTargetAttachments = colorAttachmentRefs;
        subpassDescription.m_DepthStencilAttachment  = depthAttachmentRefs;

        xiiGALSubPassDependencyDescription& subpassDependency = renderPassDescription.m_Dependencies.ExpandAndGetRef();
        subpassDependency.m_uiSourceSubPass                   = XII_GAL_SUBPASS_EXTERNAL;
        subpassDependency.m_uiDestinationSubPass              = 0;
      }

      hRenderPass = m_Device.CreateRenderPass(renderPassDescription);
      XII_VERIFY(!m_RenderPassCache.Insert(renderingSetup.m_RenderTargetSetup, hRenderPass), "Overwrote existing render pass, this is unexpected behaviour.");
    }

    XII_ASSERT_DEV(!hRenderPass.IsInvalidated(), "Render pass handle is invalidated!");

    // Since no framebuffer was retrieved, a new one needs to be created.
    {
      xiiGALFramebufferCreationDescription frameBufferDescription;
      frameBufferDescription.m_hRenderPass = hRenderPass;

      // Framebuffer size and slice count are intentionally left unattended to be filled by the GAL Implementation.

      const bool      bHasDepthAttachment    = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
      const xiiUInt32 uiColorAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

      if (bHasDepthAttachment)
      {
        xiiGALTexture*                          pDepthTexture      = m_Device.GetTextureView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget())->GetTexture();
        const xiiGALTextureCreationDescription& textureDescription = pDepthTexture->GetDescription();

        frameBufferDescription.m_Attachments.PushBack(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget());
      }

      for (xiiUInt32 i = 0; i < uiColorAttachmentCount; ++i)
      {
        xiiGALTexture*                          pColourTexture     = m_Device.GetTextureView(renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i)))->GetTexture();
        const xiiGALTextureCreationDescription& textureDescription = pColourTexture->GetDescription();

        frameBufferDescription.m_Attachments.PushBack(renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i)));
      }

      frameBufferInfo.hRenderPass  = hRenderPass;
      frameBufferInfo.hFrameBuffer = m_Device.CreateFramebuffer(frameBufferDescription);
    }
  }

  XII_ASSERT_DEV(!frameBufferInfo.hRenderPass.IsInvalidated(), "Framebuffer handle is invalidated!");
  XII_ASSERT_DEV(!frameBufferInfo.hFrameBuffer.IsInvalidated(), "Framebuffer handle is invalidated!");

  out_pRenderPass  = m_Device.GetRenderPass(frameBufferInfo.hRenderPass);
  out_pFramebuffer = m_Device.GetFramebuffer(frameBufferInfo.hFrameBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Resource Cache Hash

XII_CHECK_AT_COMPILETIME(sizeof(xiiUInt32) == sizeof(xiiGALTextureViewHandle));
namespace
{
  XII_ALWAYS_INLINE xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiGALTextureViewHandle& value)
  {
    stream << reinterpret_cast<const xiiUInt32&>(value);
    return stream;
  }
} // namespace

xiiUInt32 xiiGALPass::ResourceCacheHash::Hash(const xiiGALRenderTargetSetup& renderTargetSetup)
{
  xiiHashStreamWriter32 writer;
  writer << renderTargetSetup.GetDepthStencilTarget();

  const xiiUInt8 uiCount = renderTargetSetup.GetRenderTargetCount();
  writer << uiCount;

  for (xiiUInt8 i = 0; i < uiCount; ++i)
  {
    writer << renderTargetSetup.GetRenderTarget(i);
  }

  return writer.GetHashValue();
}

bool xiiGALPass::ResourceCacheHash::Equal(const xiiGALRenderTargetSetup& a, const xiiGALRenderTargetSetup& b)
{
  return a == b;
}

xiiUInt32 xiiGALPass::ResourceCacheHash::Hash(const xiiGALRenderingSetup& renderingSetup)
{
  xiiHashStreamWriter32 writer;
  writer << renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();

  const xiiUInt8 uiCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  writer << uiCount;

  for (xiiUInt8 i = 0; i < uiCount; ++i)
  {
    writer << renderingSetup.m_RenderTargetSetup.GetRenderTarget(i);
  }

  writer << renderingSetup.m_uiRenderTargetClearMask;
  writer << renderingSetup.m_bClearDepth;
  writer << renderingSetup.m_bClearStencil;
  writer << renderingSetup.m_bDiscardColor;
  writer << renderingSetup.m_bDiscardDepth;

  return writer.GetHashValue();
}

bool xiiGALPass::ResourceCacheHash::Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b)
{
  return a == b;
}

///////////////////////////////////////////////////////////////////////////////////////////////

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_Pass);
