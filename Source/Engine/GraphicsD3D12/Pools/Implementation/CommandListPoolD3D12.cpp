/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Pools/CommandListPoolD3D12.h>

void xiiGALCommandListPoolD3D12::ThreadPool::Push(CommandListEntry&& commandListEntry)
{
  XII_LOCK(m_Mutex);

  if (commandListEntry.m_bIsSecondary)
  {
    m_SecondaryFreeCommandLists.PushBack(commandListEntry);
  }
  else
  {
    m_PrimaryFreeCommandLists.PushBack(commandListEntry);
  }
}

void xiiGALCommandListPoolD3D12::ThreadPool::PushInFlight(CommandListEntry&& commandListEntry, xiiGALCommandListDataD3D12&& commandListData, xiiUInt64 uiFenceValue)
{
  XII_LOCK(m_Mutex);

  InFlightCommandList& inFlightCommandList = m_InFlightCommandLists.ExpandAndGetRef();
  inFlightCommandList.m_CommandList         = commandListEntry;
  inFlightCommandList.m_CommandListData     = std::move(commandListData);
  inFlightCommandList.m_uiFenceValue        = uiFenceValue;
}

xiiGALCommandListPoolD3D12::xiiGALCommandListPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALCommandQueueD3D12* pCommandQueueD3D12, D3D12_COMMAND_LIST_TYPE commandListType, xiiUInt32 uiInitialCountPerThread /*= 0U*/) :
  m_pDeviceD3D12(pDeviceD3D12), m_pCommandQueueD3D12(pCommandQueueD3D12), m_CommandListType(commandListType), m_uiInitialReserveCount(uiInitialCountPerThread)
{
  XII_ASSERT_DEBUG(m_pDeviceD3D12 != nullptr, "Invalid D3D12 device.");
  XII_ASSERT_DEBUG(m_pCommandQueueD3D12 != nullptr, "Invalid D3D12 command queue.");
}

xiiGALCommandListPoolD3D12::~xiiGALCommandListPoolD3D12()
{
  XII_LOCK(m_PoolMutex);

  for (auto& it : m_CommandListPoolsPerThread)
  {
    ThreadPool& threadPool = it.Value();

    XII_LOCK(threadPool.m_Mutex);

    for (auto& commandList : threadPool.m_PrimaryFreeCommandLists)
    {
      ReleaseCommandListEntry(m_pDeviceD3D12, commandList);
    }
    for (auto& commandList : threadPool.m_SecondaryFreeCommandLists)
    {
      ReleaseCommandListEntry(m_pDeviceD3D12, commandList);
    }
    for (auto& inFlightCommandList : threadPool.m_InFlightCommandLists)
    {
      ReleaseCommandListEntry(m_pDeviceD3D12, inFlightCommandList.m_CommandList);
    }

    threadPool.m_PrimaryFreeCommandLists.Clear();
    threadPool.m_SecondaryFreeCommandLists.Clear();
    threadPool.m_InFlightCommandLists.Clear();
  }
}

xiiGALCommandListPoolD3D12::ThreadPool& xiiGALCommandListPoolD3D12::GetOrCreateThreadPool()
{
  const xiiThreadID uiThreadID = xiiThreadUtils::GetCurrentThreadID();

  XII_LOCK(m_PoolMutex);

  bool bExisted = false;
  auto it = m_CommandListPoolsPerThread.FindOrAdd(uiThreadID, &bExisted);

  if (!bExisted)
  {
    ThreadPool& threadPool = it.Value();

    if (m_uiInitialReserveCount > 0U)
    {
      XII_LOCK(threadPool.m_Mutex);

      for (xiiUInt32 i = 0U; i < m_uiInitialReserveCount; ++i)
      {
        threadPool.m_PrimaryFreeCommandLists.PushBack(CreateCommandListEntry(false));
        threadPool.m_SecondaryFreeCommandLists.PushBack(CreateCommandListEntry(true));
      }
    }
  }

  return it.Value();
}

xiiGALCommandListPoolD3D12::CommandListEntry xiiGALCommandListPoolD3D12::CreateCommandListEntry(bool bIsSecondary)
{
  CommandListEntry commandListEntry = {};
  commandListEntry.m_bIsSecondary   = bIsSecondary;

  const D3D12_COMMAND_LIST_TYPE commandListType = bIsSecondary ? D3D12_COMMAND_LIST_TYPE_BUNDLE : m_CommandListType;

  HRESULT hResult = m_pDeviceD3D12->GetD3D12Device()->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandListEntry.m_pCommandAllocator));
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to create D3D12 command allocator for command-list pool: {}.", xiiHRESULTtoString(hResult));
    return {};
  }

  hResult = m_pDeviceD3D12->GetD3D12Device()->CreateCommandList(0U, commandListType, commandListEntry.m_pCommandAllocator, nullptr, IID_PPV_ARGS(&commandListEntry.m_pCommandList));
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to create D3D12 command list for command-list pool: {}.", xiiHRESULTtoString(hResult));
    XII_GAL_D3D12_RELEASE(commandListEntry.m_pCommandAllocator);
    return {};
  }

  // Command lists must be closed before they can be reset for recording.
  hResult = commandListEntry.m_pCommandList->Close();
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to close newly-created D3D12 command list for command-list pool: {}.", xiiHRESULTtoString(hResult));
    XII_GAL_D3D12_RELEASE(commandListEntry.m_pCommandList);
    XII_GAL_D3D12_RELEASE(commandListEntry.m_pCommandAllocator);
    return {};
  }

  return commandListEntry;
}

void xiiGALCommandListPoolD3D12::ReleaseCommandListEntry(xiiGALDeviceD3D12* pDeviceD3D12, CommandListEntry& commandListEntry)
{
  if (commandListEntry.m_pCommandList != nullptr)
  {
    IUnknown* pObject = commandListEntry.m_pCommandList;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    commandListEntry.m_pCommandList = nullptr;
  }
  if (commandListEntry.m_pCommandAllocator != nullptr)
  {
    IUnknown* pObject = commandListEntry.m_pCommandAllocator;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    commandListEntry.m_pCommandAllocator = nullptr;
  }
}

xiiGALCommandListPoolD3D12::AutoCommandList xiiGALCommandListPoolD3D12::AllocatePrimaryCommandList()
{
  ThreadPool& threadPool = GetOrCreateThreadPool();

  {
    XII_LOCK(threadPool.m_Mutex);

    if (!threadPool.m_PrimaryFreeCommandLists.IsEmpty())
    {
      CommandListEntry commandListEntry = threadPool.m_PrimaryFreeCommandLists.PeekBack();
      threadPool.m_PrimaryFreeCommandLists.PopBack();
      return AutoCommandList{&threadPool, std::move(commandListEntry)};
    }
  }

  return AutoCommandList{&threadPool, CreateCommandListEntry(false)};
}

xiiGALCommandListPoolD3D12::AutoCommandList xiiGALCommandListPoolD3D12::AllocateSecondaryCommandList()
{
  ThreadPool& threadPool = GetOrCreateThreadPool();

  {
    XII_LOCK(threadPool.m_Mutex);

    if (!threadPool.m_SecondaryFreeCommandLists.IsEmpty())
    {
      CommandListEntry commandListEntry = threadPool.m_SecondaryFreeCommandLists.PeekBack();
      threadPool.m_SecondaryFreeCommandLists.PopBack();
      return AutoCommandList{&threadPool, std::move(commandListEntry)};
    }
  }

  return AutoCommandList{&threadPool, CreateCommandListEntry(true)};
}

void xiiGALCommandListPoolD3D12::RecycleAfterSubmit(AutoCommandList&& commandList, xiiGALCommandListDataD3D12&& commandListData, xiiUInt64 uiFenceValue)
{
  if (commandList.m_pOwner == nullptr || commandList.m_CommandListEntry.m_pCommandList == nullptr)
    return;

  ThreadPool* pOwner = commandList.m_pOwner;
  CommandListEntry commandListEntry = commandList.m_CommandListEntry;

  commandList.m_pOwner           = nullptr;
  commandList.m_CommandListEntry = {};

  pOwner->PushInFlight(std::move(commandListEntry), std::move(commandListData), uiFenceValue);
}

void xiiGALCommandListPoolD3D12::ReclaimCompleted()
{
  XII_LOCK(m_PoolMutex);

  const xiiUInt64 uiCompletedFenceValue = m_pCommandQueueD3D12->GetCompletedFenceValue();

  for (auto& it : m_CommandListPoolsPerThread)
  {
    ThreadPool& threadPool = it.Value();

    XII_LOCK(threadPool.m_Mutex);

    for (xiiUInt32 i = 0U; i < threadPool.m_InFlightCommandLists.GetCount();)
    {
      InFlightCommandList& inFlightCommandList = threadPool.m_InFlightCommandLists[i];

      if (inFlightCommandList.m_uiFenceValue <= uiCompletedFenceValue)
      {
        // Release transient per-command-list data once the GPU has finished execution.
        inFlightCommandList.m_CommandListData = {};

        if (inFlightCommandList.m_CommandList.m_bIsSecondary)
        {
          threadPool.m_SecondaryFreeCommandLists.PushBack(inFlightCommandList.m_CommandList);
        }
        else
        {
          threadPool.m_PrimaryFreeCommandLists.PushBack(inFlightCommandList.m_CommandList);
        }

        threadPool.m_InFlightCommandLists.RemoveAtAndSwap(i);
      }
      else
      {
        ++i;
      }
    }
  }
}

void xiiGALCommandListPoolD3D12::ResetPools()
{
  XII_LOCK(m_PoolMutex);

  for (auto& it : m_CommandListPoolsPerThread)
  {
    ThreadPool& threadPool = it.Value();
    XII_LOCK(threadPool.m_Mutex);

    for (auto& commandListEntry : threadPool.m_PrimaryFreeCommandLists)
    {
      if (commandListEntry.m_pCommandAllocator != nullptr)
      {
        const HRESULT hResult = commandListEntry.m_pCommandAllocator->Reset();
        if (FAILED(hResult))
        {
          xiiLog::Warning("Failed to reset D3D12 primary command allocator while resetting command-list pool: {}.", xiiHRESULTtoString(hResult));
        }
      }
    }
    for (auto& commandListEntry : threadPool.m_SecondaryFreeCommandLists)
    {
      if (commandListEntry.m_pCommandAllocator != nullptr)
      {
        const HRESULT hResult = commandListEntry.m_pCommandAllocator->Reset();
        if (FAILED(hResult))
        {
          xiiLog::Warning("Failed to reset D3D12 secondary command allocator while resetting command-list pool: {}.", xiiHRESULTtoString(hResult));
        }
      }
    }
  }
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Pools_Implementation_CommandListPoolD3D12);

