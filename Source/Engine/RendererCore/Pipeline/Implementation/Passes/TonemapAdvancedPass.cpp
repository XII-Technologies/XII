#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TonemapAdvancedPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTonemapMode, 1)
  XII_ENUM_CONSTANT(xiiTonemapMode::Exponential),
  XII_ENUM_CONSTANT(xiiTonemapMode::Reinhard),
  XII_ENUM_CONSTANT(xiiTonemapMode::ReinhardModified),
  XII_ENUM_CONSTANT(xiiTonemapMode::Uncharted2),
  XII_ENUM_CONSTANT(xiiTonemapMode::FilmicALU),
  XII_ENUM_CONSTANT(xiiTonemapMode::Logarithmic),
  XII_ENUM_CONSTANT(xiiTonemapMode::AdaptiveLogarithmic),
  XII_ENUM_CONSTANT(xiiTonemapMode::Lottes),
  XII_ENUM_CONSTANT(xiiTonemapMode::Uchimura),
  XII_ENUM_CONSTANT(xiiTonemapMode::Unreal),
  XII_ENUM_CONSTANT(xiiTonemapMode::Aces),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTonemapAdvancedPass, 1, xiiRTTIDefaultAllocator<xiiTonemapAdvancedPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_PinColorInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),

    XII_ENUM_MEMBER_PROPERTY("TonemapMode", xiiTonemapMode, m_TonemapMode),
    XII_MEMBER_PROPERTY("AutoExposure", m_bAutoExposure)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("MiddleGray", m_fMiddleGray)->AddAttributes(new xiiDefaultValueAttribute(0.18f)),
    XII_MEMBER_PROPERTY("LightAdaptation", m_bLightAdaptation)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("WhitePoint", m_fWhitePoint)->AddAttributes(new xiiDefaultValueAttribute(3.0f)),
    XII_MEMBER_PROPERTY("LuminanceSaturation", m_fLuminanceSaturation)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("AverageLogLum", m_fAverageLogLum)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTonemapAdvancedPass::xiiTonemapAdvancedPass() :
  xiiRenderPipelinePass("TonemapAdvancedPass", true)
{
  m_TonemapMode          = xiiTonemapMode::Aces;
  m_bAutoExposure        = true;
  m_fMiddleGray          = 0.18f;
  m_bLightAdaptation     = true;
  m_fWhitePoint          = 3.0f;
  m_fLuminanceSaturation = 1.0f;

  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/TonemapAdvanced.xiiShader");
  XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");

  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiAdvancedTonemapConstants>(XII_STRINGIZE(xiiTonemapAdvancedPass));
}

xiiTonemapAdvancedPass::~xiiTonemapAdvancedPass()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool xiiTonemapAdvancedPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
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

void xiiTonemapAdvancedPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
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

  {
    xiiAdvancedTonemapConstants* constants = xiiRenderContext::GetConstantBufferData<xiiAdvancedTonemapConstants>(m_hConstantBuffer);
    constants->ToneMappingMode             = m_TonemapMode.GetValue();
    constants->AutoExposure                = m_bAutoExposure;
    constants->MiddleGray                  = m_fMiddleGray;
    constants->LightAdaptation             = m_bLightAdaptation;
    constants->WhitePoint                  = m_fWhitePoint;
    constants->LightAdaptation             = m_fLuminanceSaturation;
    constants->AverageLogLum               = m_fAverageLogLum;
    constants->Padding0                    = 0;
  }

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindConstantBuffer("xiiAdvancedTonemapConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColorTexture", pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapAdvancedPass);
