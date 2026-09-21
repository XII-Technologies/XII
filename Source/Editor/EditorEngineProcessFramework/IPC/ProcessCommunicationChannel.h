/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class xiiIpcChannel;
class xiiProcessMessage;

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiProcessCommunicationChannel
{
public:
  xiiProcessCommunicationChannel();
  ~xiiProcessCommunicationChannel();

  bool SendMessage(xiiProcessMessage* pMessage);

  /// Callback for 'wait for...' functions. If true is returned, the message is accepted to match the wait criteria and the waiting ends. If false is returned the wait for the message continues.
  using WaitForMessageCallback = xiiDelegate<bool(xiiProcessMessage*)>;
  xiiResult WaitForMessage(const xiiRTTI* pMessageType, xiiTime timeout, WaitForMessageCallback* pMessageCallack = nullptr);
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

  void MessageFunc(const xiiIpcProcessMessageProtocol::Event& msg);

protected:
  xiiUniquePtr<xiiIpcProcessMessageProtocol> m_pProtocol;
  xiiUniquePtr<xiiIpcChannel>                m_pChannel;
  const xiiRTTI*                             m_pFirstAllowedMessageType = nullptr;

private:
  WaitForMessageCallback m_WaitForMessageCallback;
  const xiiRTTI*         m_pWaitForMessageType = nullptr;
};
