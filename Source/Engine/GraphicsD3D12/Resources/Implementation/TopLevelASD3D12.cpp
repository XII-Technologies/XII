#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTopLevelASD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTopLevelASD3D12::xiiGALTopLevelASD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALTopLevelASD3D12::~xiiGALTopLevelASD3D12() = default;

xiiResult xiiGALTopLevelASD3D12::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALTopLevelASD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

xiiGALTopLevelASInstanceDescription xiiGALTopLevelASD3D12::GetInstanceDescription(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
  return xiiGALTopLevelASInstanceDescription();
}

xiiGALTopLevelASBuildDescription xiiGALTopLevelASD3D12::GetBuildDescription() const
{
  return xiiGALTopLevelASBuildDescription();
}

xiiGALScratchBufferSizeDescription xiiGALTopLevelASD3D12::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TopLevelASD3D12);
