#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>

xiiGALBufferViewVulkan::xiiGALBufferViewVulkan(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALBufferView(pBuffer, creationDescription)
{
}

xiiGALBufferViewVulkan::~xiiGALBufferViewVulkan() = default;

xiiResult xiiGALBufferViewVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::BufferViewDesc viewDescription;
  viewDescription.ViewType             = xiiDiligentTypeConversions::GetBufferViewType(m_Description.m_ViewType);
  viewDescription.Format.IsNormalized  = m_Description.m_Format.m_bIsNormalized;
  viewDescription.Format.NumComponents = m_Description.m_Format.m_uiComponents;
  viewDescription.Format.ValueType     = xiiDiligentTypeConversions::GetValueType(m_Description.m_Format.m_ValueType);
  viewDescription.ByteOffset           = m_Description.m_uiByteOffset;
  viewDescription.ByteWidth            = m_Description.m_uiByteWidth;

  Diligent::IBuffer* pBufferVulkan = const_cast<Diligent::IBuffer*>(static_cast<xiiGALBufferVulkan*>(m_pBuffer)->GetBuffer());

  pBufferVulkan->CreateView(viewDescription, &m_pBufferView);

  return (m_pBufferView != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALBufferViewVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pBufferView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BufferViewVulkan);
