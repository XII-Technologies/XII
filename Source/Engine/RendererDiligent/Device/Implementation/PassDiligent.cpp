#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/CommandEncoder/CommandEncoderImplDiligent.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Device/PassDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

xiiGALPassDiligent::xiiGALPassDiligent(xiiGALDevice& device) :
  xiiGALPass(device), m_GALDeviceDiligent(static_cast<xiiGALDeviceDiligent&>(device))
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderRenderState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderImplDiligent, m_GALDeviceDiligent);

  m_pRenderCommandEncoder  = XII_DEFAULT_NEW(xiiGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

xiiGALPassDiligent::~xiiGALPassDiligent() = default;

xiiGALRenderCommandEncoder* xiiGALPassDiligent::BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  // Begin render pass setup
  const xiiUInt32 uiAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

  for (xiiUInt32 i = 0; i < uiAttachmentCount; ++i)
  {
    Diligent::RenderPassAttachmentDesc renderPassDesc = {};
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

void xiiGALPassDiligent::MarkDirty()
{
  m_pCommandEncoderImpl->MarkDirty();
}

void xiiGALPassDiligent::Reset()
{
  // m_pCommandEncoderImpl->Reset();
  m_pRenderCommandEncoder->InvalidateState();
  m_pComputeCommandEncoder->InvalidateState();
}

void xiiGALPassDiligent::CreateRenderPass(const xiiGALRenderingSetup& renderingSetup)
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
    depthAttachment.Format                              = formatInfo.m_eRenderTarget;
    // \todo Handle format overrides

    depthAttachment.SampleCount = xiiDiligentUtils::ToDiligentMSAACount(textureDescription.m_SampleCount);

    if (renderingSetup.m_bDiscardDepth)
    {
      depthAttachment.InitialState = Diligent::RESOURCE_STATE_UNDEFINED;
      depthAttachment.LoadOp       = Diligent::ATTACHMENT_LOAD_OP_DISCARD;
    }
    else
    {
      depthAttachment.InitialState = renderingSetup.m_bClearDepth ? Diligent::RESOURCE_STATE_UNDEFINED : (Diligent::RESOURCE_STATE_DEPTH_READ | Diligent::RESOURCE_STATE_DEPTH_WRITE);
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

    depthAttachment.FinalState = (Diligent::RESOURCE_STATE_DEPTH_READ | Diligent::RESOURCE_STATE_DEPTH_WRITE);
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
      colorAttachment.InitialState = Diligent::RESOURCE_STATE_UNDEFINED;
      colorAttachment.LoadOp       = Diligent::ATTACHMENT_LOAD_OP_DISCARD;
    }
    else
    {
      if (renderingSetup.m_uiRenderTargetClearMask & (1u << i))
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
    colorAttachment.FinalState     = Diligent::RESOURCE_STATE_RENDER_TARGET;
  }

  // Create Render Pass References
  xiiHybridArray<Diligent::AttachmentReference, 1> DepthAttachmentReferences;
  xiiHybridArray<Diligent::AttachmentReference, 4> ColorAttachmentReferences;

}
