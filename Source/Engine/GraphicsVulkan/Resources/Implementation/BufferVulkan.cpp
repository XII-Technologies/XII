#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>

xiiGALBufferVulkan::xiiGALBufferVulkan(const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(creationDescription)
{
}

xiiGALBufferVulkan::~xiiGALBufferVulkan() = default;

xiiResult xiiGALBufferVulkan::InitPlatform(xiiGALDevice* pDevice, const xiiGALBufferData* pInitialData)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

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

  if (pInitialData != nullptr)
  {
    Diligent::BufferData initialData = {};
    initialData.pData                = pInitialData->m_pData;
    initialData.DataSize             = pInitialData->m_uiDataSize;

    pDeviceVulkan->GetDevice()->CreateBuffer(bufferDescription, &initialData, &m_pBuffer);
  }
  else
  {
    pDeviceVulkan->GetDevice()->CreateBuffer(bufferDescription, nullptr, &m_pBuffer);
  }

  return (m_pBuffer != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALBufferVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pBuffer);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BufferVulkan);
