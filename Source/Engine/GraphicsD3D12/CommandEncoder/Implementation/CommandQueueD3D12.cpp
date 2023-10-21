#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>

xiiGALCommandQueueD3D12::xiiGALCommandQueueD3D12(const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(creationDescription)
{
}

xiiGALCommandQueueD3D12::~xiiGALCommandQueueD3D12() = default;

xiiResult xiiGALCommandQueueD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  XII_ASSERT_NOT_IMPLEMENTED;

  return m_pCommandQueue == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALCommandQueueD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pCommandQueue);

  return XII_SUCCESS;
}
