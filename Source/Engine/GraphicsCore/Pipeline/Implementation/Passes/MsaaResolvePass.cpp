#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/MsaaResolvePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>


#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsaaResolvePass, 1, xiiRTTIDefaultAllocator<xiiMsaaResolvePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMsaaResolvePass::xiiMsaaResolvePass() :
  xiiRenderPipelinePass("MsaaResolvePass", true)

{
  {
    // Load shader.
    m_hDepthResolveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/MsaaDepthResolve.xiiShader");
    XII_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Could not load depth resolve shader!");
  }
}

xiiMsaaResolvePass::~xiiMsaaResolvePass() = default;

bool xiiMsaaResolvePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount == xiiGALSampleCount::OneSample)
    {
      xiiLog::Error("Input is not a valid msaa target");
      return false;
    }

    m_bIsDepth        = xiiGALTextureFormat::IsDepthFormat(pInput->m_Format);
    m_MsaaSampleCount = pInput->m_SampleCount;

    xiiGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount                    = xiiGALSampleCount::OneSample;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiMsaaResolvePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pInput  = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (m_bIsDepth)
  {
    // Setup render target
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

    // Bind render target and viewport
    auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    auto& globals          = renderViewContext.m_pRenderContext->WriteGlobalConstants();
    globals.NumMsaaSamples = m_MsaaSampleCount;

    renderViewContext.m_pRenderContext->BindShader(m_hDepthResolveShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
  else
  {
    auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, xiiGALRenderingSetup(), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    xiiGALTextureSubresource subresource;
    subresource.m_uiMipLevel   = 0;
    subresource.m_uiArraySlice = 0;

    pCommandEncoder->ResolveTexture(pOutput->m_TextureHandle, subresource, pInput->m_TextureHandle, subresource);

    if (renderViewContext.m_pCamera->IsStereoscopic())
    {
      subresource.m_uiArraySlice = 1;
      pCommandEncoder->ResolveTexture(pOutput->m_TextureHandle, subresource, pInput->m_TextureHandle, subresource);
    }
  }
}



XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_MsaaResolvePass);
