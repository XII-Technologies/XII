/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Communication/IpcChannel.h>
#  include <Foundation/Platform/Implementation/Windows/MessageLoop_win.h>
#  include <Foundation/Platform/Implementation/Windows/PipeChannel_win.h>

xiiMessageLoop_win::xiiMessageLoop_win()
{
  m_hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  XII_ASSERT_DEBUG(m_hPort != INVALID_HANDLE_VALUE, "Failed to create IO completion port!");
}

xiiMessageLoop_win::~xiiMessageLoop_win()
{
  StopUpdateThread();
  CloseHandle(m_hPort);
}


bool xiiMessageLoop_win::WaitForMessages(xiiInt32 iTimeout, xiiIpcChannel* pFilter)
{
  if (iTimeout < 0)
    iTimeout = INFINITE;

  IOItem item;
  if (!MatchCompletedIOItem(pFilter, &item))
  {
    if (!GetIOItem(iTimeout, &item))
      return false;

    if (ProcessInternalIOItem(item))
      return true;
  }

  if (item.pContext->pChannel != NULL)
  {
    if (pFilter != NULL && item.pChannel != pFilter)
    {
      m_CompletedIO.PushBack(item);
    }
    else
    {
      XII_ASSERT_DEBUG(item.pContext->pChannel == item.pChannel, "");
      static_cast<xiiPipeChannel_win*>(item.pChannel)->OnIOCompleted(item.pContext, item.uiBytesTransfered, item.uiError);
    }
  }
  return true;
}

bool xiiMessageLoop_win::GetIOItem(xiiInt32 iTimeout, IOItem* pItem)
{
  memset(pItem, 0, sizeof(*pItem));
  ULONG_PTR   key        = 0;
  OVERLAPPED* overlapped = NULL;
  if (!GetQueuedCompletionStatus(m_hPort, &pItem->uiBytesTransfered, &key, &overlapped, iTimeout))
  {
    // nothing queued
    if (overlapped == NULL)
      return false;

    pItem->uiError           = GetLastError();
    pItem->uiBytesTransfered = 0;
  }

  pItem->pChannel = reinterpret_cast<xiiIpcChannel*>(key);
  pItem->pContext = reinterpret_cast<IOContext*>(overlapped);
  return true;
}

bool xiiMessageLoop_win::ProcessInternalIOItem(const IOItem& item)
{
  if (reinterpret_cast<xiiMessageLoop_win*>(item.pContext) == this && reinterpret_cast<xiiMessageLoop_win*>(item.pChannel) == this)
  {
    // internal notification
    XII_ASSERT_DEBUG(item.uiBytesTransfered == 0, "");
    InterlockedExchange(&m_iHaveWork, 0);
    return true;
  }
  return false;
}

bool xiiMessageLoop_win::MatchCompletedIOItem(xiiIpcChannel* pFilter, IOItem* pItem)
{
  for (xiiUInt32 i = 0; i < m_CompletedIO.GetCount(); i++)
  {
    if (pFilter == NULL || m_CompletedIO[i].pChannel == pFilter)
    {
      *pItem = m_CompletedIO[i];
      m_CompletedIO.RemoveAtAndCopy(i);
      return true;
    }
  }
  return false;
}

void xiiMessageLoop_win::WakeUp()
{
  if (InterlockedExchange(&m_iHaveWork, 1))
  {
    // already running
    return;
  }
  // wake up the loop
  BOOL bSucceeded = PostQueuedCompletionStatus(m_hPort, 0, reinterpret_cast<ULONG_PTR>(this), reinterpret_cast<OVERLAPPED*>(this));
  XII_ASSERT_DEBUG(bSucceeded, "Could not PostQueuedCompletionStatus: {0}", xiiArgErrorCode(GetLastError()));
  XII_IGNORE_UNUSED(bSucceeded);
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Win_MessageLoop_win);
