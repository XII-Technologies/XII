#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

xiiInstanceData::xiiInstanceData(xiiUInt32 uiMaxInstanceCount /*= 1024*/) :
  m_uiBufferSize(0), m_uiBufferOffset(0)
{
  CreateBuffer(uiMaxInstanceCount);

  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiObjectConstants>(XII_STRINGIZE(xiiObjectConstants));
}

xiiInstanceData::~xiiInstanceData()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hInstanceDataBuffer);

  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiInstanceData::BindResources(xiiRenderContext* pRenderContext)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  pRenderContext->BindBuffer("perInstanceData", pDevice->GetDefaultResourceView(m_hInstanceDataBuffer));
  pRenderContext->BindConstantBuffer("xiiObjectConstants", m_hConstantBuffer);
}

xiiArrayPtr<xiiPerInstanceData> xiiInstanceData::GetInstanceData(xiiUInt32 uiCount, xiiUInt32& out_uiOffset)
{
  uiCount = xiiMath::Min(uiCount, m_uiBufferSize);
  if (m_uiBufferOffset + uiCount > m_uiBufferSize)
  {
    m_uiBufferOffset = 0;
  }

  out_uiOffset = m_uiBufferOffset;
  return m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
}

void xiiInstanceData::UpdateInstanceData(xiiRenderContext* pRenderContext, xiiUInt32 uiCount)
{
  XII_ASSERT_DEV(m_uiBufferOffset + uiCount <= m_uiBufferSize, "Implementation error");

  xiiGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  xiiUInt32              uiDestOffset = m_uiBufferOffset * sizeof(xiiPerInstanceData);
  auto                   pSourceData  = m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
  xiiGALUpdateMode::Enum updateMode   = (m_uiBufferOffset == 0) ? xiiGALUpdateMode::Discard : xiiGALUpdateMode::NoOverWrite;

  pGALCommandEncoder->UpdateBuffer(m_hInstanceDataBuffer, uiDestOffset, pSourceData.ToByteArray(), updateMode);


  xiiObjectConstants* pConstants = pRenderContext->GetConstantBufferData<xiiObjectConstants>(m_hConstantBuffer);
  pConstants->InstanceDataOffset = m_uiBufferOffset;

  m_uiBufferOffset += uiCount;
}

void xiiInstanceData::CreateBuffer(xiiUInt32 uiSize)
{
  m_uiBufferSize = uiSize;
  m_PerInstanceData.SetCountUninitialized(m_uiBufferSize);

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALBufferCreationDescription desc;
  desc.m_uiStructSize                = sizeof(xiiPerInstanceData);
  desc.m_uiTotalSize                 = desc.m_uiStructSize * uiSize;
  desc.m_BufferType                  = xiiGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer      = true;
  desc.m_bAllowShaderResourceView    = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hInstanceDataBuffer = pDevice->CreateBuffer(desc);
}

void xiiInstanceData::Reset()
{
  m_uiBufferOffset = 0;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInstanceDataProvider, 1, xiiRTTIDefaultAllocator<xiiInstanceDataProvider>)
{
}
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiInstanceDataProvider::xiiInstanceDataProvider() {}

xiiInstanceDataProvider::~xiiInstanceDataProvider() {}

void* xiiInstanceDataProvider::UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData)
{
  m_Data.Reset();

  return &m_Data;
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_InstanceDataProvider);
