#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/CommandEncoder/CommandListNull.h>
#include <GraphicsNull/CommandEncoder/CommandQueueNull.h>
#include <GraphicsNull/Device/DeviceNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueueNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALCommandQueueNull::xiiGALCommandQueueNull(xiiGALDeviceNull* pDeviceNull, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceNull, creationDescription)
{
  xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
  m_pDefaultCommandList                                       = XII_DEFAULT_NEW(xiiGALCommandListNull, xiiSharedPtr<xiiGALDeviceNull>(pDeviceNull, pDeviceNull->GetAllocator()), this, commandListDescription);
}

xiiGALCommandQueueNull::~xiiGALCommandQueueNull() = default;

xiiResult xiiGALCommandQueueNull::InitPlatform()
{
  XII_DELETE(m_pDefaultCommandList.m_pAllocator, m_pDefaultCommandList.m_pInstance);

  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALCommandList> xiiGALCommandQueueNull::BeginCommandList()
{
  return m_pDefaultCommandList;
}

xiiUInt64 xiiGALCommandQueueNull::Submit(xiiGALCommandList* pCommandList)
{
  XII_IGNORE_UNUSED(pCommandList);

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
