/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <TestFramework/Framework/TestFramework.h>

namespace xiiGpuTestReadback
{
  template <typename T>
  bool ReadTexturePixels(xiiGALDevice& device, const xiiSharedPtr<xiiGALTexture>& texture, xiiDynamicArray<T>& pixels)
  {
    if (!XII_TEST_BOOL(texture != nullptr))
      return false;
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    commands->Begin();
    const auto                       previous = texture->GetResourceState();
    const auto                       enqueued = readback.Enqueue(commands, {.m_pTexture = texture.Borrow()});
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = texture.Borrow();
    transition.m_NewState        = previous;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
      return false;
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return false;
    XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
    if (!XII_TEST_INT(capture.m_uiBytesPerPixel, sizeof(T)))
      return false;
    pixels.SetCount(capture.m_uiWidth * capture.m_uiHeight);
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    const auto                     result = commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped);
    if (result.Succeeded() && mapped.m_pData)
    {
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
        xiiMemoryUtils::Copy(pixels.GetData() + y * capture.m_uiWidth, xiiMemoryUtils::AddByteOffset(static_cast<const T*>(mapped.m_pData), y * mapped.m_uiStride), capture.m_uiWidth);
      commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}).AssertSuccess();
    }
    commands->End();
    return XII_TEST_RESULT(result) && XII_TEST_BOOL(mapped.m_pData != nullptr);
  }

  template <typename T>
  bool ReadTexturePixel(xiiGALDevice& device, xiiGALTexture* texture, xiiUInt32 x, xiiUInt32 y, T& value)
  {
    if (!XII_TEST_BOOL(texture != nullptr))
      return false;
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!XII_TEST_BOOL(commands != nullptr))
      return false;
    commands->Begin();
    const auto                       previous = texture->GetResourceState();
    const auto                       result   = readback.Enqueue(commands, {.m_pTexture = texture});
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = texture;
    transition.m_NewState        = previous;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    commands->End();
    if (!XII_TEST_RESULT(result))
      return false;
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return false;
    XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
    if (!XII_TEST_BOOL(capture.m_uiBytesPerPixel == sizeof(T) && x < capture.m_uiWidth && y < capture.m_uiHeight))
      return false;
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    const auto                     mappedResult = commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped);
    if (mappedResult.Succeeded() && mapped.m_pData)
      xiiMemoryUtils::Copy(&value, xiiMemoryUtils::AddByteOffset(static_cast<const T*>(mapped.m_pData), y * mapped.m_uiStride + x * sizeof(T)), 1);
    if (mappedResult.Succeeded())
      commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}).AssertSuccess();
    commands->End();
    return XII_TEST_RESULT(mappedResult) && XII_TEST_BOOL(mapped.m_pData != nullptr);
  }
} // namespace xiiGpuTestReadback
