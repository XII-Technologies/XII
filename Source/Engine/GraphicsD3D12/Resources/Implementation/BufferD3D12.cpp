#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>

xiiGALBufferD3D12::xiiGALBufferD3D12(const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(creationDescription)
{
}

xiiGALBufferD3D12::~xiiGALBufferD3D12() = default;

xiiResult xiiGALBufferD3D12::InitPlatform(xiiGALDevice* pDevice, const xiiGALBufferData* pInitialData)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  Diligent::BufferDesc bufferDescription;
  bufferDescription.Name                 = m_Description.m_sName.GetStartPointer();
  bufferDescription.Size                 = m_Description.m_uiSize;
  bufferDescription.BindFlags            = xiiDiligentTypeConversions::GetBindFlags(m_Description.m_BindFlags);
  bufferDescription.Usage                = xiiDiligentTypeConversions::GetUsage(m_Description.m_ResourceUsage);
  bufferDescription.CPUAccessFlags       = xiiDiligentTypeConversions::GetCPUAccessFlags(m_Description.m_CPUAccessFlags);
  bufferDescription.Mode                 = xiiDiligentTypeConversions::GetBufferMode(m_Description.m_Mode);
  bufferDescription.ElementByteStride    = m_Description.m_uiElementByteStride;
  bufferDescription.ImmediateContextMask = m_Description.m_uiImmediateContextMask;

  // If uniform/constant buffer, align size to 64 bytes.
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
    bufferDescription.Size = xiiMemoryUtils::AlignSize(m_Description.m_uiSize, 64ULL);

  // Set the index format for index buffers.
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::IndexBuffer))
    m_IndexFormat = m_Description.m_uiElementByteStride == 2U ? Diligent::VT_UINT16 : Diligent::VT_UINT32;

  if (pInitialData != nullptr)
  {
    Diligent::BufferData initialData = {};
    initialData.pData                = pInitialData->m_pData;
    initialData.DataSize             = pInitialData->m_uiDataSize;

    pDeviceD3D12->GetDevice()->CreateBuffer(bufferDescription, &initialData, &m_pBuffer);
  }
  else
  {
    pDeviceD3D12->GetDevice()->CreateBuffer(bufferDescription, nullptr, &m_pBuffer);
  }

  return (m_pBuffer != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALBufferD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pBuffer);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_BufferD3D12);
