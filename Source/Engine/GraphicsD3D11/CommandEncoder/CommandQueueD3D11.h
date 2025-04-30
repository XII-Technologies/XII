#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

#include <atomic>

struct ID3D11DeviceContext1;

class XII_GRAPHICSD3D11_DLL xiiGALCommandQueueD3D11 final : public xiiGALCommandQueue
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueueD3D11, xiiGALCommandQueue);

public:
  XII_ALWAYS_INLINE virtual xiiUInt64 GetNextFenceValue() const override final { return m_NextFenceValue.load(); };

  virtual xiiUInt64 GetCompletedFenceValue() override final;

  virtual xiiUInt64 WaitForIdle() override final;

  virtual xiiGALCommandList* BeginCommandList() override final;

protected:
  xiiUInt64 SubmitCommandList(xiiGALCommandList* pCommandList);

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListD3D11;

  xiiGALCommandQueueD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueueD3D11();

  void InitializePlatform();

  void DeInitializePlatform();

private:
  ID3D11DeviceContext4*                m_pImmediateContext = nullptr;
  xiiUniquePtr<xiiGALCommandListD3D11> m_pCommandListD3D11;

  // A value that will be signaled by the command queue next.
  std::atomic<xiiUInt64> m_NextFenceValue{1};

  // Last fence value completed by the GPU
  std::atomic<xiiUInt64> m_LastCompletedFenceValue{0};

  // The fence is signaled right after the command list has been submitted to the command queue for execution.
  // All command lists with fence value less or equal to the signaled value are guaranteed to be finished by the GPU.
  ID3D11Fence* m_pD3D11DFence          = nullptr;
  HANDLE       m_WaitForGPUEventHandle = {};
};
