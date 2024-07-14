#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

struct ID3D12CommandQueue;

class XII_GRAPHICSD3D12_DLL xiiGALCommandQueueD3D12 final : public xiiGALCommandQueue
{
public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  virtual xiiUInt64 GetNextFenceValue() const override final;

  /// \brief This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() override final;

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

  virtual xiiGALCommandList* BeginCommandList() override final;

  void UnbindTextureFromFramebuffer(xiiGALTextureD3D12* pTextureD3D12);

  ID3D12CommandQueue* GetD3D12CommandQueue() const;

protected:
  xiiUInt64 Submit(xiiGALCommandList* pCommandList, bool bReset);

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueueD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  xiiUniquePtr<xiiGALCommandListD3D12> m_pDefaultCommandList;
};

#include <GraphicsD3D12/CommandEncoder/Implementation/CommandQueueD3D12_inl.h>
