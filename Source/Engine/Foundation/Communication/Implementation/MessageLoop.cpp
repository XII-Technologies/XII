#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Configuration/Startup.h>

XII_IMPLEMENT_SINGLETON(xiiMessageLoop);

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Communication/Implementation/Win/MessageLoop_win.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Communication/Implementation/Linux/MessageLoop_linux.h>
#else
#  include <Foundation/Communication/Implementation/Mobile/MessageLoop_null.h>
#endif

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, MessageLoop)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "TaskSystem",
    "ThreadUtils"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    #if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
      XII_DEFAULT_NEW(xiiMessageLoop_win);
    #elif XII_ENABLED(XII_PLATFORM_LINUX)
      XII_DEFAULT_NEW(xiiMessageLoop_linux);
    #else
      XII_DEFAULT_NEW(xiiMessageLoop_null);
    #endif
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiMessageLoop* pDummy = xiiMessageLoop::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

class xiiLoopThread : public xiiThread
{
public:
  xiiLoopThread() :
    xiiThread("xiiMessageLoopThread")
  {
  }
  xiiMessageLoop*   m_pRemoteInterface = nullptr;
  virtual xiiUInt32 Run() override
  {
    m_pRemoteInterface->RunLoop();
    return 0;
  }
};

xiiMessageLoop::xiiMessageLoop() :
  m_SingletonRegistrar(this)
{
}

void xiiMessageLoop::StartUpdateThread()
{
  XII_LOCK(m_Mutex);
  if (m_pUpdateThread == nullptr)
  {
    m_pUpdateThread                     = XII_DEFAULT_NEW(xiiLoopThread);
    m_pUpdateThread->m_pRemoteInterface = this;
    m_pUpdateThread->Start();
  }
}

void xiiMessageLoop::StopUpdateThread()
{
  XII_LOCK(m_Mutex);
  if (m_pUpdateThread != nullptr)
  {
    m_bShouldQuit = true;
    WakeUp();
    m_pUpdateThread->Join();

    XII_DEFAULT_DELETE(m_pUpdateThread);
  }
}

void xiiMessageLoop::RunLoop()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  m_ThreadId = xiiThreadUtils::GetCurrentThreadID();
#endif

  while (true)
  {
    if (m_bCallTickFunction)
    {
      for (xiiIpcChannel* pChannel : m_AllAddedChannels)
      {
        if (pChannel->RequiresRegularTick())
        {
          pChannel->Tick();
        }
      }
    }

    // process all available data until all is processed and we wait for new messages.
    bool didwork = ProcessTasks();
    if (m_bShouldQuit)
      break;

    if (didwork)
      continue;

    didwork |= WaitForMessages(0, nullptr);
    if (m_bShouldQuit)
      break;

    if (didwork)
      continue;

    // wait until we have work again
    WaitForMessages(m_bCallTickFunction ? 50 : -1, nullptr); // timeout 20 times per second, if we need to call the tick function
  }
}

bool xiiMessageLoop::ProcessTasks()
{
  {
    XII_LOCK(m_TasksMutex);
    // Swap out the queues under the lock so we can process them without holding the lock
    m_ConnectQueueTask.Swap(m_ConnectQueue);
    m_SendQueueTask.Swap(m_SendQueue);
    m_DisconnectQueueTask.Swap(m_DisconnectQueue);
  }

  for (xiiIpcChannel* pChannel : m_ConnectQueueTask)
  {
    pChannel->InternalConnect();
  }
  for (xiiIpcChannel* pChannel : m_SendQueueTask)
  {
    pChannel->InternalSend();
  }
  for (xiiIpcChannel* pChannel : m_DisconnectQueueTask)
  {
    pChannel->InternalDisconnect();
  }

  bool bDidWork = !m_ConnectQueueTask.IsEmpty() || !m_SendQueueTask.IsEmpty() || !m_DisconnectQueueTask.IsEmpty();
  m_ConnectQueueTask.Clear();
  m_SendQueueTask.Clear();
  m_DisconnectQueueTask.Clear();
  return bDidWork;
}

void xiiMessageLoop::Quit()
{
  m_bShouldQuit = true;
}

void xiiMessageLoop::AddChannel(xiiIpcChannel* pChannel)
{
  {
    XII_LOCK(m_TasksMutex);
    m_AllAddedChannels.PushBack(pChannel);

    m_bCallTickFunction = false;
    for (auto pThisChannel : m_AllAddedChannels)
    {
      if (pThisChannel->RequiresRegularTick())
      {
        m_bCallTickFunction = true;
        break;
      }
    }
  }

  StartUpdateThread();
  pChannel->m_pOwner = this;
}

void xiiMessageLoop::RemoveChannel(xiiIpcChannel* pChannel)
{
  XII_LOCK(m_TasksMutex);

  m_AllAddedChannels.RemoveAndSwap(pChannel);
  m_ConnectQueue.RemoveAndSwap(pChannel);
  m_DisconnectQueue.RemoveAndSwap(pChannel);
  m_SendQueue.RemoveAndSwap(pChannel);
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_MessageLoop);
