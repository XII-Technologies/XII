/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>

class xiiProcessMessage;
class xiiIpcChannel;
class xiiLoopThread;

/// Internal sub-system used by xiiIpcChannel.
///
/// This sub-system creates a background thread as soon as the first xiiIpcChannel
/// is added to it. This class should never be needed to be accessed outside
/// of xiiIpcChannel implementations.
class XII_FOUNDATION_DLL xiiMessageLoop
{
  XII_DECLARE_SINGLETON(xiiMessageLoop);

public:
  xiiMessageLoop();
  virtual ~xiiMessageLoop() = default;
  ;

  /// Needs to be called by newly created channels' constructors.
  void AddChannel(xiiIpcChannel* pChannel);

  void RemoveChannel(xiiIpcChannel* pChannel);

protected:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, MessageLoop);
  friend class xiiLoopThread;
  friend class xiiIpcChannel;

  void StartUpdateThread();
  void StopUpdateThread();
  void RunLoop();
  bool ProcessTasks();
  void Quit();

  /// Wake up the message loop when new work comes in.
  virtual void WakeUp() = 0;
  /// Waits until a new message has been processed (sent, received).
  /// \param timeout If negative, wait indefinitely.
  /// \param pFilter If not null, wait for a message for the specific channel.
  /// \return Returns whether a message was received or the timeout was reached.
  virtual bool WaitForMessages(xiiInt32 iTimeout, xiiIpcChannel* pFilter) = 0;

  xiiThreadID          m_ThreadId = 0;
  mutable xiiMutex     m_Mutex;
  bool                 m_bShouldQuit       = false;
  bool                 m_bCallTickFunction = false;
  class xiiLoopThread* m_pUpdateThread     = nullptr;

  xiiMutex                        m_TasksMutex;
  xiiDynamicArray<xiiIpcChannel*> m_ConnectQueue;
  xiiDynamicArray<xiiIpcChannel*> m_DisconnectQueue;
  xiiDynamicArray<xiiIpcChannel*> m_SendQueue;

  // Thread local copies of the different queues for the ProcessTasks method
  xiiDynamicArray<xiiIpcChannel*> m_ConnectQueueTask;
  xiiDynamicArray<xiiIpcChannel*> m_DisconnectQueueTask;
  xiiDynamicArray<xiiIpcChannel*> m_SendQueueTask;

  xiiDynamicArray<xiiIpcChannel*> m_AllAddedChannels;
};
