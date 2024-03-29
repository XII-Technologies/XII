#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/RenderPassD3D11.h>

xiiGALRenderPassD3D11::xiiGALRenderPassD3D11(const xiiGALRenderPassCreationDescription& creationDescription) :
  xiiGALRenderPass(creationDescription)
{
}

xiiGALRenderPassD3D11::~xiiGALRenderPassD3D11() = default;

xiiResult xiiGALRenderPassD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  Diligent::RenderPassDesc renderPassDescription;

  // Render pass attachments.

  const xiiUInt32 uiAttachmentCount = m_Description.m_Attachments.GetCount();

  xiiHybridArray<Diligent::RenderPassAttachmentDesc, 16U> attachments;
  attachments.SetCount(uiAttachmentCount);

  for (xiiUInt32 i = 0; i < uiAttachmentCount; ++i)
  {
    const auto& xiiAttachment = m_Description.m_Attachments[i];
    auto&       attachment    = attachments[i];

    attachment.Format         = xiiDiligentTypeConversions::GetTextureFormat(xiiAttachment.m_Format);
    attachment.SampleCount    = xiiAttachment.m_uiSampleCount;
    attachment.LoadOp         = xiiDiligentTypeConversions::GetLoadOperation(xiiAttachment.m_LoadOperation);
    attachment.StoreOp        = xiiDiligentTypeConversions::GetStoreOperation(xiiAttachment.m_StoreOperation);
    attachment.StencilLoadOp  = xiiDiligentTypeConversions::GetLoadOperation(xiiAttachment.m_StencilLoadOperation);
    attachment.StencilStoreOp = xiiDiligentTypeConversions::GetStoreOperation(xiiAttachment.m_StencilStoreOperation);
    attachment.InitialState   = xiiDiligentTypeConversions::GetResourceState(xiiAttachment.m_InitialStateFlags);
    attachment.FinalState     = xiiDiligentTypeConversions::GetResourceState(xiiAttachment.m_FinalStateFlags);
  }
  renderPassDescription.AttachmentCount = uiAttachmentCount;
  renderPassDescription.pAttachments    = attachments.GetData();

  // Subpasses.

  const xiiUInt32 uiSubpassCount = m_Description.m_SubPasses.GetCount();

  xiiHybridArray<Diligent::SubpassDesc, 16U> subpasses;
  subpasses.SetCount(uiSubpassCount);

  xiiHybridArray<xiiHybridArray<Diligent::AttachmentReference, 16U>, 16U> perSubpassInputAttachments, perSubpassRenderTargetAttachments, perSubpassResolveAttachments;
  perSubpassInputAttachments.SetCount(uiSubpassCount);
  perSubpassRenderTargetAttachments.SetCount(uiSubpassCount);
  perSubpassResolveAttachments.SetCount(uiSubpassCount);

  xiiHybridArray<Diligent::AttachmentReference, 16U> perSubpassDepthStencilAttachments;
  perSubpassDepthStencilAttachments.SetCount(uiSubpassCount);

  xiiHybridArray<Diligent::ShadingRateAttachment, 16U> perSubpassShadingRateAttachments;
  perSubpassShadingRateAttachments.SetCount(uiSubpassCount);

  for (xiiUInt32 i = 0; i < uiSubpassCount; ++i)
  {
    const auto& xiiSubpass                     = m_Description.m_SubPasses[i];
    auto&       subpass                        = subpasses[i];
    auto&       subpassInputAttachments        = perSubpassInputAttachments[i];
    auto&       subpassRenderTargetAttachments = perSubpassRenderTargetAttachments[i];
    auto&       subpassResolveAttachments      = perSubpassResolveAttachments[i];

    const xiiUInt32 uiInputAttachmentCount = xiiSubpass.m_InputAttachments.GetCount();

    subpassInputAttachments.SetCount(uiInputAttachmentCount);
    for (xiiUInt32 j = 0; j < uiInputAttachmentCount; ++j)
    {
      const auto& xiiInputAttachment = xiiSubpass.m_InputAttachments[j];
      auto&       inputAttachment    = subpassInputAttachments[j];

      inputAttachment.AttachmentIndex = xiiInputAttachment.m_uiAttachmentIndex;
      inputAttachment.State           = xiiDiligentTypeConversions::GetResourceState(xiiInputAttachment.m_ResourceStateFlags);
    }

    const xiiUInt32 uiRenderTargetAttachmentCount = xiiSubpass.m_RenderTargetAttachments.GetCount();

    subpassRenderTargetAttachments.SetCount(uiRenderTargetAttachmentCount);
    for (xiiUInt32 j = 0; j < uiRenderTargetAttachmentCount; ++j)
    {
      const auto& xiiRenderTargetAttachment = xiiSubpass.m_RenderTargetAttachments[j];
      auto&       renderTargetAttachment    = subpassRenderTargetAttachments[j];

      renderTargetAttachment.AttachmentIndex = xiiRenderTargetAttachment.m_uiAttachmentIndex;
      renderTargetAttachment.State           = xiiDiligentTypeConversions::GetResourceState(xiiRenderTargetAttachment.m_ResourceStateFlags);
    }

    const xiiUInt32 uiResolveAttachmentsCount = xiiSubpass.m_ResolveAttachments.GetCount();

    subpassResolveAttachments.SetCount(uiResolveAttachmentsCount);
    for (xiiUInt32 j = 0; j < uiResolveAttachmentsCount; ++j)
    {
      const auto& xiiResolveAttachment = xiiSubpass.m_ResolveAttachments[j];
      auto&       resolveAttachment    = subpassResolveAttachments[j];

      resolveAttachment.AttachmentIndex = xiiResolveAttachment.m_uiAttachmentIndex;
      resolveAttachment.State           = xiiDiligentTypeConversions::GetResourceState(xiiResolveAttachment.m_ResourceStateFlags);
    }

    const bool bHasDepthAttachment = !xiiSubpass.m_DepthStencilAttachment.IsEmpty();

    if (bHasDepthAttachment)
    {
      const auto& xiiDepthStencilAttachment = xiiSubpass.m_DepthStencilAttachment[0];
      auto&       depthStencilAttachment    = perSubpassDepthStencilAttachments[i];

      depthStencilAttachment.AttachmentIndex = xiiDepthStencilAttachment.m_uiAttachmentIndex;
      depthStencilAttachment.State           = xiiDiligentTypeConversions::GetResourceState(xiiDepthStencilAttachment.m_ResourceStateFlags);
    }

    const xiiUInt32 uiPreserveAttachemntCount = xiiSubpass.m_PreserveAttachments.GetCount();

    const bool bHasShadingRateAttachment = !xiiSubpass.m_ShadingRateAttachment.IsEmpty();

    if (bHasShadingRateAttachment)
    {
      const auto& xiiShadingRateAttachment = xiiSubpass.m_ShadingRateAttachment[0];
      auto&       shadingRateAttachment    = perSubpassShadingRateAttachments[i];

      shadingRateAttachment.Attachment.AttachmentIndex = xiiShadingRateAttachment.m_AttachmentReference.m_uiAttachmentIndex;
      shadingRateAttachment.Attachment.State           = xiiDiligentTypeConversions::GetResourceState(xiiShadingRateAttachment.m_AttachmentReference.m_ResourceStateFlags);
      shadingRateAttachment.TileSize[0]                = xiiShadingRateAttachment.m_TileSize.width;
      shadingRateAttachment.TileSize[1]                = xiiShadingRateAttachment.m_TileSize.height;
    }

    subpass.InputAttachmentCount        = uiInputAttachmentCount;
    subpass.pInputAttachments           = subpassInputAttachments.GetData();
    subpass.RenderTargetAttachmentCount = uiRenderTargetAttachmentCount;
    subpass.pRenderTargetAttachments    = subpassRenderTargetAttachments.GetData();
    subpass.pResolveAttachments         = subpassResolveAttachments.GetData();
    subpass.PreserveAttachmentCount     = uiPreserveAttachemntCount;
    subpass.pPreserveAttachments        = xiiSubpass.m_PreserveAttachments.GetData();

    if (bHasDepthAttachment)
      subpass.pDepthStencilAttachment = &perSubpassDepthStencilAttachments[i];

    if (bHasShadingRateAttachment)
      subpass.pShadingRateAttachment = &perSubpassShadingRateAttachments[i];
  }
  renderPassDescription.SubpassCount = uiSubpassCount;
  renderPassDescription.pSubpasses   = subpasses.GetData();

  // Subpass dependencies.

  const xiiUInt32 uiSubpassDependencyCount = m_Description.m_Dependencies.GetCount();

  xiiHybridArray<Diligent::SubpassDependencyDesc, 16U> dependencies;
  dependencies.SetCount(uiSubpassDependencyCount);

  for (xiiUInt32 i = 0; i < uiSubpassDependencyCount; ++i)
  {
    const auto& xiiDependency = m_Description.m_Dependencies[i];
    auto&       dependency    = dependencies[i];

    dependency.SrcSubpass    = xiiDependency.m_uiSourceSubPass;
    dependency.DstSubpass    = xiiDependency.m_uiDestinationSubPass;
    dependency.SrcStageMask  = xiiDiligentTypeConversions::GetPipelineStageFlags(xiiDependency.m_SourceStageFlags);
    dependency.DstStageMask  = xiiDiligentTypeConversions::GetPipelineStageFlags(xiiDependency.m_DestinationStageFlags);
    dependency.SrcAccessMask = xiiDiligentTypeConversions::GetAccessFlags(xiiDependency.m_SourceAccessFlags);
    dependency.DstAccessMask = xiiDiligentTypeConversions::GetAccessFlags(xiiDependency.m_DestinationAccessFlags);
  }
  renderPassDescription.DependencyCount = uiSubpassDependencyCount;
  renderPassDescription.pDependencies   = dependencies.GetData();

  pDeviceD3D11->GetDevice()->CreateRenderPass(renderPassDescription, &m_pRenderPass);

  return (m_pRenderPass != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALRenderPassD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pRenderPass);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_RenderPassD3D11);
