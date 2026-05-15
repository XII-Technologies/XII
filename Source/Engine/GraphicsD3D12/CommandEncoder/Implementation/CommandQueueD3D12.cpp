/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueueD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALCommandQueueD3D12::xiiGALCommandQueueD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALCommandQueueCreationDescription& creationDescription, const xiiGALQueueInformationD3D12& queueInformation) :
  xiiGALCommandQueue(pDeviceD3D12, creationDescription), m_QueueInformation(queueInformation)
{
}

xiiGALCommandQueueD3D12::~xiiGALCommandQueueD3D12() = default;

xiiUInt64 xiiGALCommandQueueD3D12::GetCompletedFenceValue()
{
  return xiiMath::MaxValue<xiiUInt64>();
}

xiiUInt64 xiiGALCommandQueueD3D12::SubmitPlatform(xiiGALCommandList* pCommandList)
{
  XII_IGNORE_UNUSED(pCommandList);

  return xiiMath::MaxValue<xiiUInt64>();
}

xiiUInt64 xiiGALCommandQueueD3D12::WaitForIdle()
{
  return xiiMath::MaxValue<xiiUInt64>();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandQueueD3D12);
