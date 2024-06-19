#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Lights/SimplifiedDataExtractor.h>
#include <GraphicsCore/Lights/SimplifiedDataProvider.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Profiling/Profiling.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/LightDataSimplified.h>
XII_DEFINE_AS_POD_TYPE(xiiSimplifiedDataConstants);

xiiSimplifiedDataGPU::xiiSimplifiedDataGPU()
{
  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiSimplifiedDataConstants>();
}

xiiSimplifiedDataGPU::~xiiSimplifiedDataGPU()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiSimplifiedDataGPU::BindResources(xiiRenderContext* pRenderContext)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto hReflectionSpecularTextureView = pDevice->GetTexture(xiiReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint))->GetDefaultView(xiiGALTextureViewType::ShaderResource);
  auto hSkyIrradianceTextureView      = pDevice->GetTexture(xiiReflectionPool::GetSkyIrradianceTexture())->GetDefaultView(xiiGALTextureViewType::ShaderResource);

  pRenderContext->BindTextureCube("ReflectionSpecularTexture", hReflectionSpecularTextureView);
  pRenderContext->BindTexture2D("SkyIrradianceTexture", hSkyIrradianceTextureView);

  pRenderContext->BindConstantBuffer("xiiSimplifiedDataConstants", m_hConstantBuffer);
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
  xiiGALCommandList* pGALCommandList = renderViewContext.m_pRenderContext->GetCommandList();

  XII_PROFILE_AND_MARKER(pGALCommandList, "Update Clustered Data");

  if (auto pData = extractedData.GetFrameData<xiiSimplifiedDataCPU>())
  {
    m_Data.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    m_Data.m_cameraUsageHint      = pData->m_cameraUsageHint;

    // Update Constants
    const xiiRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

    xiiSimplifiedDataConstants* pConstants = renderViewContext.m_pRenderContext->GetConstantBufferData<xiiSimplifiedDataConstants>(m_Data.m_hConstantBuffer);

    pConstants->SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
  }

  return &m_Data;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_SimplifiedDataProvider);
