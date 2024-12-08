#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

struct ID3D12CommandQueue;

class XII_GRAPHICSD3D12_DLL xiiGALCommandQueueD3D12 final : public xiiGALCommandQueue
{
public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetNextFenceValue() const override final { return 0ULL; }

  /// \brief This returns the last completed value of the internal fence.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetCompletedFenceValue() override final { return 0ULL; }

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

  virtual xiiGALCommandList* BeginCommandList() override final;

  void UnbindTextureFromFramebuffer(xiiGALTextureD3D12* pTextureD3D12);

  XII_ALWAYS_INLINE ID3D12CommandQueue* GetD3D12CommandQueue() const { return m_pCommandQueueD3D12; }

protected:
  xiiUInt64 Submit(xiiGALCommandList* pCommandList, bool bReset);

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueueD3D12();

  void InitializePlatform();

  void DeInitializePlatform();

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  ID3D12CommandQueue* m_pCommandQueueD3D12;
};
