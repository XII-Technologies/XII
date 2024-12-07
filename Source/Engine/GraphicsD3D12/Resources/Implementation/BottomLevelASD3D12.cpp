#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BottomLevelASD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBottomLevelASD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBottomLevelASD3D12::xiiGALBottomLevelASD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(pDeviceD3D12, creationDescription)
{
}

xiiGALBottomLevelASD3D12::~xiiGALBottomLevelASD3D12() = default;

xiiResult xiiGALBottomLevelASD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALBottomLevelASD3D12::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

xiiUInt32 xiiGALBottomLevelASD3D12::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  return xiiUInt32();
}

xiiUInt32 xiiGALBottomLevelASD3D12::GetGeometryIndex(xiiStringView sName) const
{
  return xiiUInt32();
}

xiiUInt32 xiiGALBottomLevelASD3D12::GetActualGeometryCount() const
{
  return xiiUInt32();
}

xiiGALScratchBufferSizeDescription xiiGALBottomLevelASD3D12::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_BottomLevelASD3D12);
