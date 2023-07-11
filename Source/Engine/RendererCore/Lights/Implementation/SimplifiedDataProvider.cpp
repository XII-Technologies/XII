#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Lights/SimplifiedDataProvider.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightDataSimplified.h>
XII_DEFINE_AS_POD_TYPE(xiiSimplifiedDataConstants);

xiiSimplifiedDataGPU::xiiSimplifiedDataGPU()
{
  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiSimplifiedDataConstants>(XII_STRINGIZE(xiiSimplifiedDataConstants));
}

xiiSimplifiedDataGPU::~xiiSimplifiedDataGPU()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiSimplifiedDataGPU::BindResources(xiiRenderContext* pRenderContext)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto hReflectionSpecularTextureView = pDevice->GetDefaultResourceView(xiiReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint));
  auto hSkyIrradianceTextureView      = pDevice->GetDefaultResourceView(xiiReflectionPool::GetSkyIrradianceTexture());

  pRenderContext->BindTextureCube("ReflectionSpecularTexture", hReflectionSpecularTextureView);
  pRenderContext->BindTexture2D("SkyIrradianceTexture", hSkyIrradianceTextureView);

  pRenderContext->BindConstantBuffer(XII_STRINGIZE(xiiSimplifiedDataConstants), m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimplifiedDataProvider, 1, xiiRTTIDefaultAllocator<xiiSimplifiedDataProvider>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSimplifiedDataProvider::xiiSimplifiedDataProvider() = default;

xiiSimplifiedDataProvider::~xiiSimplifiedDataProvider() = default;

void* xiiSimplifiedDataProvider::UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData)
{
  xiiGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetRenderCommandEncoder();

  XII_PROFILE_AND_MARKER(pGALCommandEncoder, "Update Clustered Data");

  if (auto pData = extractedData.GetFrameData<xiiSimplifiedDataCPU>())
  {
    m_Data.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    m_Data.m_cameraUsageHint      = pData->m_cameraUsageHint;

    // Update Constants
    const xiiRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

    xiiSimplifiedDataConstants* pConstants =
      renderViewContext.m_pRenderContext->GetConstantBufferData<xiiSimplifiedDataConstants>(m_Data.m_hConstantBuffer);

    pConstants->SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
  }

  return &m_Data;
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataProvider);
