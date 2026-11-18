/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Events/Event.h>
#include <Foundation/Network/NetworkConnection.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class xiiEditorProcessTransport;
class xiiNetworkLocalEndpoint;

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiProcessMessage : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcessMessage, xiiReflectedClass);
};

enum class xiiProcessMessageDomain : xiiUInt8
{
  Engine,
  Asset
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiProcessCommunicationChannel
{
public:
  xiiProcessCommunicationChannel();
  ~xiiProcessCommunicationChannel();

  bool SendMessage(xiiProcessMessage* pMessage);

  /// One submission from a worker. Requires an established schema and never pumps events or waits for capacity.
  bool SendBackgroundMessage(const xiiProcessMessage& message);

  /// Callback for 'wait for...' functions. If true is returned, the message is accepted to match the wait criteria and the waiting ends. If false is returned the wait for the message continues.
  using WaitForMessageCallback = xiiDelegate<bool(xiiProcessMessage*)>;
  /// Bounded wait on the application thread. Cannot be used from a message callback.
  xiiResult WaitForMessage(const xiiRTTI* pMessageType, xiiTime timeout, WaitForMessageCallback* pMessageCallback = nullptr);
  xiiResult WaitForConnection(xiiTime timeout);
  bool      IsConnected() const;

  /// Returns true if any message was processed
  bool ProcessMessages();
  void WaitForMessages();

  struct Event
  {
    const xiiProcessMessage* m_pMessage;

    // Set to true in a message handler to cancel the ProcessMessages function and return to the caller before all messages have been processed.
    mutable bool m_bInterruptMessageProcessing = false;
  };

  xiiEvent<const Event&> m_Events;

protected:
  xiiResult      StartLocalListener(const xiiNetworkLocalEndpoint& endpoint, xiiProcessMessageDomain domain);
  xiiResult      ConnectLocal(const xiiNetworkLocalEndpoint& endpoint, xiiProcessMessageDomain domain, xiiUInt32 uiExpectedProcessId);
  xiiResult      StartRemoteListener(xiiStringView sAddress);
  xiiResult      ConnectRemote(xiiStringView sAddress);
  void           SetExpectedProcessId(xiiUInt32 uiProcessId);
  void           CloseTransport();
  bool           HasTransport() const;
  bool           HasPendingConnection() const;
  const xiiRTTI* m_pFirstAllowedMessageType = nullptr;

private:
  friend class xiiEditorProcessTransport;
  bool Pump(xiiTime wait);
  void DispatchMessage(const xiiProcessMessage& message);

  xiiMutex                                m_BackgroundSendMutex;
  xiiUniquePtr<xiiEditorProcessTransport> m_pTransport;
  WaitForMessageCallback                  m_WaitForMessageCallback;
  const xiiRTTI*                          m_pWaitForMessageType = nullptr;
  bool                                    m_bDispatchingMessage = false;
  bool                                    m_bInterrupted        = false;
};
