
#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#if XII_ENABLED(XII_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Threading/Mutex.h>

#  include <poll.h>

class xiiIpcChannel;
class xiiPipeChannel_linux;

XII_DEFINE_AS_POD_TYPE(struct pollfd);

class XII_FOUNDATION_DLL xiiMessageLoop_linux : public xiiMessageLoop
{
public:
  xiiMessageLoop_linux();
  ~xiiMessageLoop_linux();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(xiiInt32 iTimeout, xiiIpcChannel* pFilter) override;

private:
  friend class xiiPipeChannel_linux;

  enum class WaitType
  {
    Accept,
    IncomingMessage,
    Connect,
    Send
  };

  void RegisterWait(xiiPipeChannel_linux* pChannel, WaitType type, xiiInt32 fd);
  void RemovePendingWaits(xiiPipeChannel_linux* pChannel);

private:
  struct WaitInfo
  {
    XII_DECLARE_POD_TYPE();

    xiiPipeChannel_linux* m_pChannel;
    WaitType              m_type;
  };

  // m_waitInfos and m_pollInfos are alway the same size.
  // related information is stored at the same index.
  xiiHybridArray<WaitInfo, 16>      m_waitInfos;
  xiiHybridArray<struct pollfd, 16> m_pollInfos;
  xiiMutex                          m_pollMutex;
  xiiAtomicInteger32                m_numPendingPollModifications = 0;
  xiiInt32                          m_wakeupPipeReadEndFd         = -1;
  xiiInt32                          m_wakeupPipeWriteEndFd        = -1;
};

#endif
