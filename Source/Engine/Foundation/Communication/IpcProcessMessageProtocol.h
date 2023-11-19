#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Types/UniquePtr.h>

class xiiIpcChannel;
class xiiMessageLoop;


/// \brief A protocol around xiiIpcChannel to send reflected messages instead of byte array messages between client and server.
///
/// This wrapper class hooks into an existing xiiIpcChannel. The xiiIpcChannel is still responsible for all connection logic. This class merely provides a high-level messaging protocol via reflected messages derived from xiiProcessMessage.
/// Note that if this class is used, xiiIpcChannel::Send must not be called manually anymore, only use xiiIpcProcessMessageProtocol::Send.
/// Received messages are stored in a queue and must be flushed via calling ProcessMessages or WaitForMessages.
class XII_FOUNDATION_DLL xiiIpcProcessMessageProtocol
{
public:
  xiiIpcProcessMessageProtocol(xiiIpcChannel* pChannel);
  ~xiiIpcProcessMessageProtocol();

  /// \brief Sends a message. pMsg can be destroyed after the call.
  bool Send(xiiProcessMessage* pMsg);


  /// \brief Processes all pending messages by broadcasting m_MessageEvent. Not re-entrant.
  bool ProcessMessages();
  /// \brief Block and wait for new messages and call ProcessMessages.
  xiiResult WaitForMessages(xiiTime timeout = xiiTime::Zero());

public:
  xiiEvent<const xiiProcessMessage*> m_MessageEvent; ///< Will be sent from thread calling ProcessMessages or WaitForMessages.

private:
  void EnqueueMessage(xiiUniquePtr<xiiProcessMessage>&& msg);
  void SwapWorkQueue(xiiDeque<xiiUniquePtr<xiiProcessMessage>>& messages);
  void ReceiveMessageData(xiiArrayPtr<const xiiUInt8> data);

private:
  xiiIpcChannel* m_pChannel = nullptr;

  xiiMutex                                  m_IncomingQueueMutex;
  xiiDeque<xiiUniquePtr<xiiProcessMessage>> m_IncomingQueue;
};
