/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Pools/FencePoolD3D12.h>

xiiGALFencePoolD3D12::xiiGALFencePoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiInitialSize) :
  m_pDeviceD3D12(pDeviceD3D12), m_Fences(pDeviceD3D12->GetAllocator()), m_QueuedFences(pDeviceD3D12->GetAllocator())
{
  XII_LOCK(m_PoolMutex);

  for (xiiUInt32 i = 0U; i < uiInitialSize; ++i)
  {
    ID3D12Fence* pFence = CreateD3D12Fence();
    if (pFence != nullptr)
    {
      m_QueuedFences.PushBack(pFence);
    }
  }
}

xiiGALFencePoolD3D12::~xiiGALFencePoolD3D12()
{
  XII_LOCK(m_PoolMutex);

  for (ID3D12Fence*& pFence : m_Fences)
  {
    if (pFence != nullptr)
    {
      IUnknown* pObject = pFence;
      m_pDeviceD3D12->SafeReleaseDeviceObject(pObject);
      pFence = nullptr;
    }
  }

  m_QueuedFences.Clear();
  m_Fences.Clear();
}

ID3D12Fence* xiiGALFencePoolD3D12::RequestFence()
{
  XII_LOCK(m_PoolMutex);

  if (m_QueuedFences.IsEmpty())
  {
    if (ID3D12Fence* pFence = CreateD3D12Fence(); pFence != nullptr)
    {
      m_QueuedFences.PushBack(pFence);
    }
  }

  if (m_QueuedFences.IsEmpty())
    return nullptr;

  ID3D12Fence* pFence = m_QueuedFences.PeekFront();
  m_QueuedFences.PopFront();
  return pFence;
}

void xiiGALFencePoolD3D12::ReclaimFence(ID3D12Fence*& pFence)
{
  if (pFence == nullptr)
    return;

  XII_LOCK(m_PoolMutex);

  m_QueuedFences.PushBack(pFence);
  pFence = nullptr;
}

ID3D12Fence* xiiGALFencePoolD3D12::CreateD3D12Fence()
{
  ID3D12Fence* pFence = nullptr;
  const HRESULT hResult = m_pDeviceD3D12->GetD3D12Device()->CreateFence(0U, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to create D3D12 fence for fence pool: {}.", xiiHRESULTtoString(hResult));
    return nullptr;
  }

  m_Fences.PushBack(pFence);
  return pFence;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Pools_Implementation_FencePoolD3D12);

