#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/AOPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/DownscaleDepthConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SSAOConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAOPass, 1, xiiRTTIDefaultAllocator<xiiAOPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.01f, 10.0f)),
    XII_MEMBER_PROPERTY("MaxScreenSpaceRadius", m_fMaxScreenSpaceRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.01f, 2.0f)),
    XII_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new xiiDefaultValueAttribute(2.0f)),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(0.7f)),
    XII_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new xiiDefaultValueAttribute(80.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("FadeOutEnd", GetFadeOutEnd, SetFadeOutEnd)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("PositionBias", m_fPositionBias)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.0f, 1000.0f)),
    XII_MEMBER_PROPERTY("MipLevelScale", m_fMipLevelScale)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("DepthBlurThreshold", m_fDepthBlurThreshold)->AddAttributes(new xiiDefaultValueAttribute(2.0f), new xiiClampValueAttribute(0.01f, xiiVariant())),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAOPass::xiiAOPass() :
  xiiRenderPipelinePass("AOPass", true), m_fRadius(1.0f), m_fMaxScreenSpaceRadius(1.0f), m_fContrast(2.0f), m_fIntensity(0.7f), m_fFadeOutStart(80.0f), m_fFadeOutEnd(100.0f), m_fPositionBias(5.0f), m_fMipLevelScale(10.0f), m_fDepthBlurThreshold(2.0f)
{
  m_hNoiseTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>("Textures/SSAONoise.dds");

  m_hDownscaleShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/DownscaleDepth.xiiShader");
  XII_ASSERT_DEV(m_hDownscaleShader.IsValid(), "Could not load downsample shader!");

  m_hSSAOShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SSAO.xiiShader");
  XII_ASSERT_DEV(m_hSSAOShader.IsValid(), "Could not load SSAO shader!");

  m_hBlurShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SSAOBlur.xiiShader");
  XII_ASSERT_DEV(m_hBlurShader.IsValid(), "Could not load SSAO shader!");

  m_hDownscaleConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiDownscaleDepthConstants>(XII_STRINGIZE(xiiDownscaleDepthConstants));
  m_hSSAOConstantBuffer      = xiiRenderContext::CreateConstantBufferStorage<xiiSSAOConstants>(XII_STRINGIZE(xiiSSAOConstants));
}

xiiAOPass::~xiiAOPass()
{
  if (!m_hSSAOSamplerState.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
    m_hSSAOSamplerState.Invalidate();
  }

  xiiRenderContext::DeleteConstantBufferStorage(m_hDownscaleConstantBuffer);
  m_hDownscaleConstantBuffer.Invalidate();

  xiiRenderContext::DeleteConstantBufferStorage(m_hSSAOConstantBuffer);
  m_hSSAOConstantBuffer.Invalidate();
}

bool xiiAOPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  if (auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex])
  {
    if (!pDepthInput->m_bAllowShaderResourceView)
    {
      xiiLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    if (pDepthInput->m_SampleCount != xiiGALMSAASampleCount::None)
    {
      xiiLog::Error("'{0}' input must be resolved", GetName());
      return false;
    }

    xiiGALTextureCreationDescription desc = *pDepthInput;
    desc.m_Format                         = xiiGALResourceFormat::RGHalf;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiAOPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex];
  auto pOutput     = outputs[m_PinOutput.m_uiOutputIndex];
  if (pDepthInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  xiiGALDevice* pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiGALPass*   pGALPass = pDevice->BeginPass(GetName());
  XII_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  xiiUInt32 uiWidth  = pDepthInput->m_Desc.m_uiWidth;
  xiiUInt32 uiHeight = pDepthInput->m_Desc.m_uiHeight;

  xiiUInt32 uiNumMips   = 3;
  xiiUInt32 uiHzbWidth  = xiiMath::RoundUp(uiWidth, 1u << uiNumMips);
  xiiUInt32 uiHzbHeight = xiiMath::RoundUp(uiHeight, 1u << uiNumMips);

  float fHzbScaleX = (float)uiWidth / uiHzbWidth;
  float fHzbScaleY = (float)uiHeight / uiHzbHeight;

  // Find temp targets
  xiiGALTextureHandle                             hzbTexture;
  xiiHybridArray<xiiVec2, 8>                      hzbSizes;
  xiiHybridArray<xiiGALResourceViewHandle, 8>     hzbResourceViews;
  xiiHybridArray<xiiGALRenderTargetViewHandle, 8> hzbRenderTargetViews;

  xiiGALTextureHandle tempSSAOTexture;

  {
    {
      xiiGALTextureCreationDescription desc;
      desc.m_uiWidth                  = uiHzbWidth / 2;
      desc.m_uiHeight                 = uiHzbHeight / 2;
      desc.m_uiMipLevelCount          = 3;
      desc.m_Type                     = xiiGALTextureType::Texture2DArray;
      desc.m_Format                   = xiiGALResourceFormat::RHalf;
      desc.m_bCreateRenderTarget      = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_uiArraySize              = pOutput->m_Desc.m_uiArraySize;

      hzbTexture = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
    }

    for (xiiUInt32 i = 0; i < uiNumMips; ++i)
    {
      uiHzbWidth  = uiHzbWidth / 2;
      uiHzbHeight = uiHzbHeight / 2;

      hzbSizes.PushBack(xiiVec2((float)uiHzbWidth, (float)uiHzbHeight));

      {
        xiiGALResourceViewCreationDescription desc;
        desc.m_hTexture               = hzbTexture;
        desc.m_uiMostDetailedMipLevel = i;
        desc.m_uiMipLevelsToUse       = 1;
        desc.m_uiArraySize            = pOutput->m_Desc.m_uiArraySize;

        hzbResourceViews.PushBack(pDevice->CreateResourceView(desc));
      }

      {
        xiiGALRenderTargetViewCreationDescription desc;
        desc.m_hTexture     = hzbTexture;
        desc.m_uiMipLevel   = i;
        desc.m_uiSliceCount = pOutput->m_Desc.m_uiArraySize;

        hzbRenderTargetViews.PushBack(pDevice->CreateRenderTargetView(desc));
      }
    }

    tempSSAOTexture = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, xiiGALResourceFormat::RGHalf, xiiGALMSAASampleCount::None, pOutput->m_Desc.m_uiArraySize);
  }

  // Mip map passes
  {
    CreateSamplerState();

    for (xiiUInt32 i = 0; i < uiNumMips; ++i)
    {
      xiiGALResourceViewHandle hInputView;
      xiiVec2                  pixelSize;

      if (i == 0)
      {
        hInputView = pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle);
        pixelSize  = xiiVec2(1.0f / uiWidth, 1.0f / uiHeight);
      }
      else
      {
        hInputView = hzbResourceViews[i - 1];
        pixelSize  = xiiVec2(1.0f).CompDiv(hzbSizes[i - 1]);
      }

      xiiGALRenderTargetViewHandle hOutputView = hzbRenderTargetViews[i];
      xiiVec2                      targetSize  = hzbSizes[i];

      xiiGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hOutputView);
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, xiiRectFloat(targetSize.x, targetSize.y), "SSAOMipMaps", renderViewContext.m_pCamera->IsStereoscopic());

      xiiDownscaleDepthConstants* constants = xiiRenderContext::GetConstantBufferData<xiiDownscaleDepthConstants>(m_hDownscaleConstantBuffer);
      constants->PixelSize                  = pixelSize;
      constants->LinearizeDepth             = (i == 0);

      renderViewContext.m_pRenderContext->BindConstantBuffer("xiiDownscaleDepthConstants", m_hDownscaleConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hDownscaleShader);

      renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", hInputView);
      renderViewContext.m_pRenderContext->BindSamplerState("DepthSampler", m_hSSAOSamplerState);

      renderViewContext.m_pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::Triangles, 1);

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Update constants
  {
    float fadeOutScale  = -1.0f / xiiMath::Max(0.001f, (m_fFadeOutEnd - m_fFadeOutStart));
    float fadeOutOffset = -fadeOutScale * m_fFadeOutStart + 1.0f;

    xiiSSAOConstants* constants     = xiiRenderContext::GetConstantBufferData<xiiSSAOConstants>(m_hSSAOConstantBuffer);
    constants->TexCoordsScale       = xiiVec2(fHzbScaleX, fHzbScaleY);
    constants->FadeOutParams        = xiiVec2(fadeOutScale, fadeOutOffset);
    constants->WorldRadius          = m_fRadius;
    constants->MaxScreenSpaceRadius = m_fMaxScreenSpaceRadius;
    constants->Contrast             = m_fContrast;
    constants->Intensity            = m_fIntensity;
    constants->PositionBias         = m_fPositionBias / 1000.0f;
    constants->MipLevelScale        = m_fMipLevelScale;
    constants->DepthBlurScale       = 1.0f / m_fDepthBlurThreshold;
  }

  // SSAO pass
  {
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempSSAOTexture));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "SSAO", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiSSAOConstants", m_hSSAOConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hSSAOShader);

    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindTexture2D("LowResDepthTexture", pDevice->GetDefaultResourceView(hzbTexture));
    renderViewContext.m_pRenderContext->BindSamplerState("DepthSampler", m_hSSAOSamplerState);

    renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, xiiResourceAcquireMode::BlockTillLoaded);

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::Triangles, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Blur pass
  {
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "Blur", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiSSAOConstants", m_hSSAOConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hBlurShader);

    renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", pDevice->GetDefaultResourceView(tempSSAOTexture));

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::Triangles, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Return temp targets
  if (!hzbTexture.IsInvalidated())
  {
    xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hzbTexture);
  }

  if (!tempSSAOTexture.IsInvalidated())
  {
    xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempSSAOTexture);
  }
}

void xiiAOPass::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor              = xiiColor::White;

  auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

void xiiAOPass::SetFadeOutStart(float fStart)
{
  m_fFadeOutStart = xiiMath::Clamp(fStart, 0.0f, m_fFadeOutEnd);
}

float xiiAOPass::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void xiiAOPass::SetFadeOutEnd(float fEnd)
{
  if (m_fFadeOutEnd == fEnd)
    return;

  m_fFadeOutEnd = xiiMath::Max(fEnd, m_fFadeOutStart);

  if (!m_hSSAOSamplerState.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
    m_hSSAOSamplerState.Invalidate();
  }
}

float xiiAOPass::GetFadeOutEnd() const
{
  return m_fFadeOutEnd;
}

void xiiAOPass::CreateSamplerState()
{
  if (m_hSSAOSamplerState.IsInvalidated())
  {
    xiiGALSamplerStateCreationDescription desc;
    desc.m_MinFilter   = xiiGALTextureFilterMode::Point;
    desc.m_MagFilter   = xiiGALTextureFilterMode::Point;
    desc.m_MipFilter   = xiiGALTextureFilterMode::Point;
    desc.m_AddressU    = xiiImageAddressMode::ClampBorder;
    desc.m_AddressV    = xiiImageAddressMode::ClampBorder;
    desc.m_AddressW    = xiiImageAddressMode::ClampBorder;
    desc.m_BorderColor = xiiColor::White * m_fFadeOutEnd;

    m_hSSAOSamplerState = xiiGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AOPass);
