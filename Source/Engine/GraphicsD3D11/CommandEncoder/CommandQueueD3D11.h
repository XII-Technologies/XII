#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSD3D11_DLL xiiGALCommandQueueD3D11 final : public xiiGALCommandQueue
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueueD3D11, xiiGALCommandQueue);

public:
  virtual xiiUInt64 GetNextFenceValue() const override final;

  virtual xiiUInt64 GetCompletedFenceValue() const override final;

  virtual xiiUInt64 WaitForIdle() override final;

  virtual xiiGALCommandList* BeginCommandList() override final;

  void AddSwapChainCommandListReference(xiiGALCommandListD3D11* pCommandListD3D11);
  void RemoveSwapChainCommandListReference(xiiGALCommandListD3D11* pCommandListD3D11);
  void ReleaseSwapChainCommanListReferences();

protected:
  virtual void SubmitPlatform(xiiGALCommandList* pCommandList, bool bReset) override final;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueueD3D11();

protected:
  xiiUInt64 m_uiCompletedFenceValue = 0U;

  xiiDeque<xiiGALCommandList*>             m_CommandLists;
  xiiDynamicArray<xiiGALCommandListD3D11*> m_SwapChainCommandListReferences;
};

#include <GraphicsD3D11/CommandEncoder/Implementation/CommandQueueD3D11_inl.h>
