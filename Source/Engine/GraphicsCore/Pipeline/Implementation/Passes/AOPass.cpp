#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Passes/AOPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/Resources/Sampler.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/DownscaleDepthConstants.h>
#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/SSAOConstants.h>

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
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Post Processing")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAOPass::xiiAOPass() :
  xiiRenderPipelinePass("AOPass", true)
{
  m_hNoiseTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>("Textures/SSAONoise.dds");

  m_hDownscaleShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/DownscaleDepth.xiiShader");
  XII_ASSERT_DEV(m_hDownscaleShader.IsValid(), "Could not load downsample shader!");

  m_hSSAOShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SSAO.xiiShader");
  XII_ASSERT_DEV(m_hSSAOShader.IsValid(), "Could not load SSAO shader!");

  m_hBlurShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SSAOBlur.xiiShader");
  XII_ASSERT_DEV(m_hBlurShader.IsValid(), "Could not load SSAO shader!");

  m_hDownscaleConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiDownscaleDepthConstants>();
  m_hSSAOConstantBuffer      = xiiRenderContext::CreateConstantBufferStorage<xiiSSAOConstants>();
}

xiiAOPass::~xiiAOPass()
{
  m_pSSAOSampler.Clear();

  xiiRenderContext::DeleteConstantBufferStorage(m_hDownscaleConstantBuffer);
  m_hDownscaleConstantBuffer.Invalidate();

  xiiRenderContext::DeleteConstantBufferStorage(m_hSSAOConstantBuffer);
  m_hSSAOConstantBuffer.Invalidate();
}

bool xiiAOPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  if (auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex])
  {
    if (!pDepthInput->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    {
      xiiLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    if (pDepthInput->m_uiSampleCount != xiiGALMSAASampleCount::OneSample)
    {
      xiiLog::Error("'{0}' input must be resolved", GetName());
      return false;
    }

    xiiGALTextureCreationDescription desc = *pDepthInput;
    desc.m_Format                         = xiiGALResourceFormat::RG16Float;
    desc.m_BindFlags.Add(xiiGALBindFlags::RenderTarget);
    desc.m_BindFlags.Remove(xiiGALBindFlags::DepthStencil);

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
    return;

  xiiUInt32 uiWidth  = pDepthInput->m_TextureDescription.m_Size.width;
  xiiUInt32 uiHeight = pDepthInput->m_TextureDescription.m_Size.height;

  xiiUInt32 uiNumMips   = 3;
  xiiUInt32 uiHzbWidth  = xiiMath::RoundUp(uiWidth, 1u << uiNumMips);
  xiiUInt32 uiHzbHeight = xiiMath::RoundUp(uiHeight, 1u << uiNumMips);

  float fHzbScaleX = (float)uiWidth / uiHzbWidth;
  float fHzbScaleY = (float)uiHeight / uiHzbHeight;

  // Find temp targets
  xiiSharedPtr<xiiGALTexture>                        pHzbTexture;
  xiiHybridArray<xiiVec2, 8>                         hzbSizes;
  xiiHybridArray<xiiSharedPtr<xiiGALTextureView>, 8> hzbResourceViews;
  xiiHybridArray<xiiSharedPtr<xiiGALTextureView>, 8> hzbRenderTargetViews;

  xiiSharedPtr<xiiGALTexture> pTempSSAOTexture;

  {
    {
      xiiGALTextureCreationDescription desc;
      desc.m_Size.width         = uiHzbWidth / 2;
      desc.m_Size.height        = uiHzbHeight / 2;
      desc.m_uiMipLevels        = 3;
      desc.m_Type               = xiiGALResourceDimension::Texture2DArray;
      desc.m_Format             = xiiGALResourceFormat::R16Float;
      desc.m_BindFlags          = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
      desc.m_uiArraySizeOrDepth = pOutput->m_TextureDescription.m_uiArraySizeOrDepth;

      pHzbTexture = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
    }

    for (xiiUInt32 i = 0; i < uiNumMips; ++i)
    {
      uiHzbWidth  = uiHzbWidth / 2;
      uiHzbHeight = uiHzbHeight / 2;

      hzbSizes.PushBack(xiiVec2((float)uiHzbWidth, (float)uiHzbHeight));

      {
        xiiGALTextureViewCreationDescription desc;
        desc.m_ViewType                  = xiiGALTextureViewType::ShaderResource;
        desc.m_Format                    = xiiGALResourceFormat::R16Float;
        desc.m_uiMostDetailedMip         = i;
        desc.m_uiMipLevelCount           = 1;
        desc.m_uiArrayOrDepthSlicesCount = pOutput->m_TextureDescription.m_uiArraySizeOrDepth;

        hzbResourceViews.PushBack(pHzbTexture->CreateView(desc));
      }

      {
        xiiGALTextureViewCreationDescription desc;
        desc.m_ViewType                  = xiiGALTextureViewType::RenderTarget;
        desc.m_Format                    = xiiGALResourceFormat::R16Float;
        desc.m_uiMostDetailedMip         = i;
        desc.m_uiMipLevelCount           = 1;
        desc.m_uiArrayOrDepthSlicesCount = pOutput->m_TextureDescription.m_uiArraySizeOrDepth;

        hzbRenderTargetViews.PushBack(pHzbTexture->CreateView(desc));
      }
    }

    pTempSSAOTexture = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, xiiGALResourceFormat::RG16Float, xiiGALMSAASampleCount::OneSample, pOutput->m_TextureDescription.m_uiArraySizeOrDepth, true);
  }

  // Mip map passes
  {
    CreateSampler();

    for (xiiUInt32 i = 0; i < uiNumMips; ++i)
    {
      xiiSharedPtr<xiiGALTextureView> pInputView;
      xiiVec2                         pixelSize;

      if (i == 0)
      {
        pInputView = pDepthInput->m_pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
        pixelSize  = xiiVec2(1.0f / uiWidth, 1.0f / uiHeight);
      }
      else
      {
        pInputView = hzbResourceViews[i - 1];
        pixelSize  = xiiVec2(1.0f).CompDiv(hzbSizes[i - 1]);
      }

      xiiSharedPtr<xiiGALTextureView> pOutputView = hzbRenderTargetViews[i];
      xiiVec2                         targetSize  = hzbSizes[i];

      xiiGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pOutputView);
      renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, xiiRectFloat(targetSize.x, targetSize.y), "SSAOMipMaps", renderViewContext.m_pCamera->IsStereoscopic());

      xiiDownscaleDepthConstants* constants = xiiRenderContext::GetConstantBufferData<xiiDownscaleDepthConstants>(m_hDownscaleConstantBuffer);
      constants->PixelSize                  = pixelSize;
      constants->LinearizeDepth             = (i == 0);

      renderViewContext.m_pRenderContext->BindConstantBuffer("xiiDownscaleDepthConstants", m_hDownscaleConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hDownscaleShader);

      renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pInputView);
      renderViewContext.m_pRenderContext->BindSampler("DepthSampler", m_pSSAOSampler);

      renderViewContext.m_pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1);

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
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pTempSSAOTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(renderViewContext, renderingSetup, "SSAO", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiSSAOConstants", m_hSSAOConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hSSAOShader);

    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDepthInput->m_pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    renderViewContext.m_pRenderContext->BindTexture2D("LowResDepthTexture", pHzbTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    renderViewContext.m_pRenderContext->BindSampler("DepthSampler", m_pSSAOSampler);

    renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, xiiResourceAcquireMode::BlockTillLoaded);

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Blur pass
  {
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pOutput->m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(renderViewContext, renderingSetup, "Blur", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiSSAOConstants", m_hSSAOConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hBlurShader);

    renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", pTempSSAOTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Return temp targets
  if (!pHzbTexture)
  {
    xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(pHzbTexture);
  }

  if (!pTempSSAOTexture)
  {
    xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(pTempSSAOTexture);
  }
}

void xiiAOPass::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pOutput->m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor              = xiiColor::White;

  auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName());
}

xiiResult xiiAOPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_fRadius;
  inout_stream << m_fMaxScreenSpaceRadius;
  inout_stream << m_fContrast;
  inout_stream << m_fIntensity;
  inout_stream << m_fFadeOutStart;
  inout_stream << m_fFadeOutEnd;
  inout_stream << m_fPositionBias;
  inout_stream << m_fMipLevelScale;
  inout_stream << m_fDepthBlurThreshold;

  return XII_SUCCESS;
}

xiiResult xiiAOPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_fRadius;
  inout_stream >> m_fMaxScreenSpaceRadius;
  inout_stream >> m_fContrast;
  inout_stream >> m_fIntensity;
  inout_stream >> m_fFadeOutStart;
  inout_stream >> m_fFadeOutEnd;
  inout_stream >> m_fPositionBias;
  inout_stream >> m_fMipLevelScale;
  inout_stream >> m_fDepthBlurThreshold;

  return XII_SUCCESS;
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

  m_pSSAOSampler.Clear();
}

float xiiAOPass::GetFadeOutEnd() const
{
  return m_fFadeOutEnd;
}

void xiiAOPass::CreateSampler()
{
  if (!m_pSSAOSampler)
  {
    xiiGALSamplerCreationDescription desc;
    desc.m_MinFilter          = xiiGALFilterType::Point;
    desc.m_MagFilter          = xiiGALFilterType::Point;
    desc.m_MipFilter          = xiiGALFilterType::Point;
    desc.m_AddressU           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::ClampBorder);
    desc.m_AddressV           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::ClampBorder);
    desc.m_AddressW           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::ClampBorder);
    desc.m_BorderColor        = xiiColor::White * m_fFadeOutEnd;
    desc.m_ComparisonFunction = xiiGALComparisonFunction::Never;
    desc.m_fMipLODBias        = 0.0f;
    desc.m_fMinLOD            = -1.0f;
    desc.m_fMaxLOD            = 42000.0f;
    desc.m_uiMaxAnisotropy    = 4U;

    m_pSSAOSampler = xiiGALDevice::GetDefaultDevice()->CreateSampler(desc);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_AOPass);
