#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Passes/BloomPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Profiling/Profiling.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/BloomConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBloomPass, 1, xiiRTTIDefaultAllocator<xiiBloomPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.01f, 1.0f)),
    XII_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(0.3f)),
    XII_MEMBER_PROPERTY("InnerTintColor", m_InnerTintColor),
    XII_MEMBER_PROPERTY("MidTintColor", m_MidTintColor),
    XII_MEMBER_PROPERTY("OuterTintColor", m_OuterTintColor),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBloomPass::xiiBloomPass() :
  xiiRenderPipelinePass("BloomPass", true)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Bloom.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load bloom shader!");
  }

  {
    m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiBloomConstants>();
  }
}

xiiBloomPass::~xiiBloomPass()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool xiiBloomPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      xiiLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    // Output is half-res
    xiiGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_uiWidth                        = desc.m_uiWidth / 2;
    desc.m_uiHeight                       = desc.m_uiHeight / 2;
    desc.m_Format                         = xiiGALResourceFormat::RG11B10Float;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiBloomPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput  = inputs[m_PinInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  xiiGALDevice* pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiGALPass*   pGALPass = pDevice->BeginPass(GetName());
  XII_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  xiiUInt32 uiWidth        = pColorInput->m_Desc.m_uiWidth;
  xiiUInt32 uiHeight       = pColorInput->m_Desc.m_uiHeight;
  bool      bFastDownscale = xiiMath::IsEven(uiWidth) && xiiMath::IsEven(uiHeight);

  const float     fMaxRes         = (float)xiiMath::Max(uiWidth, uiHeight);
  const float     fRadius         = xiiMath::Clamp(m_fRadius, 0.01f, 1.0f);
  const float     fDownscaledSize = 4.0f / fRadius;
  const float     fNumBlurPasses  = xiiMath::Log2(fMaxRes / fDownscaledSize);
  const xiiUInt32 uiNumBlurPasses = (xiiUInt32)xiiMath::Ceil(fNumBlurPasses);

  // Find temp targets
  xiiHybridArray<xiiVec2, 8>             targetSizes;
  xiiHybridArray<xiiGALTextureHandle, 8> tempDownscaleTextures;
  xiiHybridArray<xiiGALTextureHandle, 8> tempUpscaleTextures;

  for (xiiUInt32 i = 0; i < uiNumBlurPasses; ++i)
  {
    uiWidth  = xiiMath::Max(uiWidth / 2, 1u);
    uiHeight = xiiMath::Max(uiHeight / 2, 1u);
    targetSizes.PushBack(xiiVec2((float)uiWidth, (float)uiHeight));
    auto uiSliceCount = pColorOutput->m_Desc.m_uiArraySize;

    tempDownscaleTextures.PushBack(xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, xiiGALResourceFormat::RG11B10Float, xiiGALMSAASampleCount::None, uiSliceCount));

    // biggest upscale target is the output and lowest is not needed
    if (i > 0 && i < uiNumBlurPasses - 1)
    {
      tempUpscaleTextures.PushBack(xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, xiiGALResourceFormat::RG11B10Float, xiiGALMSAASampleCount::None, uiSliceCount));
    }
    else
    {
      tempUpscaleTextures.PushBack(xiiGALTextureHandle());
    }
  }

  renderViewContext.m_pRenderContext->BindConstantBuffer("xiiBloomConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);

  // Downscale passes
  {
    xiiTempHashedString sInitialDownscale     = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE";
    xiiTempHashedString sInitialDownscaleFast = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE_FAST";
    xiiTempHashedString sDownscale            = "BLOOM_PASS_MODE_DOWNSCALE";
    xiiTempHashedString sDownscaleFast        = "BLOOM_PASS_MODE_DOWNSCALE_FAST";

    for (xiiUInt32 i = 0; i < uiNumBlurPasses; ++i)
    {
      xiiGALTextureHandle hInput;
      if (i == 0)
      {
        hInput = pColorInput->m_TextureHandle;
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", bFastDownscale ? sInitialDownscaleFast : sInitialDownscale);
      }
      else
      {
        hInput = tempDownscaleTextures[i - 1];
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", bFastDownscale ? sDownscaleFast : sDownscale);
      }

      xiiGALTextureHandle hOutput    = tempDownscaleTextures[i];
      xiiVec2             targetSize = targetSizes[i];

      xiiGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, xiiRectFloat(targetSize.x, targetSize.y), "Downscale", renderViewContext.m_pCamera->IsStereoscopic());

      xiiColor tintColor = (i == uiNumBlurPasses - 1) ? xiiColor(m_OuterTintColor) : xiiColor::White;
      UpdateConstantBuffer(xiiVec2(1.0f).CompDiv(targetSize), tintColor);

      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();

      bFastDownscale = xiiMath::IsEven((xiiInt32)targetSize.x) && xiiMath::IsEven((xiiInt32)targetSize.y);
    }
  }

  // Upscale passes
  {
    const float fBlurRadius = 2.0f * fNumBlurPasses / uiNumBlurPasses;
    const float fMidPass    = (uiNumBlurPasses - 1.0f) / 2.0f;

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "BLOOM_PASS_MODE_UPSCALE");

    for (xiiUInt32 i = uiNumBlurPasses - 1; i-- > 0;)
    {
      xiiGALTextureHandle hNextInput = tempDownscaleTextures[i];
      xiiGALTextureHandle hInput;
      if (i == uiNumBlurPasses - 2)
      {
        hInput = tempDownscaleTextures[i + 1];
      }
      else
      {
        hInput = tempUpscaleTextures[i + 1];
      }

      xiiGALTextureHandle hOutput;
      if (i == 0)
      {
        hOutput = pColorOutput->m_TextureHandle;
      }
      else
      {
        hOutput = tempUpscaleTextures[i];
      }

      xiiVec2 targetSize = targetSizes[i];

      xiiGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, xiiRectFloat(targetSize.x, targetSize.y), "Upscale", renderViewContext.m_pCamera->IsStereoscopic());

      xiiColor tintColor;
      float    fPass = (float)i;
      if (fPass < fMidPass)
      {
        tintColor = xiiMath::Lerp<xiiColor>(m_InnerTintColor, m_MidTintColor, fPass / fMidPass);
      }
      else
      {
        tintColor = xiiMath::Lerp<xiiColor>(m_MidTintColor, m_OuterTintColor, (fPass - fMidPass) / fMidPass);
      }

      UpdateConstantBuffer(xiiVec2(fBlurRadius).CompDiv(targetSize), tintColor);

      renderViewContext.m_pRenderContext->BindTexture2D("NextColorTexture", pDevice->GetDefaultResourceView(hNextInput));
      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Return temp targets
  for (auto hTexture : tempDownscaleTextures)
  {
    if (!hTexture.IsInvalidated())
    {
      xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTexture);
    }
  }

  for (auto hTexture : tempUpscaleTextures)
  {
    if (!hTexture.IsInvalidated())
    {
      xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTexture);
    }
  }
}

void xiiBloomPass::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor              = xiiColor::Black;

  auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, "Clear");
}

xiiResult xiiBloomPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fRadius;
  inout_stream << m_fThreshold;
  inout_stream << m_fIntensity;
  inout_stream << m_InnerTintColor;
  inout_stream << m_MidTintColor;
  inout_stream << m_OuterTintColor;
  return XII_SUCCESS;
}

xiiResult xiiBloomPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fRadius;
  inout_stream >> m_fThreshold;
  inout_stream >> m_fIntensity;
  inout_stream >> m_InnerTintColor;
  inout_stream >> m_MidTintColor;
  inout_stream >> m_OuterTintColor;
  return XII_SUCCESS;
}

void xiiBloomPass::UpdateConstantBuffer(xiiVec2 pixelSize, const xiiColor& tintColor)
{
  xiiBloomConstants* constants = xiiRenderContext::GetConstantBufferData<xiiBloomConstants>(m_hConstantBuffer);
  constants->PixelSize         = pixelSize;
  constants->BloomThreshold    = m_fThreshold;
  constants->BloomIntensity    = m_fIntensity;

  constants->TintColor = tintColor;
}



XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_BloomPass);
