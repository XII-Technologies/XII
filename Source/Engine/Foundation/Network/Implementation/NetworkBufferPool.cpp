/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/NetworkBufferPool.h>

#include <Foundation/Memory/Allocator.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>

#include <cstring>
#include <utility>

class xiiNetworkBufferPoolState : public xiiRefCountingImpl
{
public:
  xiiNetworkBufferPoolState(const xiiNetworkBufferPoolConfiguration& configuration, xiiAllocator* pAllocator) :
    m_Configuration(configuration), m_pAllocator(pAllocator) {}

  ~xiiNetworkBufferPoolState() { Trim(); }

  void Trim()
  {
    XII_LOCK(m_Mutex);
    for (xiiUInt32 uiBucket = 0; uiBucket < 8; ++uiBucket)
    {
      while (m_pFree[uiBucket] != nullptr)
      {
        void* pBlock = m_pFree[uiBucket];
        std::memcpy(&m_pFree[uiBucket], pBlock, sizeof(void*));
        m_pAllocator->Deallocate(pBlock);
        m_Statistics.m_uiCommittedBytes -= m_Configuration.m_uiSizeClasses[uiBucket];
      }
    }
  }

  void Release(xiiUInt8* pBlock, xiiUInt32 uiCapacity, xiiUInt32 uiBucket)
  {
    XII_LOCK(m_Mutex);
    m_Statistics.m_uiLeasedBytes -= uiCapacity;
    --m_Statistics.m_uiActiveBuffers;
    if (uiBucket < 8)
    {
      std::memcpy(pBlock, &m_pFree[uiBucket], sizeof(void*));
      m_pFree[uiBucket] = pBlock;
    }
    else
    {
      m_pAllocator->Deallocate(pBlock);
      m_Statistics.m_uiCommittedBytes -= uiCapacity;
    }
  }

  xiiNetworkBufferPoolConfiguration m_Configuration;
  xiiAllocator*                     m_pAllocator;
  mutable xiiMutex                  m_Mutex;
  void*                             m_pFree[8] = {};
  xiiNetworkBufferPoolStatistics    m_Statistics;
};

xiiNetworkIOBuffer::xiiNetworkIOBuffer() = default;
xiiNetworkIOBuffer::~xiiNetworkIOBuffer() { Clear(); }

xiiNetworkIOBuffer::xiiNetworkIOBuffer(xiiNetworkIOBuffer&& other) noexcept
{
  *this = std::move(other);
}

xiiNetworkIOBuffer& xiiNetworkIOBuffer::operator=(xiiNetworkIOBuffer&& other) noexcept
{
  if (this != &other)
  {
    Clear();
    m_pState     = std::move(other.m_pState);
    m_pData      = std::exchange(other.m_pData, nullptr);
    m_uiCapacity = std::exchange(other.m_uiCapacity, 0);
    m_uiCount    = std::exchange(other.m_uiCount, 0);
    m_uiBucket   = std::exchange(other.m_uiBucket, 0);
  }
  return *this;
}

void xiiNetworkIOBuffer::Clear()
{
  if (m_pData != nullptr)
  {
    auto pState = std::move(m_pState);
    pState->Release(m_pData, m_uiCapacity, m_uiBucket);
    m_pData      = nullptr;
    m_uiCapacity = 0;
    m_uiCount    = 0;
    m_uiBucket   = 0;
  }
}

xiiNetworkError xiiNetworkIOBuffer::SetCount(xiiUInt32 uiCount)
{
  if (!IsValid() || uiCount > m_uiCapacity)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  m_uiCount = uiCount;
  return {};
}

xiiNetworkBufferPool::xiiNetworkBufferPool()  = default;
xiiNetworkBufferPool::~xiiNetworkBufferPool() = default;

xiiNetworkError xiiNetworkBufferPool::Initialize(const xiiNetworkBufferPoolConfiguration& configuration, xiiAllocator* pAllocator)
{
  if (m_pState)
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  bool      bEnded     = false;
  xiiUInt32 uiPrevious = 0;
  for (xiiUInt32 uiSize : configuration.m_uiSizeClasses)
  {
    if (uiSize == 0)
    {
      bEnded = true;
      continue;
    }
    if (bEnded || uiSize < sizeof(void*) || uiSize <= uiPrevious)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    uiPrevious = uiSize;
  }
  if (configuration.m_uiSizeClasses[0] == 0 || configuration.m_uiMemoryBudget < configuration.m_uiSizeClasses[0])
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  if (pAllocator == nullptr)
  {
    pAllocator = xiiFoundation::GetDefaultAllocator();
  }
  m_pState = XII_NEW(pAllocator, xiiNetworkBufferPoolState, configuration, pAllocator);
  return {};
}

xiiNetworkError xiiNetworkBufferPool::Acquire(xiiUInt32 uiCount, xiiNetworkIOBuffer& out_buffer)
{
  if (!m_pState)
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotRunning);
  }
  xiiNetworkIOBuffer buffer;
  {
    XII_LOCK(m_pState->m_Mutex);
    xiiUInt32 uiBucket   = 8;
    xiiUInt32 uiCapacity = uiCount == 0 ? 1 : uiCount;
    for (xiiUInt32 uiIndex = 0; uiIndex < 8; ++uiIndex)
    {
      if (m_pState->m_Configuration.m_uiSizeClasses[uiIndex] >= uiCapacity)
      {
        uiBucket   = uiIndex;
        uiCapacity = m_pState->m_Configuration.m_uiSizeClasses[uiIndex];
        break;
      }
    }
    void* pBlock = uiBucket < 8 ? m_pState->m_pFree[uiBucket] : nullptr;
    if (pBlock != nullptr)
    {
      std::memcpy(&m_pState->m_pFree[uiBucket], pBlock, sizeof(void*));
      ++m_pState->m_Statistics.m_uiReuses;
    }
    else
    {
      const xiiUInt64 uiAvailable = m_pState->m_Configuration.m_uiMemoryBudget - m_pState->m_Statistics.m_uiCommittedBytes;
      if (uiCapacity > uiAvailable)
      {
        ++m_pState->m_Statistics.m_uiBackpressureEvents;
        return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
      }
      pBlock = m_pState->m_pAllocator->Allocate(uiCapacity, alignof(void*));
      if (pBlock == nullptr)
      {
        return xiiNetworkError(xiiNetworkErrorCode::ResourceExhausted);
      }
      m_pState->m_Statistics.m_uiCommittedBytes += uiCapacity;
      ++m_pState->m_Statistics.m_uiAllocations;
    }
    buffer.m_pState     = m_pState;
    buffer.m_pData      = static_cast<xiiUInt8*>(pBlock);
    buffer.m_uiCount    = uiCount;
    buffer.m_uiCapacity = uiCapacity;
    buffer.m_uiBucket   = uiBucket;
    m_pState->m_Statistics.m_uiLeasedBytes += uiCapacity;
    ++m_pState->m_Statistics.m_uiActiveBuffers;
  }
  out_buffer = std::move(buffer);
  return {};
}

void xiiNetworkBufferPool::Trim()
{
  if (m_pState)
  {
    m_pState->Trim();
  }
}

xiiNetworkBufferPoolStatistics xiiNetworkBufferPool::GetStatistics() const
{
  if (!m_pState)
  {
    return {};
  }
  XII_LOCK(m_pState->m_Mutex);
  return m_pState->m_Statistics;
}
