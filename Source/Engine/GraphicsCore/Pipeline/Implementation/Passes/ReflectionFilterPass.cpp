#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <GraphicsCore/Pipeline/View.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>
#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/ReflectionIrradianceConstants.h>

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
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiReflectionFilterPass::xiiReflectionFilterPass() :
  xiiRenderPipelinePass("ReflectionFilterPass")
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  {
    m_pFilteredSpecularConstantBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(pDevice, sizeof(xiiReflectionFilteredSpecularConstants));
    m_hFilteredSpecularShader         = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ReflectionFilteredSpecular.xiiShader");
    XII_ASSERT_DEV(m_hFilteredSpecularShader.IsValid(), "Could not load ReflectionFilteredSpecular shader!");

    m_pIrradianceConstantBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(pDevice, sizeof(xiiReflectionIrradianceConstants));
    m_hIrradianceShader         = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ReflectionIrradiance.xiiShader");
    XII_ASSERT_DEV(m_hIrradianceShader.IsValid(), "Could not load ReflectionIrradiance shader!");
  }
}

xiiReflectionFilterPass::~xiiReflectionFilterPass()
{
  m_pFilteredSpecularConstantBuffer.Clear();
  m_pIrradianceConstantBuffer.Clear();
}

bool xiiReflectionFilterPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  {
    xiiGALTextureCreationDescription textureDescription;
    textureDescription.m_Size.width         = xiiReflectionPool::GetReflectionCubeMapSize();
    textureDescription.m_Size.height        = textureDescription.m_Size.width;
    textureDescription.m_Format             = xiiGALResourceFormat::RGBA16Float;
    textureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
    textureDescription.m_uiArraySizeOrDepth = 6U;
    textureDescription.m_uiMipLevels        = xiiMath::Log2i(textureDescription.m_Size.width) - 1;
    textureDescription.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

    outputs[m_PinFilteredSpecular.m_uiOutputIndex] = textureDescription;
  }

  return true;
}

void xiiReflectionFilterPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  if (!m_pInputCubemap)
    return;

  #ifdef CORE_ENABLE
  // We cannot allow the filter to work on fallback resources as the step will not be repeated for static cube maps. Thus, we force loading the shaders and disable async shader loading in this scope.
  xiiResourceManager::ForceLoadResourceNow(m_hFilteredSpecularShader);
  xiiResourceManager::ForceLoadResourceNow(m_hIrradianceShader);
  bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  XII_SCOPE_EXIT(renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

  if (m_pInputCubemap->GetDescription().m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips))
  {
    auto pCommandList = xiiRenderContext::BeginRenderingScope(renderViewContext, xiiGALRenderingSetup(), "MipMaps");
    renderViewContext.m_pRenderContext->GetCommandList()->GenerateMips(m_pInputCubemap->GetDefaultView(xiiGALTextureViewType::ShaderResource));
  }

  {
    auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
    if (pFilteredSpecularOutput != nullptr && pFilteredSpecularOutput->m_pTexture != nullptr)
    {
      xiiUInt32 uiNumMipMaps = pFilteredSpecularOutput->m_TextureDescription.m_uiMipLevels;

      xiiUInt32 uiWidth  = pFilteredSpecularOutput->m_TextureDescription.m_Size.width;
      xiiUInt32 uiHeight = pFilteredSpecularOutput->m_TextureDescription.m_Size.height;

      auto pCommandList = xiiRenderContext::BeginComputeScope(renderViewContext, "ReflectionFilter");
      renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", m_pInputCubemap->GetDefaultView(xiiGALTextureViewType::ShaderResource));
      renderViewContext.m_pRenderContext->BindConstantBuffer("xiiReflectionFilteredSpecularConstants", m_hFilteredSpecularConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);

      for (xiiUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        xiiSharedPtr<xiiGALTextureView> pFilterOutput;
        {
          xiiGALTextureViewCreationDescription viewDescription;
          viewDescription.m_ViewType                  = xiiGALTextureViewType::UnorderedAccess;
          viewDescription.m_ResourceDimension         = xiiGALResourceDimension::Texture2DArray;
          viewDescription.m_uiMostDetailedMip         = uiMipMapIndex;
          viewDescription.m_uiFirstArrayOrDepthSlice  = m_uiSpecularOutputIndex * 6;
          viewDescription.m_uiArrayOrDepthSlicesCount = 6;
          pFilterOutput                               = pFilteredSpecularOutput->m_pTexture->CreateView(viewDescription);
        }
        renderViewContext.m_pRenderContext->BindTextureUAV("ReflectionOutput", pFilterOutput);
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
  if (pIrradianceOutput != nullptr && pIrradianceOutput->m_pTexture != nullptr)
  {
    auto pCommandList = xiiRenderContext::BeginComputeScope(renderViewContext, "Irradiance");

    xiiSharedPtr<xiiGALTextureView> hIrradianceOutput;
    {
      xiiGALTextureViewCreationDescription viewDescription;
      viewDescription.m_ViewType                  = xiiGALTextureViewType::UnorderedAccess;
      viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
      viewDescription.m_uiMostDetailedMip         = 0U;
      viewDescription.m_uiArrayOrDepthSlicesCount = 1U;

      hIrradianceOutput = pIrradianceOutput->m_pTexture->CreateView(viewDescription);
    }
    renderViewContext.m_pRenderContext->BindTextureUAV("IrradianceOutput", hIrradianceOutput);
    renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", m_pInputCubemap->GetDefaultView(xiiGALTextureViewType::ShaderResource));

    UpdateIrradianceConstantBuffer();

    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiReflectionIrradianceConstants", m_hIrradianceConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

    renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult();
  }
#endif
}

xiiResult xiiReflectionFilterPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fIntensity;
  inout_stream << m_fSaturation;
  inout_stream << m_uiSpecularOutputIndex;
  inout_stream << m_uiIrradianceOutputIndex;
  // inout_stream << m_pInputCubemap; Runtime only property
  return XII_SUCCESS;
}

xiiResult xiiReflectionFilterPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fIntensity;
  inout_stream >> m_fSaturation;
  inout_stream >> m_uiSpecularOutputIndex;
  inout_stream >> m_uiIrradianceOutputIndex;
  return XII_SUCCESS;
}

xiiUInt32 xiiReflectionFilterPass::GetInputCubemap() const
{
  XII_ASSERT_NOT_IMPLEMENTED;
  // return m_pInputCubemap.GetInternalID().m_Data;
  return 0;
}

void xiiReflectionFilterPass::SetInputCubemap(xiiUInt32 uiCubemapHandle)
{
  // m_pInputCubemap = xiiGALTextureHandle(xiiGAL::xii24_8Id(uiCubemapHandle));
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiReflectionFilterPass::UpdateFilteredSpecularConstantBuffer(xiiUInt32 uiMipMapIndex, xiiUInt32 uiNumMipMaps)
{
#ifdef CORE_ENABLE
  auto constants        = xiiRenderContext::GetConstantBufferData<xiiReflectionFilteredSpecularConstants>(m_hFilteredSpecularConstantBuffer);
  constants->MipLevel   = uiMipMapIndex;
  constants->Intensity  = m_fIntensity;
  constants->Saturation = m_fSaturation;
  #endif
}

void xiiReflectionFilterPass::UpdateIrradianceConstantBuffer()
{
#ifdef CORE_ENABLE
  auto constants         = xiiRenderContext::GetConstantBufferData<xiiReflectionIrradianceConstants>(m_hIrradianceConstantBuffer);
  constants->LodLevel    = 6; // TODO: calculate from cubemap size and number of samples
  constants->Intensity   = m_fIntensity;
  constants->Saturation  = m_fSaturation;
  constants->OutputIndex = m_uiIrradianceOutputIndex;
#endif
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
