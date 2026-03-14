#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/SelectionHighlightConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSelectionHighlightPass, 1, xiiRTTIDefaultAllocator<xiiSelectionHighlightPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Colour", m_PinColour),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_MEMBER_PROPERTY("HighlightColour", m_HighlightColour)->AddAttributes(new xiiDefaultValueAttribute(xiiColorScheme::LightUI(xiiColorScheme::Yellow))),
    XII_MEMBER_PROPERTY("OverlayOpacity", m_fOverlayOpacity)->AddAttributes(new xiiDefaultValueAttribute(0.1f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSelectionHighlightPass::xiiSelectionHighlightPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::None)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SelectionHighlight.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Failed to load selection highlight shader!");
  }

  m_pConstantBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(xiiGALDevice::GetDefaultDevice(), sizeof(xiiSelectionHighlightConstants), "xiiSelectionHighlightConstants");
}

xiiSelectionHighlightPass::~xiiSelectionHighlightPass() = default;

xiiResult xiiSelectionHighlightPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_HighlightColour;
  inout_stream << m_fOverlayOpacity;

  return XII_SUCCESS;
}

xiiResult xiiSelectionHighlightPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_HighlightColour;
  inout_stream >> m_fOverlayOpacity;

  return XII_SUCCESS;
}

xiiResult xiiSelectionHighlightPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  // Colour attachment.
  if (pInputs[m_PinColour.m_uiInputIndex])
  {
    pOutputs[m_PinColour.m_uiOutputIndex] = *pInputs[m_PinColour.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No input colour attachment connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiSelectionHighlightPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  auto pOutputColourAttachment = pOutputs[m_PinColour.m_uiOutputIndex];
  if (pOutputColourAttachment == nullptr)
    return;

  auto pInputDepthAttachment = pInputs[m_PinDepthStencil.m_uiInputIndex];
  if (pInputDepthAttachment == nullptr)
    return;

  xiiRenderDataBatchList renderDataBatchList = GetPipeline()->GetRenderDataBatchesWithCategory(xiiDefaultRenderDataCategories::Selection);
  if (renderDataBatchList.GetBatchCount() == 0)
    return;

  xiiSharedPtr<xiiGALDevice>  pDevice = xiiGALDevice::GetDefaultDevice();
  xiiSharedPtr<xiiGALTexture> pDepthTexture;

  // Render all selection objects to depth target only.
  {
    xiiGALTextureCreationDescription depthTextureDescription;
    depthTextureDescription.m_Type               = xiiGALResourceDimension::Texture2DArray;
    depthTextureDescription.m_Format             = pInputDepthAttachment->m_Resource.m_Texture.m_Description.m_Format;
    depthTextureDescription.m_Size               = pInputDepthAttachment->m_Resource.m_Texture.m_Description.m_Size;
    depthTextureDescription.m_uiArraySizeOrDepth = pInputDepthAttachment->m_Resource.m_Texture.m_Description.m_uiArraySizeOrDepth;
    depthTextureDescription.m_uiMipLevels        = 1;
    depthTextureDescription.m_uiSampleCount      = pInputDepthAttachment->m_Resource.m_Texture.m_Description.m_uiSampleCount;
    depthTextureDescription.m_BindFlags          = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    depthTextureDescription.m_Usage              = xiiGALResourceUsage::Default;

    pDepthTexture = xiiGPUResourcePool::GetDefaultInstance()->GetTexture(depthTextureDescription);

    xiiRenderingSetup renderingSetup;
    renderingSetup.SetDepthStencilAttachment({pDepthTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil), 1.0, 0U, xiiGALAttachmentLoadOperation::Clear, xiiGALAttachmentStoreOperation::Store, xiiGALAttachmentLoadOperation::Clear, xiiGALAttachmentStoreOperation::Store}).Build();

    auto pRenderContext = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
    RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::Selection);
  }

  // Now reconstruct the highlight overlay from the depth target.
  {
    xiiRenderingSetup renderingSetup;
    renderingSetup.AddColorAttachment({pOutputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget)}).Build();

    auto pRenderContext = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    {
      xiiGALMapHelper<xiiSelectionHighlightConstants> pSelectionHighlightConstants(pRenderContext->GetCommandList(), m_pConstantBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pSelectionHighlightConstants->HighlightColor = m_HighlightColour;
      pSelectionHighlightConstants->OverlayOpacity = m_fOverlayOpacity;
    }

    pRenderContext->BindShader(m_hShader);
    pRenderContext->BindConstantBuffer("xiiSelectionHighlightConstants", m_pConstantBuffer);
    pRenderContext->BindTexture("SelectionDepthTexture", pDepthTexture);
    pRenderContext->BindTexture("SceneDepthTexture", pInputDepthAttachment->m_Resource.m_Texture.m_pTexture);
    pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1U);
    pRenderContext->DrawMeshBuffer().IgnoreResult();

    xiiGPUResourcePool::GetDefaultInstance()->ReturnTexture(std::move(pDepthTexture));
  }
}
