#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/InstanceDataProvider.h>

#include <Shaders/Common/ObjectConstants.h>

xiiInstanceData::xiiInstanceData(xiiUInt32 uiMaxInstanceCount /*= 1024*/)
{
  CreateBuffer(uiMaxInstanceCount);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_pObjectConstantsBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(pDevice, sizeof(xiiObjectConstants));
}

xiiInstanceData::~xiiInstanceData()
{
  m_pInstanceDataBuffer.Clear();
  m_pObjectConstantsBuffer.Clear();
}

void xiiInstanceData::BindResources(xiiSharedPtr<xiiGALCommandList> pCommandList)
{
#ifdef CORE_ENABLE
  pRenderContext->BindBuffer("perInstanceData", m_pInstanceDataBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindConstantBuffer("xiiObjectConstants", m_hConstantBuffer);
#endif
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

void xiiInstanceData::UpdateInstanceData(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt32 uiCount)
{
  XII_ASSERT_DEV(m_uiBufferOffset + uiCount <= m_uiBufferSize, "Implementation error");

  /// \todo Use dynamic buffer and fix the offset calculation not being applied on the GPU.

  xiiUInt32                   uiDestOffset = m_uiBufferOffset * sizeof(xiiPerInstanceData);
  auto                        pSourceData  = m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
  xiiBitflags<xiiGALMapFlags> mapFlags     = (m_uiBufferOffset == 0) ? xiiGALMapFlags::Discard : xiiGALMapFlags::NoOverWrite;

  xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_pInstanceDataBuffer, uiDestOffset, pSourceData.ToByteArray()).AssertSuccess("Failed to update instance data.");

  {
    xiiGALMapHelper<xiiObjectConstants> pObjectConstants(pCommandList, m_pObjectConstantsBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);

    pObjectConstants->InstanceDataOffset = m_uiBufferOffset;
  }

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

  m_pInstanceDataBuffer = pDevice->CreateBuffer(bufferDescription);
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
