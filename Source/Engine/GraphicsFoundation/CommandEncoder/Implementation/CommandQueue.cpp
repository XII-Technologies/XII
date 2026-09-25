/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

xiiUInt64 xiiGALCommandQueue::Submit(xiiGALCommandList* pCommandList)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  {
    XII_ASSERT_DEV(pCommandList != nullptr, "Submit failed: Command list is null.");

    const xiiGALCommandListCreationDescription& description = pCommandList->GetDescription();

    XII_ASSERT_DEV(m_Description.m_QueueFlags.AreAllSet(description.m_QueueFlags), "Submit failed: Command list queue flags [{0}] are incompatible with this queue [{1}].", xiiArgEnum(description.m_QueueFlags), xiiArgEnum(m_Description.m_QueueFlags));

    const xiiGALCommandList::RecordingState recordingState = pCommandList->GetRecordingState();

    XII_ASSERT_DEV(recordingState == xiiGALCommandList::RecordingState::Ended, "Submit(): primary command list must have been Ended before submission (current state={0}).", static_cast<xiiUInt8>(pCommandList->GetRecordingState()));
  }
#endif

  const xiiGALCommandListCreationDescription& description = pCommandList->GetDescription();

  xiiUInt64 uiFenceValue = SubmitPlatform(pCommandList);

  if (!description.m_Flags.IsSet(xiiGALCommandListFlags::MultiSubmit))
  {
    pCommandList->Reset();
  }

  return uiFenceValue;
}

void xiiGALCommandQueue::WaitForFenceValue(xiiUInt64 uiFenceValue)
{
  if (GetCompletedFenceValue() < uiFenceValue)
    WaitForIdle();
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandQueue);
