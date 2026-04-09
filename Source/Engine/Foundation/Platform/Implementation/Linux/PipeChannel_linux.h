#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#if XII_ENABLED(XII_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/IpcChannel.h>

#  include <sys/stat.h>
#  include <sys/types.h>


class XII_FOUNDATION_DLL xiiPipeChannel_linux : public xiiIpcChannel
{
public:
  xiiPipeChannel_linux(xiiStringView sAddress, Mode::Enum mode);
  ~xiiPipeChannel_linux();

private:
  friend class xiiMessageLoop;
  friend class xiiMessageLoop_linux;

  // All functions from here on down are run from worker thread only
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;

  // These are called from MessageLoop_linux on OS events
  void AcceptIncomingConnection();
  void ProcessIncomingPackages();
  void ProcessConnectSuccessfull();

private:
  xiiString m_ServerSocketPath;
  xiiString m_ClientSocketPath;
  xiiInt32  m_ServerSocketFd = -1;
  xiiInt32  m_ClientSocketFd = -1;

  xiiUInt8 m_InputBuffer[4096];

  xiiUInt64 m_uiPreviousSendOffset = 0;
};
#endif
