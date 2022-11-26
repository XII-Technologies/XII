#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>

class xiiIpcChannel;
class xiiProcessMessage;

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiProcessCommunicationChannel
{
public:
  xiiProcessCommunicationChannel();
  ~xiiProcessCommunicationChannel();

  void SendMessage(xiiProcessMessage* pMessage);

  /// /brief Callback for 'wait for...' functions. If true is returned, the message is accepted to match the wait criteria and
  ///        the waiting ends. If false is returned the wait for the message continues.
  typedef xiiDelegate<bool(xiiProcessMessage*)> WaitForMessageCallback;
  xiiResult                                     WaitForMessage(const xiiRTTI* pMessageType, xiiTime tTimeout, WaitForMessageCallback* pMessageCallack = nullptr);
  xiiResult                                     WaitForConnection(xiiTime tTimeout);

  /// \brief Returns true if any message was processed
  bool ProcessMessages();
  void WaitForMessages();

  struct Event
  {
    const xiiProcessMessage* m_pMessage;
  };

  xiiEvent<const Event&> m_Events;

  void MessageFunc(const xiiProcessMessage* msg);

protected:
  xiiIpcChannel* m_pChannel                 = nullptr;
  const xiiRTTI* m_pFirstAllowedMessageType = nullptr;

private:
  WaitForMessageCallback m_WaitForMessageCallback;
  const xiiRTTI*         m_pWaitForMessageType = nullptr;
};
