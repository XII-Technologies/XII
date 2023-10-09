#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>

xiiGALBufferViewD3D12::xiiGALBufferViewD3D12(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALBufferView(pBuffer, creationDescription)
{
}

xiiGALBufferViewD3D12::~xiiGALBufferViewD3D12() = default;

xiiResult xiiGALBufferViewD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  Diligent::BufferViewDesc viewDescription;
  viewDescription.ViewType             = xiiDiligentTypeConversions::GetBufferViewType(m_Description.m_ViewType);
  viewDescription.Format.IsNormalized  = m_Description.m_Format.m_bIsNormalized;
  viewDescription.Format.NumComponents = m_Description.m_Format.m_uiComponents;
  viewDescription.Format.ValueType     = xiiDiligentTypeConversions::GetValueType(m_Description.m_Format.m_ValueType);
  viewDescription.ByteOffset           = m_Description.m_uiByteOffset;
  viewDescription.ByteWidth            = m_Description.m_uiByteWidth;

  Diligent::IBuffer* pBufferD3D12 = const_cast<Diligent::IBuffer*>(static_cast<xiiGALBufferD3D12*>(m_pBuffer)->GetBuffer());

  pBufferD3D12->CreateView(viewDescription, &m_pBufferView);

  return (m_pBufferView != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALBufferViewD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pBufferView);

  return XII_SUCCESS;
}
