/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandQueueD3D12 final : public xiiGALCommandQueue
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueueD3D12, xiiGALCommandQueue);

public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetNextFenceValue() const override final { return m_uiNextFenceValue; }

  /// \brief This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() override final;

  XII_ALWAYS_INLINE const xiiGALQueueInformationD3D12& GetQueueInformation() const { return m_QueueInformation; };

  virtual xiiUInt64 SubmitPlatform(xiiGALCommandList* pCommandList) override final;

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListD3D12;

  xiiGALCommandQueueD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALCommandQueueCreationDescription& creationDescription, const xiiGALQueueInformationD3D12& queueInformation);

  virtual ~xiiGALCommandQueueD3D12();

private:
  xiiGALQueueInformationD3D12 m_QueueInformation;

  xiiAtomicIntegerU64 m_uiNextFenceValue{1ULL};
  xiiUInt64           m_uiLastSyncPointValue{0ULL};
};
