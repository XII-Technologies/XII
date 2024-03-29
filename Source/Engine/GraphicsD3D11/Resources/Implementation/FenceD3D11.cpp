#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/FenceD3D11.h>

xiiGALFenceD3D11::xiiGALFenceD3D11(const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(creationDescription), m_pFenceCompleteEvent{CreateEvent(NULL, TRUE, FALSE, NULL)}
{
}

xiiGALFenceD3D11::~xiiGALFenceD3D11() = default;

xiiResult xiiGALFenceD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  XII_ASSERT_DEV(m_pFenceCompleteEvent == NULL, "Failed to create fence complete event.");

  D3D11_FENCE_FLAGS flags = D3D11_FENCE_FLAG_NONE;
  if (FAILED(pDeviceD3D11->GetDeviceD3D11()->CreateFence(0, flags, IID_PPV_ARGS(&m_pFence))))
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALFenceD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  // Schedule deletion on device.
  XII_ASSERT_NOT_IMPLEMENTED;

  if (m_pFenceCompleteEvent != nullptr && m_pFenceCompleteEvent != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_pFenceCompleteEvent);
  }
  return XII_SUCCESS;
}

void xiiGALFenceD3D11::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_pFence != nullptr)
  {
    xiiStringBuilder sb;
    m_pFence->SetPrivateData(WKPDID_D3DDebugObjectName, sName.GetElementCount(), sName.GetData(sb));
  }
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_FenceD3D11);
