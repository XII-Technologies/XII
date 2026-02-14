#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALQueryD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALQueryD3D12::xiiGALQueryD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(pDeviceD3D12, creationDescription)
{
}

xiiGALQueryD3D12::~xiiGALQueryD3D12() = default;

xiiResult xiiGALQueryD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALQueryD3D12::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

bool xiiGALQueryD3D12::GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate)
{
  return false;
}

void xiiGALQueryD3D12::Invalidate()
{
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_QueryD3D12);
