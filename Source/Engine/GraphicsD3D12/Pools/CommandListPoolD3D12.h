/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsD3D12/CommandEncoder/CommandListDataD3D12.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandListPoolD3D12
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALCommandListPoolD3D12);

public:
  struct CommandListEntry
  {
    XII_DECLARE_POD_TYPE();

    ID3D12CommandAllocator*    m_pCommandAllocator = nullptr;
    ID3D12GraphicsCommandList* m_pCommandList      = nullptr;
    bool                       m_bIsSecondary      = false;
  };

  struct InFlightCommandList
  {
    CommandListEntry           m_CommandList;
    xiiUInt64                  m_uiFenceValue = 0ULL;
    xiiGALCommandListDataD3D12 m_CommandListData;
  };

  struct ThreadPool
  {
    XII_ALWAYS_INLINE ThreadPool() = default;

    XII_ALWAYS_INLINE ThreadPool(ThreadPool&& other) noexcept :
      m_PrimaryFreeCommandLists(std::move(other.m_PrimaryFreeCommandLists)),
      m_SecondaryFreeCommandLists(std::move(other.m_SecondaryFreeCommandLists)),
      m_InFlightCommandLists(std::move(other.m_InFlightCommandLists))
    {
    }

    ThreadPool& operator=(ThreadPool&& other) noexcept
    {
      if (this != &other)
      {
        XII_LOCK(m_Mutex);
        XII_LOCK(other.m_Mutex);

        m_PrimaryFreeCommandLists   = std::move(other.m_PrimaryFreeCommandLists);
        m_SecondaryFreeCommandLists = std::move(other.m_SecondaryFreeCommandLists);
        m_InFlightCommandLists      = std::move(other.m_InFlightCommandLists);
      }
      return *this;
    }

    void Push(CommandListEntry&& commandListEntry);
    void PushInFlight(CommandListEntry&& commandListEntry, xiiGALCommandListDataD3D12&& commandListData, xiiUInt64 uiFenceValue);

    xiiDynamicArray<CommandListEntry>   m_PrimaryFreeCommandLists;
    xiiDynamicArray<CommandListEntry>   m_SecondaryFreeCommandLists;
    xiiDynamicArray<InFlightCommandList> m_InFlightCommandLists;
    xiiMutex                             m_Mutex;
  };

  struct AutoCommandList
  {
    XII_DISALLOW_COPY_AND_ASSIGN(AutoCommandList);

    XII_ALWAYS_INLINE AutoCommandList() = default;

    XII_ALWAYS_INLINE AutoCommandList(ThreadPool* pThreadPool, CommandListEntry&& commandListEntry) noexcept :
      m_pOwner(pThreadPool), m_CommandListEntry(std::move(commandListEntry))
    {
    }

    XII_ALWAYS_INLINE AutoCommandList(AutoCommandList&& rhs) noexcept :
      m_pOwner(rhs.m_pOwner), m_CommandListEntry(rhs.m_CommandListEntry)
    {
      rhs.m_pOwner = nullptr;
      rhs.m_CommandListEntry = {};
    }

    XII_ALWAYS_INLINE AutoCommandList& operator=(AutoCommandList&& rhs) noexcept
    {
      if (this == &rhs)
        return *this;

      if (m_pOwner != nullptr && m_CommandListEntry.m_pCommandList != nullptr)
      {
        m_pOwner->Push(std::move(m_CommandListEntry));
      }

      m_pOwner            = rhs.m_pOwner;
      m_CommandListEntry  = rhs.m_CommandListEntry;

      rhs.m_pOwner           = nullptr;
      rhs.m_CommandListEntry = {};

      return *this;
    }

    XII_ALWAYS_INLINE ~AutoCommandList()
    {
      if (m_pOwner != nullptr && m_CommandListEntry.m_pCommandList != nullptr)
      {
        m_pOwner->Push(std::move(m_CommandListEntry));
      }
    }

    XII_ALWAYS_INLINE ID3D12CommandAllocator* GetCommandAllocator() const { return m_CommandListEntry.m_pCommandAllocator; }
    XII_ALWAYS_INLINE ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandListEntry.m_pCommandList; }
    XII_ALWAYS_INLINE bool IsSecondary() const { return m_CommandListEntry.m_bIsSecondary; }

  private:
    friend class xiiGALCommandListPoolD3D12;

    ThreadPool*       m_pOwner          = nullptr;
    CommandListEntry  m_CommandListEntry = {};
  };

  AutoCommandList AllocatePrimaryCommandList();
  AutoCommandList AllocateSecondaryCommandList();

  void RecycleAfterSubmit(AutoCommandList&& commandList, xiiGALCommandListDataD3D12&& commandListData, xiiUInt64 uiFenceValue);

  void ReclaimCompleted();
  void ResetPools();

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D12;
  friend class xiiGALCommandQueueD3D12;

  xiiGALCommandListPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALCommandQueueD3D12* pCommandQueueD3D12, D3D12_COMMAND_LIST_TYPE commandListType, xiiUInt32 uiInitialCountPerThread = 0U);
  ~xiiGALCommandListPoolD3D12();

  ThreadPool&        GetOrCreateThreadPool();
  CommandListEntry   CreateCommandListEntry(bool bIsSecondary);

  static void        ReleaseCommandListEntry(xiiGALDeviceD3D12* pDeviceD3D12, CommandListEntry& commandListEntry);

private:
  xiiGALDeviceD3D12*       m_pDeviceD3D12       = nullptr;
  xiiGALCommandQueueD3D12* m_pCommandQueueD3D12 = nullptr;
  D3D12_COMMAND_LIST_TYPE  m_CommandListType    = D3D12_COMMAND_LIST_TYPE_DIRECT;
  xiiUInt32                m_uiInitialReserveCount = 0U;

  xiiMutex                                   m_PoolMutex;
  xiiMap<xiiThreadID, ThreadPool>            m_CommandListPoolsPerThread;
};

