#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

#include <GraphicsFoundation/Resources/Texture.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/SelectionHighlightConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSelectionHighlightPass, 1, xiiRTTIDefaultAllocator<xiiSelectionHighlightPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_PinColor),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),

    XII_MEMBER_PROPERTY("HighlightColor", m_HighlightColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColorScheme::LightUI(xiiColorScheme::Yellow))),
    XII_MEMBER_PROPERTY("OverlayOpacity", m_fOverlayOpacity)->AddAttributes(new xiiDefaultValueAttribute(0.1f))
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSelectionHighlightPass::xiiSelectionHighlightPass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true)
{
  // Load shader.
  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SelectionHighlight.xiiShader");
  XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load selection highlight shader!");

  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiSelectionHighlightConstants>();
}

xiiSelectionHighlightPass::~xiiSelectionHighlightPass()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool xiiSelectionHighlightPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
    return true;
  }

  return false;
}

void xiiSelectionHighlightPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinColor.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  auto pDepthInput = inputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthInput == nullptr)
  {
    return;
  }

  xiiRenderDataBatchList renderDataBatchList = GetPipeline()->GetRenderDataBatchesWithCategory(xiiDefaultRenderDataCategories::Selection);
  if (renderDataBatchList.GetBatchCount() == 0)
  {
    return;
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALTextureHandle hDepthTexture;

  // render all selection objects to depth target only
  {
    xiiUInt32                      uiWidth      = pColorOutput->m_Desc.m_Size.width;
    xiiUInt32                      uiHeight     = pColorOutput->m_Desc.m_Size.height;
    xiiEnum<xiiGALMSAASampleCount> sampleCount  = (xiiGALMSAASampleCount::Enum)pColorOutput->m_Desc.m_uiSampleCount;
    xiiUInt32                      uiSliceCount = pColorOutput->m_Desc.m_uiArraySizeOrDepth;

    hDepthTexture = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, xiiGALTextureFormat::D24UNormalizedS8UInt, sampleCount, uiSliceCount);

    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetTexture(hDepthTexture)->GetDefaultView(xiiGALTextureViewType::DepthStencil));
    renderingSetup.m_bClearDepth   = true;
    renderingSetup.m_bClearStencil = true;

    auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");

    RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::Selection);
  }

  // reconstruct selection overlay from depth target
  {
    auto constants            = xiiRenderContext::GetConstantBufferData<xiiSelectionHighlightConstants>(m_hConstantBuffer);
    constants->HighlightColor = m_HighlightColor;
    constants->OverlayOpacity = m_fOverlayOpacity;

    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(pColorOutput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));

    auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiSelectionHighlightConstants", m_hConstantBuffer);
    renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("SelectionDepthTexture", pDevice->GetTexture(hDepthTexture)->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepthTexture", pDevice->GetTexture(pDepthInput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::ShaderResource));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hDepthTexture);
  }
}

xiiResult xiiSelectionHighlightPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_HighlightColor;
  inout_stream << m_fOverlayOpacity;
  return XII_SUCCESS;
}

xiiResult xiiSelectionHighlightPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_HighlightColor;
  inout_stream >> m_fOverlayOpacity;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SelectionHighlightPass);
