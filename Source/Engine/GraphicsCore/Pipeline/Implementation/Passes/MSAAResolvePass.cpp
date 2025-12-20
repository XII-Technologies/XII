#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/MSAAResolvePass.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMSAAResolvePass, 1, xiiRTTIDefaultAllocator<xiiMSAAResolvePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_MEMBER_PROPERTY("FrameConstants", m_PinFrameConstants),
    XII_ENUM_MEMBER_PROPERTY("SampleCount", xiiGALMSAASampleCount, m_SampleCount),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMSAAResolvePass::xiiMSAAResolvePass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::None)
{
  {
    // Load shader.
    m_hDepthResolveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/MsaaDepthResolve.xiiShader");
    XII_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Failed to load MSAA depth resolve shader!");
  }
}

xiiMSAAResolvePass::~xiiMSAAResolvePass() = default;

xiiResult xiiMSAAResolvePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_SampleCount;

  return XII_SUCCESS;
}

xiiResult xiiMSAAResolvePass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_SampleCount;

  return XII_SUCCESS;
}

xiiResult xiiMSAAResolvePass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  // Colour attachment.
  if (pInputs[m_PinInput.m_uiInputIndex])
  {
    if (pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_uiSampleCount == static_cast<xiiUInt32>(xiiGALMSAASampleCount::OneSample))
    {
      xiiLog::Error("Input texture must be a MSAA source in pass '{0}'!", GetName());
      return XII_FAILURE;
    }

    m_bIsDepthResolve = xiiGALResourceFormat::IsDepthFormat(pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_Format);
    m_SampleCount     = static_cast<xiiGALMSAASampleCount::Enum>(pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_uiSampleCount);

    xiiRenderPipelinePassResource request           = *pInputs[m_PinInput.m_uiInputIndex];
    request.m_Texture.m_Description.m_uiSampleCount = xiiGALMSAASampleCount::OneSample;
    pOutputs[m_PinOutput.m_uiOutputIndex]           = request;
  }
  else
  {
    xiiLog::Error("No input colour attachment connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiMSAAResolvePass::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(view);
  XII_IGNORE_UNUSED(pOutputs);

  m_pRenderPass.Clear();
  m_FramebufferCache.Clear();

  auto pColourOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pColourOutput == nullptr)
    return XII_FAILURE;

  if (m_bIsDepthResolve)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

    xiiGALRenderPassCreationDescription renderPassDescription;

    xiiGALRenderPassAttachmentDescription& colourAttachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
    colourAttachment.m_Format                               = pColourOutput->m_Resource.m_Texture.m_Description.m_Format;
    colourAttachment.m_uiSampleCount                        = pColourOutput->m_Resource.m_Texture.m_Description.m_uiSampleCount;
    colourAttachment.m_LoadOperation                        = xiiGALAttachmentLoadOperation::Load;
    colourAttachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
    colourAttachment.m_StencilLoadOperation                 = xiiGALAttachmentLoadOperation::Load;
    colourAttachment.m_StencilStoreOperation                = xiiGALAttachmentStoreOperation::Store;
    colourAttachment.m_InitialStateFlags                    = xiiGALResourceStateFlags::DepthWrite;
    colourAttachment.m_FinalStateFlags                      = xiiGALResourceStateFlags::DepthWrite;

    xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses.ExpandAndGetRef();
    subpass.m_DepthStencilAttachment.PushBack(xiiGALAttachmentReferenceDescription{.m_uiAttachmentIndex = 0U, .m_ResourceStateFlags = xiiGALResourceStateFlags::DepthWrite});

    xiiGALSubPassDependencyDescription& dependency = renderPassDescription.m_Dependencies.ExpandAndGetRef();
    dependency.m_uiSourceSubPass                   = XII_GAL_SUBPASS_EXTERNAL;
    dependency.m_uiDestinationSubPass              = 0U;
    dependency.m_SourceStageFlags                  = xiiGALPipelineStageFlags::EarlyFragmentTests | xiiGALPipelineStageFlags::RenderTarget;
    dependency.m_DestinationStageFlags             = xiiGALPipelineStageFlags::EarlyFragmentTests | xiiGALPipelineStageFlags::RenderTarget;
    dependency.m_SourceAccessFlags                 = xiiGALAccessFlags::DepthStencilWrite;
    dependency.m_DestinationAccessFlags            = xiiGALAccessFlags::DepthStencilWrite;

    m_pRenderPass = pDevice->CreateRenderPass(renderPassDescription);
  }

  return XII_SUCCESS;
}

void xiiMSAAResolvePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pOutputs);

  auto pInputColourAttachment = pInputs[m_PinInput.m_uiInputIndex];
  if (pInputColourAttachment == nullptr)
    return;

  auto pOutputColourAttachment = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutputColourAttachment == nullptr)
    return;

  auto pFrameConstants = pInputs[m_PinFrameConstants.m_uiInputIndex];
  if (pFrameConstants == nullptr)
    return;

  if (m_bIsDepthResolve)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

    xiiSharedPtr<xiiGALTextureView>             pColourAttachmentView = pOutputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
    const xiiGALTextureCreationDescription&     textureDescription    = pOutputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDescription();
    const xiiGALTextureViewCreationDescription& viewDescription       = pColourAttachmentView->GetDescription();

    // Create or retrieve a corresponding framebuffer from the cache if present.
    xiiSharedPtr<xiiGALFramebuffer> pFramebuffer;
    {
      xiiGALFramebufferCreationDescription frameBufferDescription;
      frameBufferDescription.m_pRenderPass       = m_pRenderPass;
      frameBufferDescription.m_FramebufferSize   = textureDescription.m_Size;
      frameBufferDescription.m_uiArraySliceCount = viewDescription.m_uiArrayOrDepthSlicesCount;
      frameBufferDescription.m_Attachments.PushBack(pColourAttachmentView);

      for (xiiSharedPtr<xiiGALFramebuffer>& pCachedFramebuffer : m_FramebufferCache)
      {
        if (pCachedFramebuffer->GetDescription() == frameBufferDescription)
        {
          pFramebuffer = pCachedFramebuffer;
          break;
        }
      }

      if (!pFramebuffer)
      {
        pFramebuffer = pDevice->CreateFramebuffer(frameBufferDescription);

        m_FramebufferCache.PushBack(pFramebuffer);
      }
    }
    renderViewContext.m_CommandListData.m_pRenderPass      = m_pRenderPass;
    renderViewContext.m_CommandListData.m_pFramebuffer     = pFramebuffer;
    renderViewContext.m_CommandListData.m_pGlobalConstants = pFrameConstants->m_Resource.m_Buffer.m_pBuffer;

    renderViewContext.SetShaderPermutationVariable("MSAA", "TRUE");

    xiiSharedPtr<xiiGALGraphicsPipelineState> pPipelineState = CreatePipelineState(renderViewContext);
    if (!pPipelineState)
      return;

    renderViewContext.m_pCommandList->Begin();
    {
      renderViewContext.m_pCommandList->SetPipelineState(pPipelineState);
      renderViewContext.m_pCommandList->SetViewport({renderViewContext.m_pViewData->m_ViewPortRect});
      renderViewContext.m_pCommandList->ResolveAndSetConstantBuffer(XII_PP_STRINGIFY(xiiGlobalConstants), renderViewContext.m_CommandListData.m_pGlobalConstants);
      renderViewContext.m_pCommandList->ResolveAndSetShaderResourceTextureView("depthTexture", pInputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));

      const xiiUInt32 uiVertsPerPrimitive = xiiGALPrimitiveTopology::VerticesPerPrimitive(pPipelineState->GetDescription().m_GraphicsPipeline.m_PrimitiveTopology);
      xiiUInt32       uiPrimitiveCount    = uiVertsPerPrimitive;
      xiiUInt32       uiInstanceCount     = renderViewContext.m_pCamera->IsStereoscopic() ? 2U : 1U;

      renderViewContext.m_pCommandList->CommitShaderResources().IgnoreResult();
      renderViewContext.m_pCommandList->BeginRenderPass({renderViewContext.m_CommandListData.m_pRenderPass, renderViewContext.m_CommandListData.m_pFramebuffer});
      renderViewContext.m_pCommandList->Draw({uiPrimitiveCount, uiInstanceCount});
      renderViewContext.m_pCommandList->EndRenderPass();
    }
    renderViewContext.m_pCommandList->End();
  }
  else
  {
    xiiGALResolveTextureSubresourceDescription resolveDescription;
    resolveDescription.m_uiSourceMipLevel                 = 0;
    resolveDescription.m_uiSourceSlice                    = 0;
    resolveDescription.m_SourceTextureTransitionMode      = xiiGALStateTransitionMode::Transition;
    resolveDescription.m_uiDestinationMipLevel            = 0;
    resolveDescription.m_uiDestinationSlice               = 0;
    resolveDescription.m_DestinationTextureTransitionMode = xiiGALStateTransitionMode::Transition;

    renderViewContext.m_pCommandList->Begin();
    {
      renderViewContext.m_pCommandList->ResolveTextureSubResource(pInputColourAttachment->m_Resource.m_Texture.m_pTexture, pOutputColourAttachment->m_Resource.m_Texture.m_pTexture, resolveDescription);

      if (renderViewContext.m_pCamera->IsStereoscopic())
      {
        resolveDescription.m_uiSourceSlice      = 1;
        resolveDescription.m_uiDestinationSlice = 1;

        renderViewContext.m_pCommandList->ResolveTextureSubResource(pInputColourAttachment->m_Resource.m_Texture.m_pTexture, pOutputColourAttachment->m_Resource.m_Texture.m_pTexture, resolveDescription);
      }
    }
    renderViewContext.m_pCommandList->End();
  }
}

xiiSharedPtr<xiiGALGraphicsPipelineState> xiiMSAAResolvePass::CreatePipelineState(const xiiRenderViewContext& renderViewContext) const
{
  xiiShaderPermutationResourceHandle hShaderPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hDepthResolveShader, renderViewContext.GetPermutationVariables(), false);

  xiiGALGraphicsPipelineStateCreationDescription graphicsPipelineStateDescription;
  graphicsPipelineStateDescription.m_GraphicsPipeline.m_pRenderPass       = renderViewContext.m_CommandListData.m_pRenderPass;
  graphicsPipelineStateDescription.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
  graphicsPipelineStateDescription.m_GraphicsPipeline.m_uiSampleMask      = 0xFFFFFFFFU;

  {
    xiiResourceLock<xiiShaderPermutationResource> pShaderPermutation(hShaderPermutation, xiiResourceAcquireMode::AllowLoadingFallback);

    if (!pShaderPermutation->IsShaderValid())
      return nullptr;

    graphicsPipelineStateDescription.m_pPipelineResourceSignature = pShaderPermutation->GetPipelineResourceSignature();
    graphicsPipelineStateDescription.m_pVertexShader              = pShaderPermutation->GetGALShader(xiiGALShaderType::Vertex);
    graphicsPipelineStateDescription.m_pGeometryShader            = pShaderPermutation->GetGALShader(xiiGALShaderType::Geometry);
    graphicsPipelineStateDescription.m_pPixelShader               = pShaderPermutation->GetGALShader(xiiGALShaderType::Pixel);

    graphicsPipelineStateDescription.m_GraphicsPipeline.m_pBlendState        = pShaderPermutation->GetBlendState();
    graphicsPipelineStateDescription.m_GraphicsPipeline.m_pRasterizerState   = pShaderPermutation->GetRasterizerState();
    graphicsPipelineStateDescription.m_GraphicsPipeline.m_pDepthStencilState = pShaderPermutation->GetDepthStencilState();
  }

  return xiiGALPipelineCache::GetPipeline(graphicsPipelineStateDescription);
}
