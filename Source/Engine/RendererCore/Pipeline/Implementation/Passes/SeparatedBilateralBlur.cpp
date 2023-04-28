#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SeparatedBilateralBlur.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BilateralBlurConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSeparatedBilateralBlurPass, 2, xiiRTTIDefaultAllocator<xiiSeparatedBilateralBlurPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BlurSource", m_PinBlurSourceInput),
    XII_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ACCESSOR_PROPERTY("BlurRadius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(7)),
      // Should we really expose that? This gives the user control over the error compared to a perfect gaussian.
      // In theory we could also compute this for a given error from the blur radius. See http://dev.theomader.com/gaussian-kernel-calculator/ for visualization.
    XII_ACCESSOR_PROPERTY("GaussianSigma", GetGaussianSigma, SetGaussianSigma)->AddAttributes(new xiiDefaultValueAttribute(4.0f)),
    XII_ACCESSOR_PROPERTY("Sharpness", GetSharpness, SetSharpness)->AddAttributes(new xiiDefaultValueAttribute(120.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSeparatedBilateralBlurPass::xiiSeparatedBilateralBlurPass() :
  xiiRenderPipelinePass("SeparatedBilateral"), m_uiRadius(7), m_fGaussianSigma(3.5f), m_fSharpness(120.0f)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBilateralBlurCB = xiiRenderContext::CreateConstantBufferStorage<xiiBilateralBlurConstants>(XII_STRINGIZE(xiiBilateralBlurConstants));
  }
}

xiiSeparatedBilateralBlurPass::~xiiSeparatedBilateralBlurPass()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hBilateralBlurCB);
  m_hBilateralBlurCB.Invalidate();
}

bool xiiSeparatedBilateralBlurPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_ASSERT_DEBUG(inputs.GetCount() == 2, "Unexpected number of inputs for xiiSeparatedBilateralBlurPass.");

  // Color
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex])
  {
    xiiLog::Error("No blur target connected to bilateral blur pass!");
    return false;
  }
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    xiiLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }

  // Depth
  if (!inputs[m_PinDepthInput.m_uiInputIndex])
  {
    xiiLog::Error("No depth connected to bilateral blur pass!");
    return false;
  }
  if (!inputs[m_PinDepthInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    xiiLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }
  if (inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_uiWidth != inputs[m_PinDepthInput.m_uiInputIndex]->m_uiWidth || inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_uiHeight != inputs[m_PinDepthInput.m_uiInputIndex]->m_uiHeight)
  {
    xiiLog::Error("Blur target and depth buffer for bilateral blur pass need to have the same dimensions.");
    return false;
  }

  // Output format maches input format.
  outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinBlurSourceInput.m_uiInputIndex];

  return true;
}

void xiiSeparatedBilateralBlurPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    xiiGALDevice* pDevice  = xiiGALDevice::GetDefaultDevice();
    xiiGALPass*   pGALPass = pDevice->BeginPass(GetName());
    XII_SCOPE_EXIT(pDevice->EndPass(pGALPass));

    // Setup input view and sampler
    xiiGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture                               = inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_TextureHandle;
    xiiGALResourceViewHandle hBlurSourceInputView = xiiGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);
    rvcd.m_hTexture                               = inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle;
    xiiGALResourceViewHandle hDepthInputView      = xiiGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Get temp texture for horizontal target / vertical source.
    xiiGALTextureCreationDescription tempTextureDesc = outputs[m_PinBlurSourceInput.m_uiInputIndex]->m_Desc;
    tempTextureDesc.m_bAllowShaderResourceView       = true;
    tempTextureDesc.m_bCreateRenderTarget            = true;
    xiiGALTextureHandle tempTexture                  = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempTextureDesc);
    rvcd.m_hTexture                                  = tempTexture;
    xiiGALResourceViewHandle hTempTextureRView       = xiiGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    xiiGALRenderingSetup renderingSetup;

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", hDepthInputView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiBilateralBlurConstants", m_hBilateralBlurCB);

    // Horizontal
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempTexture));
      auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_HORIZONTAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hBlurSourceInputView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Vertical
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
      auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_VERTICAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hTempTextureRView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Give back temp texture.
    xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempTexture);
  }
}

void xiiSeparatedBilateralBlurPass::SetRadius(xiiUInt32 uiRadius)
{
  m_uiRadius = uiRadius;

  xiiBilateralBlurConstants* cb = xiiRenderContext::GetConstantBufferData<xiiBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->BlurRadius                = m_uiRadius;
}

xiiUInt32 xiiSeparatedBilateralBlurPass::GetRadius() const
{
  return m_uiRadius;
}

void xiiSeparatedBilateralBlurPass::SetGaussianSigma(const float sigma)
{
  m_fGaussianSigma = sigma;

  xiiBilateralBlurConstants* cb = xiiRenderContext::GetConstantBufferData<xiiBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->GaussianFalloff           = 1.0f / (2.0f * m_fGaussianSigma * m_fGaussianSigma);
}

float xiiSeparatedBilateralBlurPass::GetGaussianSigma() const
{
  return m_fGaussianSigma;
}

void xiiSeparatedBilateralBlurPass::SetSharpness(const float fSharpness)
{
  m_fSharpness = fSharpness;

  xiiBilateralBlurConstants* cb = xiiRenderContext::GetConstantBufferData<xiiBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->Sharpness                 = m_fSharpness;
}

float xiiSeparatedBilateralBlurPass::GetSharpness() const
{
  return m_fSharpness;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiSeparatedBilateralBlurPassPatch_1_2 : public xiiGraphPatch
{
public:
  xiiSeparatedBilateralBlurPassPatch_1_2() :
    xiiGraphPatch("xiiSeparatedBilateralBlurPass", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Blur Radius", "BlurRadius");
    pNode->RenameProperty("Gaussian Standard Deviation", "GaussianSigma");
    pNode->RenameProperty("Bilateral Sharpness", "Sharpness");
  }
};

xiiSeparatedBilateralBlurPassPatch_1_2 g_xiiSeparatedBilateralBlurPassPatch_1_2;



XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
