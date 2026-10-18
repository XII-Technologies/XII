/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <TestFramework/Framework/TestFramework.h>
namespace xiiGpuTestReadback
{
  template <typename T>
  bool ReadBufferRecord(xiiGALDevice& device, xiiGALBuffer* source, xiiUInt64 offset, T& value)
  {
    if (!XII_TEST_BOOL(source != nullptr))
      return false;
    xiiGALBufferCreationDescription description;
    description.m_uiSize         = sizeof(T);
    description.m_Usage          = xiiGALResourceUsage::Staging;
    description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
    auto staging                 = device.CreateBuffer(description);
    auto commands                = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!XII_TEST_BOOL(staging && commands))
      return false;
    commands->Begin();
    const auto previous = source->GetResourceState();
    commands->CopyBufferRegion(source, offset, staging.Borrow(), 0, sizeof(T));
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = source;
    transition.m_NewState        = previous;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    commands->End();
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    commands->Begin();
    void*      mapped = nullptr;
    const auto result = commands->MapBuffer(staging.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped);
    if (result.Succeeded() && mapped)
      xiiMemoryUtils::Copy(&value, static_cast<const T*>(mapped), 1);
    if (result.Succeeded())
      commands->UnmapBuffer(staging.Borrow(), xiiGALMapType::Read).AssertSuccess();
    commands->End();
    return XII_TEST_RESULT(result) && XII_TEST_BOOL(mapped != nullptr);
  }

} // namespace xiiGpuTestReadback
