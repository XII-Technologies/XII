#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTonemapPass, 1, xiiRTTIDefaultAllocator<xiiTonemapPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_PinColorInput),
    XII_MEMBER_PROPERTY("Bloom", m_PinBloomInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ACCESSOR_PROPERTY("VignettingTexture", GetVignettingTextureFile, SetVignettingTextureFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    XII_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::Orange)),
    XII_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new xiiClampValueAttribute(0.0f, 2.0f), new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("LUT1Strength", m_fLut1Strength)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("LUT2Strength", m_fLut2Strength)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("LUT1", GetLUT1TextureFile, SetLUT1TextureFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    XII_ACCESSOR_PROPERTY("LUT2", GetLUT2TextureFile, SetLUT2TextureFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTonemapPass::xiiTonemapPass() :
  xiiRenderPipelinePass("TonemapPass", true)
{
  m_hVignettingTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>("White.color");
  m_hNoiseTexture      = xiiResourceManager::LoadResource<xiiTexture2DResource>("Textures/BlueNoise.dds");

  m_MoodColor     = xiiColor::Orange;
  m_fMoodStrength = 0.0f;
  m_fSaturation   = 1.0f;
  m_fContrast     = 1.0f;
  m_fLut1Strength = 1.0f;
  m_fLut2Strength = 0.0f;

  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Tonemap.xiiShader");
  XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");

  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiTonemapConstants>(XII_STRINGIZE(xiiTonemapConstants));
}

xiiTonemapPass::~xiiTonemapPass()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool xiiTonemapPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  xiiGALDevice*              pDevice       = xiiGALDevice::GetDefaultDevice();
  const xiiGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Color
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  if (pColorInput != nullptr)
  {
    if (const xiiGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      const xiiGALTextureCreationDescription& desc = pTexture->GetDescription();

#if 0
      if (desc.m_uiWidth != pColorInput->m_uiWidth || desc.m_uiHeight != pColorInput->m_uiHeight)
      {
        xiiLog::Error("Render target sizes don't match");
        return false;
      }
#endif

      outputs[m_PinOutput.m_uiOutputIndex].SetAsRenderTarget(pColorInput->m_uiWidth, pColorInput->m_uiHeight, desc.m_Format);
      outputs[m_PinOutput.m_uiOutputIndex].m_uiArraySize = pColorInput->m_uiArraySize;
      outputs[m_PinOutput.m_uiOutputIndex].m_Type        = xiiGALTextureType::Texture2DArray;
    }
    else
    {
      xiiLog::Error("View '{0}' does not have a valid color target", view.GetName());
      return false;
    }
  }
  else
  {
    xiiLog::Error("No input connected to tone map pass!");
    return false;
  }

  return true;
}

void xiiTonemapPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput  = inputs[m_PinColorInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Setup render target
  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  // Determine how many LUTs are active
  xiiUInt32                  numLUTs         = 0;
  xiiTexture3DResourceHandle luts[2]         = {};
  float                      lutStrengths[2] = {};

  if (m_hLUT1.IsValid())
  {
    luts[numLUTs]         = m_hLUT1;
    lutStrengths[numLUTs] = m_fLut1Strength;
    numLUTs++;
  }

  if (m_hLUT2.IsValid())
  {
    luts[numLUTs]         = m_hLUT2;
    lutStrengths[numLUTs] = m_fLut2Strength;
    numLUTs++;
  }

  {
    xiiTonemapConstants* constants = xiiRenderContext::GetConstantBufferData<xiiTonemapConstants>(m_hConstantBuffer);
    constants->AutoExposureParams.SetZero();
    constants->MoodColor    = m_MoodColor;
    constants->MoodStrength = m_fMoodStrength;
    constants->Saturation   = m_fSaturation;
    constants->Lut1Strength = lutStrengths[0];
    constants->Lut2Strength = lutStrengths[1];

    // Pre-calculate factors of a s-shaped polynomial-function
    const float m = (0.5f - 0.5f * m_fContrast) / (0.5f + 0.5f * m_fContrast);
    const float a = 2.0f * m - 2.0f;
    const float b = -3.0f * m + 3.0f;

    constants->ContrastParams = xiiVec4(a, b, m, 0.0f);
  }

  xiiGALResourceViewHandle hBloomTextureView;
  auto                     pBloomInput = inputs[m_PinBloomInput.m_uiInputIndex];
  if (pBloomInput != nullptr)
  {
    hBloomTextureView = pDevice->GetDefaultResourceView(pBloomInput->m_TextureHandle);
  }

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindConstantBuffer(XII_STRINGIZE(xiiTonemapConstants), m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("VignettingTexture", m_hVignettingTexture, xiiResourceAcquireMode::BlockTillLoaded);
  renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, xiiResourceAcquireMode::BlockTillLoaded);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColorTexture", pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle));
  renderViewContext.m_pRenderContext->BindTexture2D("BloomTexture", hBloomTextureView);
  renderViewContext.m_pRenderContext->BindTexture3D("Lut1Texture", luts[0]);
  renderViewContext.m_pRenderContext->BindTexture3D("Lut2Texture", luts[1]);

  xiiTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

void xiiTonemapPass::SetVignettingTextureFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hVignettingTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(szFile);
  }
}

const char* xiiTonemapPass::GetVignettingTextureFile() const
{
  if (!m_hVignettingTexture.IsValid())
    return "";

  return m_hVignettingTexture.GetResourceID();
}


void xiiTonemapPass::SetLUT1TextureFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT1 = xiiResourceManager::LoadResource<xiiTexture3DResource>(szFile);
  }
}

const char* xiiTonemapPass::GetLUT1TextureFile() const
{
  if (!m_hLUT1.IsValid())
    return "";

  return m_hLUT1.GetResourceID();
}

void xiiTonemapPass::SetLUT2TextureFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT2 = xiiResourceManager::LoadResource<xiiTexture3DResource>(szFile);
  }
}

const char* xiiTonemapPass::GetLUT2TextureFile() const
{
  if (!m_hLUT2.IsValid())
    return "";

  return m_hLUT2.GetResourceID();
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);
