#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionIrradianceConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionFilterPass, 1, xiiRTTIDefaultAllocator<xiiReflectionFilterPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FilteredSpecular", m_PinFilteredSpecular),
    XII_MEMBER_PROPERTY("AvgLuminance", m_PinAvgLuminance),
    XII_MEMBER_PROPERTY("IrradianceData", m_PinIrradianceData),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("SpecularOutputIndex", m_uiSpecularOutputIndex),
    XII_MEMBER_PROPERTY("IrradianceOutputIndex", m_uiIrradianceOutputIndex),
    XII_ACCESSOR_PROPERTY("InputCubemap", GetInputCubemap, SetInputCubemap)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiReflectionFilterPass::xiiReflectionFilterPass() :
  xiiRenderPipelinePass("ReflectionFilterPass"), m_fIntensity(1.0f), m_fSaturation(1.0f), m_uiIrradianceOutputIndex(0)
{
  {
    m_hFilteredSpecularConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiReflectionFilteredSpecularConstants>(XII_STRINGIZE(xiiReflectionFilteredSpecularConstants));
    m_hFilteredSpecularShader         = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ReflectionFilteredSpecular.xiiShader");
    XII_ASSERT_DEV(m_hFilteredSpecularShader.IsValid(), "Could not load ReflectionFilteredSpecular shader!");

    m_hIrradianceConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiReflectionIrradianceConstants>(XII_STRINGIZE(xiiReflectionIrradianceConstants));
    m_hIrradianceShader         = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ReflectionIrradiance.xiiShader");
    XII_ASSERT_DEV(m_hIrradianceShader.IsValid(), "Could not load ReflectionIrradiance shader!");
  }
}

xiiReflectionFilterPass::~xiiReflectionFilterPass()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hIrradianceConstantBuffer);
  m_hIrradianceConstantBuffer.Invalidate();
}

bool xiiReflectionFilterPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  {
    xiiGALTextureCreationDescription desc;
    desc.m_uiWidth                                 = xiiReflectionPool::GetReflectionCubeMapSize();
    desc.m_uiHeight                                = desc.m_uiWidth;
    desc.m_Format                                  = xiiGALResourceFormat::RGBAHalf;
    desc.m_Type                                    = xiiGALTextureType::TextureCube;
    desc.m_bAllowUAV                               = true;
    desc.m_uiMipLevelCount                         = xiiMath::Log2i(desc.m_uiWidth) - 1;
    outputs[m_PinFilteredSpecular.m_uiOutputIndex] = desc;
  }

  return true;
}

void xiiReflectionFilterPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto pInputCubemap = pDevice->GetTexture(m_hInputCubemap);
  if (pInputCubemap == nullptr)
  {
    return;
  }

  // We cannot allow the filter to work on fallback resources as the step will not be repeated for static cube maps. Thus, we force loading the shaders and disable async shader loading in this scope.
  xiiResourceManager::ForceLoadResourceNow(m_hFilteredSpecularShader);
  xiiResourceManager::ForceLoadResourceNow(m_hIrradianceShader);
  bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  xiiGALPass* pGALPass = pDevice->BeginPass(GetName());
  XII_SCOPE_EXIT(
    pDevice->EndPass(pGALPass);
    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

  if (pInputCubemap->GetDescription().m_bAllowDynamicMipGeneration)
  {
    auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(pGALPass, renderViewContext, xiiGALRenderingSetup(), "MipMaps");
    pCommandEncoder->GenerateMipMaps(pDevice->GetDefaultResourceView(m_hInputCubemap));
  }

  {
    auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
    if (pFilteredSpecularOutput != nullptr && !pFilteredSpecularOutput->m_TextureHandle.IsInvalidated())
    {
      xiiUInt32 uiNumMipMaps = pFilteredSpecularOutput->m_Desc.m_uiMipLevelCount;

      xiiUInt32 uiWidth  = pFilteredSpecularOutput->m_Desc.m_uiWidth;
      xiiUInt32 uiHeight = pFilteredSpecularOutput->m_Desc.m_uiHeight;

      auto pCommandEncoder = xiiRenderContext::BeginComputeScope(pGALPass, renderViewContext, "ReflectionFilter");
      renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetDefaultResourceView(m_hInputCubemap));
      renderViewContext.m_pRenderContext->BindConstantBuffer(XII_STRINGIZE(xiiReflectionFilteredSpecularConstants), m_hFilteredSpecularConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);

      for (xiiUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        xiiGALUnorderedAccessViewHandle hFilterOutput;
        {
          xiiGALUnorderedAccessViewCreationDescription desc;
          desc.m_hTexture          = pFilteredSpecularOutput->m_TextureHandle;
          desc.m_uiMipLevelToUse   = uiMipMapIndex;
          desc.m_uiFirstArraySlice = m_uiSpecularOutputIndex * 6;
          desc.m_uiArraySize       = 6;
          hFilterOutput            = pDevice->CreateUnorderedAccessView(desc);
        }
        renderViewContext.m_pRenderContext->BindUAV("ReflectionOutput", hFilterOutput);
        UpdateFilteredSpecularConstantBuffer(uiMipMapIndex, uiNumMipMaps);

        constexpr xiiUInt32 uiThreadsX  = 8;
        constexpr xiiUInt32 uiThreadsY  = 8;
        const xiiUInt32     uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
        const xiiUInt32     uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 6).IgnoreResult();

        uiWidth >>= 1;
        uiHeight >>= 1;
      }
    }
  }

  auto pIrradianceOutput = outputs[m_PinIrradianceData.m_uiOutputIndex];
  if (pIrradianceOutput != nullptr && !pIrradianceOutput->m_TextureHandle.IsInvalidated())
  {
    auto pCommandEncoder = xiiRenderContext::BeginComputeScope(pGALPass, renderViewContext, "Irradiance");

    xiiGALUnorderedAccessViewHandle hIrradianceOutput;
    {
      xiiGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pIrradianceOutput->m_TextureHandle;

      hIrradianceOutput = pDevice->CreateUnorderedAccessView(desc);
    }
    renderViewContext.m_pRenderContext->BindUAV("IrradianceOutput", hIrradianceOutput);

    renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetDefaultResourceView(m_hInputCubemap));

    UpdateIrradianceConstantBuffer();

    renderViewContext.m_pRenderContext->BindConstantBuffer(XII_STRINGIZE(xiiReflectionIrradianceConstants), m_hIrradianceConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

    renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult();
  }
}

xiiUInt32 xiiReflectionFilterPass::GetInputCubemap() const
{
  return m_hInputCubemap.GetInternalID().m_Data;
}

void xiiReflectionFilterPass::SetInputCubemap(xiiUInt32 uiCubemapHandle)
{
  m_hInputCubemap = xiiGALTextureHandle(xiiGAL::xii18_14Id(uiCubemapHandle));
}

void xiiReflectionFilterPass::UpdateFilteredSpecularConstantBuffer(xiiUInt32 uiMipMapIndex, xiiUInt32 uiNumMipMaps)
{
  auto constants        = xiiRenderContext::GetConstantBufferData<xiiReflectionFilteredSpecularConstants>(m_hFilteredSpecularConstantBuffer);
  constants->MipLevel   = uiMipMapIndex;
  constants->Intensity  = m_fIntensity;
  constants->Saturation = m_fSaturation;
}

void xiiReflectionFilterPass::UpdateIrradianceConstantBuffer()
{
  auto constants         = xiiRenderContext::GetConstantBufferData<xiiReflectionIrradianceConstants>(m_hIrradianceConstantBuffer);
  constants->LodLevel    = 6; // TODO: calculate from cubemap size and number of samples
  constants->Intensity   = m_fIntensity;
  constants->Saturation  = m_fSaturation;
  constants->OutputIndex = m_uiIrradianceOutputIndex;
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
