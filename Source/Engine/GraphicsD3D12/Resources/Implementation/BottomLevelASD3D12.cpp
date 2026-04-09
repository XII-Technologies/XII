#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BottomLevelASD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBottomLevelASD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBottomLevelASD3D12::xiiGALBottomLevelASD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALBottomLevelASD3D12::~xiiGALBottomLevelASD3D12() = default;

xiiResult xiiGALBottomLevelASD3D12::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALBottomLevelASD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_BottomLevelASD3D12);
