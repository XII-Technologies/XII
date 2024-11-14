#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Passes/SeparatedBilateralBlur.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/BilateralBlurConstants.h>

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
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Utilities")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSeparatedBilateralBlurPass::xiiSeparatedBilateralBlurPass() :
  xiiRenderPipelinePass("SeparatedBilateral")
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBilateralBlurCB = xiiRenderContext::CreateConstantBufferStorage<xiiBilateralBlurConstants>();
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
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
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
  if (!inputs[m_PinDepthInput.m_uiInputIndex]->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
  {
    xiiLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }
  if (inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_Size.width != inputs[m_PinDepthInput.m_uiInputIndex]->m_Size.width || inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_Size.height != inputs[m_PinDepthInput.m_uiInputIndex]->m_Size.height)
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
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

    // Setup input view and sampler
    xiiGALTextureViewCreationDescription rvcd;
    rvcd.m_hTexture                              = inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_TextureHandle;
    xiiGALTextureViewHandle hBlurSourceInputView = xiiGALDevice::GetDefaultDevice()->CreateTextureView(rvcd);
    rvcd.m_hTexture                              = inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle;
    xiiGALTextureViewHandle hDepthInputView      = xiiGALDevice::GetDefaultDevice()->CreateTextureView(rvcd);

    // Get temp texture for horizontal target / vertical source.
    xiiGALTextureCreationDescription tempTextureDesc = outputs[m_PinBlurSourceInput.m_uiInputIndex]->m_TextureDescription;
    tempTextureDesc.m_BindFlags.Add(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget);
    xiiGALTextureHandle tempTexture           = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempTextureDesc);
    rvcd.m_hTexture                           = tempTexture;
    xiiGALTextureViewHandle hTempTextureRView = xiiGALDevice::GetDefaultDevice()->CreateTextureView(rvcd);

    xiiGALRenderingSetup renderingSetup;

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", hDepthInputView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiBilateralBlurConstants", m_hBilateralBlurCB);

    // Horizontal
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(tempTexture)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      auto pCommandList = xiiRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, "", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_HORIZONTAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hBlurSourceInputView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Vertical
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      auto pCommandList = xiiRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, "", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_VERTICAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hTempTextureRView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Give back temp texture.
    xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempTexture);
  }
}

xiiResult xiiSeparatedBilateralBlurPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_uiRadius;
  inout_stream << m_fGaussianSigma;
  inout_stream << m_fSharpness;
  return XII_SUCCESS;
}

xiiResult xiiSeparatedBilateralBlurPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_uiRadius;
  inout_stream >> m_fGaussianSigma;
  inout_stream >> m_fSharpness;
  return XII_SUCCESS;
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

void xiiSeparatedBilateralBlurPass::SetGaussianSigma(const float fSigma)
{
  m_fGaussianSigma = fSigma;

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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
