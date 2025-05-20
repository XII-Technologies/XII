#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/InstanceDataProvider.h>
#include <GraphicsCore/RenderContext/RenderContext.h>


#include <Shaders/Common/ObjectConstants.h>

xiiInstanceData::xiiInstanceData(xiiUInt32 uiMaxInstanceCount /*= 1024*/)
{
  CreateBuffer(uiMaxInstanceCount);

  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiObjectConstants>();
}

xiiInstanceData::~xiiInstanceData()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hInstanceDataBuffer);

  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiInstanceData::BindResources(xiiRenderContext* pRenderContext)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  pRenderContext->BindBuffer("perInstanceData", pDevice->GetBuffer(m_hInstanceDataBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource));
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

void xiiInstanceData::UpdateInstanceData(xiiGALCommandList* pCommandList, xiiUInt32 uiCount)
{
  XII_ASSERT_DEV(m_uiBufferOffset + uiCount <= m_uiBufferSize, "Implementation error");

  xiiUInt32                   uiDestOffset = m_uiBufferOffset * sizeof(xiiPerInstanceData);
  auto                        pSourceData  = m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
  xiiBitflags<xiiGALMapFlags> mapFlags     = (m_uiBufferOffset == 0) ? xiiGALMapFlags::Discard : xiiGALMapFlags::NoOverWrite;

  xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_hInstanceDataBuffer, uiDestOffset, pSourceData.ToByteArray()).AssertSuccess();

  xiiObjectConstants* pConstants = xiiRenderContext::GetConstantBufferData<xiiObjectConstants>(m_hConstantBuffer);
  pConstants->InstanceDataOffset = m_uiBufferOffset;

  m_uiBufferOffset += uiCount;
}

void xiiInstanceData::CreateBuffer(xiiUInt32 uiSize)
{
  m_uiBufferSize = uiSize;
  m_PerInstanceData.SetCountUninitialized(m_uiBufferSize);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  bufferDescription.m_uiElementByteStride = sizeof(xiiPerInstanceData);
  bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * uiSize;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
  bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

  m_hInstanceDataBuffer = pDevice->CreateBuffer(bufferDescription);
}

void xiiInstanceData::Reset()
{
  m_uiBufferOffset = 0;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInstanceDataProvider, 1, xiiRTTIDefaultAllocator<xiiInstanceDataProvider>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiInstanceDataProvider::xiiInstanceDataProvider()  = default;
xiiInstanceDataProvider::~xiiInstanceDataProvider() = default;

void* xiiInstanceDataProvider::UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData)
{
  m_Data.Reset();

  return &m_Data;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_InstanceDataProvider);
