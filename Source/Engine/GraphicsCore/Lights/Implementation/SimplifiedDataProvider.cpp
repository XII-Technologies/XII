#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Lights/SimplifiedDataExtractor.h>
#include <GraphicsCore/Lights/SimplifiedDataProvider.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsFoundation/Profiling/Profiling.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/LightDataSimplified.h>
XII_DEFINE_AS_POD_TYPE(xiiSimplifiedDataConstants);

xiiSimplifiedDataGPU::xiiSimplifiedDataGPU()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_pSimplifiedDataConstantBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(pDevice, sizeof(xiiSimplifiedDataConstants));
}

xiiSimplifiedDataGPU::~xiiSimplifiedDataGPU()
{
  m_pSimplifiedDataConstantBuffer.Clear();
}

void xiiSimplifiedDataGPU::BindResources(xiiRenderContext* pRenderContext)
{
  pRenderContext->BindTexture("ReflectionSpecularTexture", xiiReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint));
  pRenderContext->BindTexture("SkyIrradianceTexture", xiiReflectionPool::GetSkyIrradianceTexture());

  pRenderContext->BindConstantBuffer("xiiSimplifiedDataConstants", m_pSimplifiedDataConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimplifiedDataProvider, 1, xiiRTTIDefaultAllocator<xiiSimplifiedDataProvider>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSimplifiedDataProvider::xiiSimplifiedDataProvider() = default;

xiiSimplifiedDataProvider::~xiiSimplifiedDataProvider() = default;

void* xiiSimplifiedDataProvider::UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData)
{
  if (auto pData = extractedData.GetFrameData<xiiSimplifiedDataCPU>())
  {
    m_Data.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    m_Data.m_cameraUsageHint      = pData->m_cameraUsageHint;

    // Update Constants
    {
      xiiGALMapHelper<xiiSimplifiedDataConstants> pConstants(renderViewContext.m_pRenderContext->GetCommandList(), m_Data.m_pSimplifiedDataConstantBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pConstants->SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    }
  }
  return &m_Data;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_SimplifiedDataProvider);
