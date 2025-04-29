#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/CommandEncoder/CommandListNull.h>
#include <GraphicsNull/CommandEncoder/CommandQueueNull.h>
#include <GraphicsNull/Device/DeviceNull.h>

xiiGALCommandQueueNull::xiiGALCommandQueueNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceNull, creationDescription)
{
  xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
  m_pDefaultCommandList                                       = XII_DEFAULT_NEW(xiiGALCommandListNull, pDeviceNull, this, commandListDescription);
}

xiiGALCommandQueueNull::~xiiGALCommandQueueNull() = default;

xiiResult xiiGALCommandQueueNull::InitPlatform()
{
  return XII_SUCCESS;
}

xiiGALCommandList* xiiGALCommandQueueNull::BeginCommandList()
{
  return m_pDefaultCommandList.Borrow();
}

xiiUInt64 xiiGALCommandQueueNull::Submit(xiiGALCommandList* pCommandList)
{
  if (pCommandList)
  {
    pCommandList->Reset();
  }
  return 0U;
}

xiiUInt64 xiiGALCommandQueueNull::GetNextFenceValue() const
{
  return 0ULL;
}

xiiUInt64 xiiGALCommandQueueNull::GetCompletedFenceValue()
{
  return 0ULL;
}

xiiUInt64 xiiGALCommandQueueNull::WaitForIdle()
{
  return 0ULL;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_CommandEncoder_Implementation_CommandQueueNull);
