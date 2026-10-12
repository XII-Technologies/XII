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
    xiiGALBufferCreationDescription buffer;
    buffer.m_uiSize                              = sizeof(T);
    buffer.m_Usage                               = xiiGALResourceUsage::Staging;
    buffer.m_CPUAccessFlags                      = xiiGALCPUAccessFlag::Read;
    auto                                 staging = device.CreateBuffer(buffer);
    xiiGALCommandListCreationDescription description;
    description.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
    auto commands            = device.CreateCommandList(description);
    if (!XII_TEST_BOOL(staging != nullptr && commands != nullptr))
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
    auto* queue = device.GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
    queue->WaitForFenceValue(queue->Submit(commands.Borrow()));
    commands->Begin();
    void*      data   = nullptr;
    const auto mapped = commands->MapBuffer(staging.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, data);
    if (mapped.Succeeded() && data)
      xiiMemoryUtils::Copy(&value, static_cast<const T*>(data), 1);
    if (mapped.Succeeded())
      commands->UnmapBuffer(staging.Borrow(), xiiGALMapType::Read).AssertSuccess();
    commands->End();
    return XII_TEST_RESULT(mapped) && XII_TEST_BOOL(data != nullptr);
  }

} // namespace xiiGpuTestReadback
