#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferViewD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBufferViewD3D12::xiiGALBufferViewD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALBufferView(pDeviceD3D12, pBuffer, creationDescription)
{
}

xiiGALBufferViewD3D12::~xiiGALBufferViewD3D12() = default;

xiiResult xiiGALBufferViewD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALBufferViewD3D12::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_BufferViewD3D12);
