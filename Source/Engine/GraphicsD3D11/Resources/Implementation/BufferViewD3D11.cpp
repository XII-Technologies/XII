#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/BufferD3D11.h>
#include <GraphicsD3D11/Resources/BufferViewD3D11.h>

xiiGALBufferViewD3D11::xiiGALBufferViewD3D11(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALBufferView(pBuffer, creationDescription)
{
}

xiiGALBufferViewD3D11::~xiiGALBufferViewD3D11() = default;

xiiResult xiiGALBufferViewD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  Diligent::BufferViewDesc viewDescription;
  viewDescription.Name                 = m_Description.m_sName.GetStartPointer();
  viewDescription.ViewType             = xiiDiligentTypeConversions::GetBufferViewType(m_Description.m_ViewType);
  viewDescription.Format.IsNormalized  = m_Description.m_Format.m_bIsNormalized;
  viewDescription.Format.NumComponents = m_Description.m_Format.m_uiComponents;
  viewDescription.Format.ValueType     = xiiDiligentTypeConversions::GetValueType(m_Description.m_Format.m_ValueType);
  viewDescription.ByteOffset           = m_Description.m_uiByteOffset;
  viewDescription.ByteWidth            = m_Description.m_uiByteWidth;

  Diligent::IBuffer* pBufferD3D11 = static_cast<xiiGALBufferD3D11*>(m_pBuffer)->GetBuffer();

  pBufferD3D11->CreateView(viewDescription, &m_pBufferView);

  return (m_pBufferView != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALBufferViewD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pBufferView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_BufferViewD3D11);
