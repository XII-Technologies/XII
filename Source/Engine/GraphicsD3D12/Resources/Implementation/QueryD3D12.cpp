#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALQueryD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALQueryD3D12::xiiGALQueryD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALQueryD3D12::~xiiGALQueryD3D12() = default;

xiiResult xiiGALQueryD3D12::InitPlatform()
{
  return XII_FAILURE;
}

bool xiiGALQueryD3D12::GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate)
{
  XII_IGNORE_UNUSED(pData);
  XII_IGNORE_UNUSED(uiDataSize);
  XII_IGNORE_UNUSED(bAutoInvalidate);
  return false;
}

void xiiGALQueryD3D12::Invalidate()
{
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_QueryD3D12);
