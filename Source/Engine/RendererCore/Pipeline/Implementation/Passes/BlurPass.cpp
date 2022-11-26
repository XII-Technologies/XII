#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/BlurPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlurConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBlurPass, 1, xiiRTTIDefaultAllocator<xiiBlurPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(15)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBlurPass::xiiBlurPass() :
  xiiRenderPipelinePass("BlurPass"), m_iRadius(15)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Blur.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBlurCB = xiiRenderContext::CreateConstantBufferStorage<xiiBlurConstants>();
  }
}

xiiBlurPass::~xiiBlurPass()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hBlurCB);
  m_hBlurCB.Invalidate();
}

bool xiiBlurPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      xiiLog::Error("Blur pass input must allow shader resoure view.");
      return false;
    }

    outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinInput.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No input connected to blur pass!");
    return false;
  }

  return true;
}

void xiiBlurPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

    // Setup render target
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
    renderingSetup.m_uiRenderTargetClearMask = xiiInvalidIndex;
    renderingSetup.m_ClearColor              = xiiColor(1.0f, 0.0f, 0.0f);

    // Bind render target and viewport
    auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    // Setup input view and sampler
    xiiGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture                        = inputs[m_PinInput.m_uiInputIndex]->m_TextureHandle;
    xiiGALResourceViewHandle hResourceView = xiiGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("Input", hResourceView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiBlurConstants", m_hBlurCB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

void xiiBlurPass::SetRadius(xiiInt32 iRadius)
{
  m_iRadius = iRadius;

  xiiBlurConstants* cb = xiiRenderContext::GetConstantBufferData<xiiBlurConstants>(m_hBlurCB);
  cb->BlurRadius       = m_iRadius;
}

xiiInt32 xiiBlurPass::GetRadius() const
{
  return m_iRadius;
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BlurPass);
