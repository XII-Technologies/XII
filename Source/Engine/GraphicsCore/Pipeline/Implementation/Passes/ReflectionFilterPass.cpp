#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Resources/Texture.h>

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
  {
    m_hFilteredSpecularConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiReflectionFilteredSpecularConstants>();
    m_hFilteredSpecularShader         = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ReflectionFilteredSpecular.xiiShader");
    XII_ASSERT_DEV(m_hFilteredSpecularShader.IsValid(), "Could not load ReflectionFilteredSpecular shader!");

    m_hIrradianceConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiReflectionIrradianceConstants>();
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
    desc.m_Size.width         = xiiReflectionPool::GetReflectionCubeMapSize();
    desc.m_Size.height        = desc.m_Size.width;
    desc.m_Format             = xiiGALResourceFormat::RGBA16Float;
    desc.m_Type               = xiiGALResourceDimension::TextureCube;
    desc.m_uiArraySizeOrDepth = 6U;
    desc.m_uiMipLevels        = xiiMath::Log2i(desc.m_Size.width) - 1;
    desc.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

    outputs[m_PinFilteredSpecular.m_uiOutputIndex] = desc;
  }

  return true;
}

void xiiReflectionFilterPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto pInputCubemap = pDevice->GetTexture(m_hInputCubemap);
  if (pInputCubemap == nullptr)
    return;

  // We cannot allow the filter to work on fallback resources as the step will not be repeated for static cube maps. Thus, we force loading the shaders and disable async shader loading in this scope.
  xiiResourceManager::ForceLoadResourceNow(m_hFilteredSpecularShader);
  xiiResourceManager::ForceLoadResourceNow(m_hIrradianceShader);
  bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  XII_SCOPE_EXIT(renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

  if (pInputCubemap->GetDescription().m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips))
  {
    auto pCommandList = xiiRenderContext::BeginRenderingScope(renderViewContext, xiiGALRenderingSetup(), "MipMaps");
    renderViewContext.m_pRenderContext->GetCommandList()->GenerateMips(pDevice->GetTexture(m_hInputCubemap)->GetDefaultView(xiiGALTextureViewType::ShaderResource));
  }

  {
    auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
    if (pFilteredSpecularOutput != nullptr && !pFilteredSpecularOutput->m_TextureHandle.IsInvalidated())
    {
      xiiUInt32 uiNumMipMaps = pFilteredSpecularOutput->m_TextureDescription.m_uiMipLevels;

      xiiUInt32 uiWidth  = pFilteredSpecularOutput->m_TextureDescription.m_Size.width;
      xiiUInt32 uiHeight = pFilteredSpecularOutput->m_TextureDescription.m_Size.height;

      auto pCommandList = xiiRenderContext::BeginComputeScope(renderViewContext, "ReflectionFilter");
      renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetTexture(m_hInputCubemap)->GetDefaultView(xiiGALTextureViewType::ShaderResource));
      renderViewContext.m_pRenderContext->BindConstantBuffer("xiiReflectionFilteredSpecularConstants", m_hFilteredSpecularConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);

      for (xiiUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        xiiGALTextureViewHandle hFilterOutput;
        {
          xiiGALTextureViewCreationDescription desc;
          desc.m_ViewType                  = xiiGALTextureViewType::UnorderedAccess;
          desc.m_ResourceDimension         = xiiGALResourceDimension::Texture2DArray;
          desc.m_hTexture                  = pFilteredSpecularOutput->m_TextureHandle;
          desc.m_uiMostDetailedMip         = uiMipMapIndex;
          desc.m_uiFirstArrayOrDepthSlice  = m_uiSpecularOutputIndex * 6;
          desc.m_uiArrayOrDepthSlicesCount = 6;
          hFilterOutput                    = pDevice->CreateTextureView(desc);
        }
        renderViewContext.m_pRenderContext->BindTextureUAV("ReflectionOutput", hFilterOutput);
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
    auto pCommandList = xiiRenderContext::BeginComputeScope(renderViewContext, "Irradiance");

    xiiGALTextureViewHandle hIrradianceOutput;
    {
      xiiGALTextureViewCreationDescription desc;
      desc.m_ViewType                  = xiiGALTextureViewType::UnorderedAccess;
      desc.m_hTexture                  = pIrradianceOutput->m_TextureHandle;
      desc.m_uiFirstArrayOrDepthSlice  = 0U;
      desc.m_uiMostDetailedMip         = 0U;
      desc.m_uiArrayOrDepthSlicesCount = 1U;

      hIrradianceOutput = pDevice->CreateTextureView(desc);
    }
    renderViewContext.m_pRenderContext->BindTextureUAV("IrradianceOutput", hIrradianceOutput);

    renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetTexture(m_hInputCubemap)->GetDefaultView(xiiGALTextureViewType::ShaderResource));

    UpdateIrradianceConstantBuffer();

    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiReflectionIrradianceConstants", m_hIrradianceConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

    renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult();
  }
}

xiiResult xiiReflectionFilterPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fIntensity;
  inout_stream << m_fSaturation;
  inout_stream << m_uiSpecularOutputIndex;
  inout_stream << m_uiIrradianceOutputIndex;
  // inout_stream << m_hInputCubemap; Runtime only property
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
  return m_hInputCubemap.GetInternalID().m_Data;
}

void xiiReflectionFilterPass::SetInputCubemap(xiiUInt32 uiCubemapHandle)
{
  m_hInputCubemap = xiiGALTextureHandle(xiiGAL::xii24_8Id(uiCubemapHandle));
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
