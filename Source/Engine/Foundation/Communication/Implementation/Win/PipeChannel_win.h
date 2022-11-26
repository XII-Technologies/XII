#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Communication/IpcChannel.h>

struct IOContext
{
  OVERLAPPED     Overlapped; ///< Must be first field in class so we can do a reinterpret cast from *Overlapped to *IOContext.
  xiiIpcChannel* pChannel;   ///< Owner of this IOContext.
};

class XII_FOUNDATION_DLL xiiPipeChannel_win : public xiiIpcChannel
{
public:
  xiiPipeChannel_win(const char* szAddress, Mode::Enum mode);
  ~xiiPipeChannel_win();

private:
  friend class xiiMessageLoop;
  friend class xiiMessageLoop_win;

  bool CreatePipe(const char* szAddress);

  virtual void AddToMessageLoop(xiiMessageLoop* pMsgLoop) override;

  // All functions from here on down are run from worker thread only
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;

  bool ProcessConnection();
  bool ProcessIncomingMessages(DWORD uiBytesRead);
  bool ProcessOutgoingMessages(DWORD uiBytesWritten);


protected:
  void OnIOCompleted(IOContext* pContext, DWORD uiBytesTransfered, DWORD uiError);

private:
  struct State
  {
    explicit State(xiiPipeChannel_win* pChannel);
    ~State();
    IOContext          Context;
    xiiAtomicInteger32 IsPending = false; ///< Whether an async operation is in process.
  };

  enum Constants
  {
    BUFFER_SIZE = 4096,
  };

  // Shared data
  State m_InputState;
  State m_OutputState;

  // Setup in ctor
  HANDLE m_hPipeHandle = INVALID_HANDLE_VALUE;

  // Only accessed from worker thread
  xiiUInt8 m_InputBuffer[BUFFER_SIZE];
};

#endif
