#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/Pass.h>

xiiVec3U32 GetMipLevelSize(xiiUInt32 uiMipLevelSize, const xiiGALTextureCreationDescription& textureDescription)
{
  xiiVec3U32 size = {textureDescription.m_Size.width, textureDescription.m_Size.height, textureDescription.m_uiArraySizeOrDepth};
  size.x          = xiiMath::Max(1u, size.x >> uiMipLevelSize);
  size.y          = xiiMath::Max(1u, size.y >> uiMipLevelSize);
  size.z          = xiiMath::Max(1u, size.z >> uiMipLevelSize);
  return size;
}

xiiGALPass::xiiGALPass(xiiGALDevice& device) :
  m_Device(device)
{
}

xiiGALPass::~xiiGALPass() = default;

xiiGALGraphicsCommandEncoder* xiiGALPass::BeginRendering(const xiiGALRenderPassCreationDescription& renderPassDescription)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "Command Encoder nesting is not permitted.");

  xiiGALRenderPass*  pRenderPass  = nullptr;
  xiiGALFramebuffer* pFramebuffer = nullptr;
  GetRenderPassAndFramebuffer(renderPassDescription, pRenderPass, pFramebuffer);

  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Graphics;

  xiiGALGraphicsCommandEncoder* pCommandEncoder = BeginRenderingPlatform(nullptr, nullptr, renderPassDescription.m_sName);

  if (!renderPassDescription.m_sName.IsEmpty())
  {
    pCommandEncoder->PushMarker(renderPassDescription.m_sName);

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

void xiiGALPass::GetRenderPassAndFramebuffer(const xiiGALRenderPassCreationDescription& renderPassDescription, xiiGALRenderPass* out_pRenderPass, xiiGALFramebuffer* out_pFramebuffer)
{
  const xiiUInt32 uiHash = renderPassDescription.CalculateHash();

  // Create or retrieve render pass.

  RenderPassFrameBufferInfo renderPassFramebufferInfo;
  if (!m_RenderPassFramebufferCache.TryGetValue(uiHash, renderPassFramebufferInfo))
  {
    renderPassFramebufferInfo.hRenderPass = m_Device.CreateRenderPass(renderPassDescription);

    // Create new framebuffer.

    xiiGALFramebufferCreationDescription framebufferDescription;
    framebufferDescription.m_hRenderPass = renderPassFramebufferInfo.hRenderPass;

    const xiiUInt32 uiAttachmentCount = renderPassDescription.m_Attachments.GetCount();
    framebufferDescription.m_Attachments.Reserve(uiAttachmentCount);

    for (xiiUInt32 i = 0; i < uiAttachmentCount; ++i)
    {
      const auto& attachment            = renderPassDescription.m_Attachments[i];
      auto&       framebufferAttachment = framebufferDescription.m_Attachments[i];
    }

    renderPassFramebufferInfo.hFrameBuffer = m_Device.CreateFramebuffer(framebufferDescription);

    XII_VERIFY(!m_RenderPassFramebufferCache.Insert(uiHash, renderPassFramebufferInfo), "Render pass description key hash collision!");
  }

  out_pRenderPass  = m_Device.GetRenderPass(renderPassFramebufferInfo.hRenderPass);
  out_pFramebuffer = m_Device.GetFramebuffer(renderPassFramebufferInfo.hFrameBuffer);
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_Pass);
