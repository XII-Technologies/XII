/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

/// Base class with shared functionality for xiiLongOpControllerManager and xiiLongOpWorkerManager
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLongOpManager
{
public:
  /// Needs to be called early to initialize the IPC channel to use.
  void Startup(xiiProcessCommunicationChannel* pCommunicationChannel);

  /// Call this to shut down the IPC communication.
  void Shutdown();

  /// Publicly exposed mutex for some special cases.
  mutable xiiMutex m_Mutex;

protected:
  virtual void ProcessCommunicationChannelEventHandler(const xiiProcessCommunicationChannel::Event& e) = 0;

  xiiProcessCommunicationChannel*                                      m_pCommunicationChannel = nullptr;
  xiiEvent<const xiiProcessCommunicationChannel::Event&>::Unsubscriber m_Unsubscriber;
};
