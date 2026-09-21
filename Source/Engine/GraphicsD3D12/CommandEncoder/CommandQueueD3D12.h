/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

struct ID3D12Fence;

class XII_GRAPHICSD3D12_DLL xiiGALCommandQueueD3D12 final : public xiiGALCommandQueue
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueueD3D12, xiiGALCommandQueue);

public:
  /// This returns the value of the internal fence that will be signaled the next time.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetNextFenceValue() const override final { return m_uiNextFenceValue; }

  /// This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() override final;

  XII_ALWAYS_INLINE ID3D12CommandQueue* GetD3D12CommandQueue() const { return m_QueueInformation.m_pCommandQueue; }

  XII_ALWAYS_INLINE const xiiGALQueueInformationD3D12& GetQueueInformation() const { return m_QueueInformation; };

  virtual xiiUInt64 SubmitPlatform(xiiGALCommandList* pCommandList) override final;

  /// This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListD3D12;

  xiiGALCommandQueueD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALCommandQueueCreationDescription& creationDescription, const xiiGALQueueInformationD3D12& queueInformation);

  virtual ~xiiGALCommandQueueD3D12();

private:
  xiiGALQueueInformationD3D12 m_QueueInformation;

  ID3D12Fence* m_pD3D12QueueFence = nullptr;
  HANDLE       m_hFenceEvent      = nullptr;

  xiiAtomicIntegerU64 m_uiNextFenceValue{1ULL};
  xiiUInt64           m_uiLastSyncPointValue{0ULL};
};
