/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/NetworkIOOperation.h>

#include <Foundation/Network/Implementation/NetworkIOInternal.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <utility>
#include <variant>

namespace xiiNetworkIOInternal
{
  void OperationQueue::Push(Operation& operation)
  {
    operation.m_pPrevious = m_pTail;
    operation.m_pNext     = nullptr;
    if (m_pTail != nullptr)
    {
      m_pTail->m_pNext = &operation;
    }
    else
    {
      m_pHead = &operation;
    }
    m_pTail = &operation;
  }

  void OperationQueue::Remove(Operation& operation)
  {
    if (operation.m_pPrevious != nullptr)
    {
      operation.m_pPrevious->m_pNext = operation.m_pNext;
    }
    else
    {
      m_pHead = operation.m_pNext;
    }
    if (operation.m_pNext != nullptr)
    {
      operation.m_pNext->m_pPrevious = operation.m_pPrevious;
    }
    else
    {
      m_pTail = operation.m_pPrevious;
    }
    operation.m_pPrevious = nullptr;
    operation.m_pNext     = nullptr;
  }

  xiiNetworkSocket& Registration::GetSocket()
  {
    return const_cast<xiiNetworkSocket&>(std::as_const(*this).GetSocket());
  }

  const xiiNetworkSocket& Registration::GetSocket() const
  {
    if (const auto* pSocket = std::get_if<xiiTcpSocket>(&m_Socket))
    {
      return *pSocket;
    }
    if (const auto* pSocket = std::get_if<xiiTcpListener>(&m_Socket))
    {
      return *pSocket;
    }
    return std::get<xiiUdpSocket>(m_Socket);
  }

  xiiUInt32 BuildSendSpans(const Operation& operation, xiiArrayPtr<const xiiUInt8>* out_pSpans, xiiUInt32 uiLimit)
  {
    xiiUInt32 uiSkip  = operation.m_uiProgress;
    xiiUInt32 uiCount = 0;
    for (const auto& buffer : operation.m_Result.m_Buffers)
    {
      if (uiSkip >= buffer.GetCount())
      {
        uiSkip -= buffer.GetCount();
        continue;
      }
      const xiiUInt32 uiBytes = std::min(buffer.GetCount() - uiSkip, uiLimit);
      if (uiBytes > 0)
      {
        out_pSpans[uiCount++] = buffer.GetBytes().GetSubArray(uiSkip, uiBytes);
        uiLimit -= uiBytes;
      }
      uiSkip = 0;
      if (uiLimit == 0)
      {
        break;
      }
    }
    return uiCount;
  }

  xiiUInt32 BuildReceiveSpans(Operation& operation, xiiArrayPtr<xiiUInt8>* out_pSpans, xiiUInt32 uiLimit)
  {
    xiiUInt32 uiSkip  = operation.m_uiProgress;
    xiiUInt32 uiCount = 0;
    for (auto& buffer : operation.m_Result.m_Buffers)
    {
      if (uiSkip >= buffer.GetCount())
      {
        uiSkip -= buffer.GetCount();
        continue;
      }
      const xiiUInt32 uiBytes = std::min(buffer.GetCount() - uiSkip, uiLimit);
      if (uiBytes > 0)
      {
        out_pSpans[uiCount++] = buffer.GetBytes().GetSubArray(uiSkip, uiBytes);
        uiLimit -= uiBytes;
      }
      uiSkip = 0;
      if (uiLimit == 0)
      {
        break;
      }
    }
    return uiCount;
  }

  void Worker::FixDeadline(xiiUInt32 uiIndex)
  {
    while (uiIndex > 0)
    {
      const xiiUInt32 uiParent = (uiIndex - 1) / 2;
      if (m_DeadlineHeap[uiParent]->m_Deadline <= m_DeadlineHeap[uiIndex]->m_Deadline)
      {
        break;
      }
      std::swap(m_DeadlineHeap[uiIndex], m_DeadlineHeap[uiParent]);
      m_DeadlineHeap[uiIndex]->m_uiDeadlineIndex  = uiIndex;
      m_DeadlineHeap[uiParent]->m_uiDeadlineIndex = uiParent;
      uiIndex                                     = uiParent;
    }
    while (uiIndex < m_DeadlineHeap.GetCount() / 2)
    {
      xiiUInt32 uiChild = (uiIndex * 2) + 1;
      if (uiChild + 1 < m_DeadlineHeap.GetCount() && m_DeadlineHeap[uiChild + 1]->m_Deadline < m_DeadlineHeap[uiChild]->m_Deadline)
      {
        ++uiChild;
      }
      if (m_DeadlineHeap[uiIndex]->m_Deadline <= m_DeadlineHeap[uiChild]->m_Deadline)
      {
        break;
      }
      std::swap(m_DeadlineHeap[uiIndex], m_DeadlineHeap[uiChild]);
      m_DeadlineHeap[uiIndex]->m_uiDeadlineIndex = uiIndex;
      m_DeadlineHeap[uiChild]->m_uiDeadlineIndex = uiChild;
      uiIndex                                    = uiChild;
    }
  }

  void Worker::AddDeadline(Operation& operation)
  {
    if (operation.m_Deadline.IsNegative())
    {
      return;
    }
    operation.m_uiDeadlineIndex = m_DeadlineHeap.GetCount();
    m_DeadlineHeap.PushBack(&operation);
    FixDeadline(operation.m_uiDeadlineIndex);
  }

  void Worker::RemoveDeadline(Operation& operation)
  {
    if (operation.m_uiDeadlineIndex == xiiInvalidIndex)
    {
      return;
    }
    const xiiUInt32 uiIndex = operation.m_uiDeadlineIndex;
    m_DeadlineHeap.RemoveAtAndSwap(uiIndex);
    operation.m_uiDeadlineIndex = xiiInvalidIndex;
    if (uiIndex < m_DeadlineHeap.GetCount())
    {
      m_DeadlineHeap[uiIndex]->m_uiDeadlineIndex = uiIndex;
      FixDeadline(uiIndex);
    }
  }

  void Worker::Expire()
  {
    const xiiTime now = xiiTime::Now();
    while (!m_DeadlineHeap.IsEmpty() && m_DeadlineHeap[0]->m_Deadline <= now)
    {
      Operation& operation = *m_DeadlineHeap[0];
      RemoveDeadline(operation);
      Cancel(operation, xiiNetworkError(xiiNetworkErrorCode::TimedOut));
    }
  }

  xiiInt32 Worker::GetWaitMilliseconds() const
  {
    if (m_pReadyHead != nullptr)
    {
      return 0;
    }
    if (m_DeadlineHeap.IsEmpty())
    {
      return -1;
    }
    const double fMilliseconds = std::ceil((m_DeadlineHeap[0]->m_Deadline - xiiTime::Now()).GetMilliseconds());
    if (fMilliseconds <= 0)
    {
      return 0;
    }
    return fMilliseconds >= INT_MAX ? INT_MAX : static_cast<xiiInt32>(fMilliseconds);
  }
} // namespace xiiNetworkIOInternal
