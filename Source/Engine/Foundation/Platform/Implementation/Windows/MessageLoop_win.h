#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>

class xiiIpcChannel;
struct IOContext;

class XII_FOUNDATION_DLL xiiMessageLoop_win : public xiiMessageLoop
{
public:
  struct IOItem
  {
    XII_DECLARE_POD_TYPE();

    xiiIpcChannel* pChannel = nullptr;
    IOContext*     pContext = nullptr;
    DWORD          uiBytesTransfered;
    DWORD          uiError;
  };

public:
  xiiMessageLoop_win();
  ~xiiMessageLoop_win();

  HANDLE GetPort() const { return m_hPort; }

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(xiiInt32 iTimeout, xiiIpcChannel* pFilter) override;

  bool GetIOItem(xiiInt32 iTimeout, IOItem* pItem);
  bool ProcessInternalIOItem(const IOItem& item);
  bool MatchCompletedIOItem(xiiIpcChannel* pFilter, IOItem* pItem);

private:
  xiiDynamicArray<IOItem> m_CompletedIO;
  LONG                    m_iHaveWork = 0;
  HANDLE                  m_hPort     = INVALID_HANDLE_VALUE;
};

#endif
