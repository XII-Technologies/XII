#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueue, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALCommandQueue::xiiGALCommandQueue(xiiGALDevice* pDevice, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALObject(), m_Description(creationDescription), m_pDevice(pDevice)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Invalid command queue device provided.");
}

xiiGALCommandQueue::~xiiGALCommandQueue() = default;

xiiUInt64 xiiGALCommandQueue::Submit(xiiSharedPtr<xiiGALCommandList> pCommandList)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(pCommandList != nullptr, "Submit failed: Command list is null.");

  const xiiGALCommandListCreationDescription& description = pCommandList->GetDescription();

  XII_ASSERT_DEV(m_Description.m_QueueFlags.AreAllSet(description.m_QueueFlags), "Submit failed: Command list queue flags [{0}] are incompatible with this queue [{1}].", description.m_QueueFlags.GetValue(), m_Description.m_QueueFlags.GetValue());

  const xiiGALCommandList::RecordingState recordingState = pCommandList->GetRecordingState();

  XII_ASSERT_DEV(recordingState != xiiGALCommandList::RecordingState::Reset && recordingState != xiiGALCommandList::RecordingState::Submitted, "Submit failed: Command list is in an invalid state [{0}]. Valid states: Recording or Ended.", static_cast<xiiUInt8>(recordingState));
#endif

  if (pCommandList->GetRecordingState() == xiiGALCommandList::RecordingState::Recording)
  {
    pCommandList->End();
  }

  return SubmitPlatform(pCommandList);
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandQueue);
