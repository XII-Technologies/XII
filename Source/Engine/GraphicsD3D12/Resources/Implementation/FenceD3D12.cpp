#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>

xiiGALFenceD3D12::xiiGALFenceD3D12(const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(creationDescription)
{
}

xiiGALFenceD3D12::~xiiGALFenceD3D12() = default;

xiiResult xiiGALFenceD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  Diligent::FenceDesc fenceDescription;
  fenceDescription.Name = m_Description.m_sName.GetStartPointer();
  fenceDescription.Type = xiiDiligentTypeConversions::GetFenceType(m_Description.m_Type);

  pDeviceD3D12->GetDevice()->CreateFence(fenceDescription, &m_pFence);

  return (m_pFence != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALFenceD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pFence);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_FenceD3D12);
