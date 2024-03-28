#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>

xiiGALFenceD3D12::xiiGALFenceD3D12(const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(creationDescription), m_pFenceCompleteEvent{CreateEvent(NULL, TRUE, FALSE, NULL)}
{
}

xiiGALFenceD3D12::~xiiGALFenceD3D12() = default;

xiiResult xiiGALFenceD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  XII_ASSERT_DEV(m_pFenceCompleteEvent == NULL, "Failed to create fence complete event.");

  D3D12_FENCE_FLAGS flags = (m_Description.m_Type == xiiGALFenceType::General) ? D3D12_FENCE_FLAG_SHARED : D3D12_FENCE_FLAG_NONE;
  if (FAILED(pDeviceD3D12->GetDeviceD3D12()->CreateFence(0, flags, IID_PPV_ARGS(&m_pFence))))
  {
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALFenceD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  // Schedule deletion on device.
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_FenceD3D12);
