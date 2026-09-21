/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Types/UniquePtr.h>

class xiiIpcChannel;
class xiiMessageLoop;

/// A protocol wrapper around xiiIpcChannel to send and receive reflected messages instead of raw byte arrays.
///
/// This class hooks into an existing xiiIpcChannel instance to provide a high-level messaging protocol using reflected messages derived from xiiProcessMessage.
/// The underlying xiiIpcChannel remains responsible for all connection logic, while this protocol focuses solely on message serialization, deserialization, and dispatch.
///
/// \note When using this protocol, do not call xiiIpcChannel::Send directly. Instead, use xiiIpcProcessMessageProtocol::Send to ensure proper message formatting and handling.
///
/// Received messages are stored in an internal queue and must be processed explicitly by calling ProcessMessages() or WaitForMessages().
class XII_FOUNDATION_DLL xiiIpcProcessMessageProtocol
{
public:
  /// Constructs the protocol wrapper for a given IPC channel.
  ///
  /// \param pChannel - Pointer to an existing xiiIpcChannel instance. The channel must remain valid for the lifetime of this protocol object.
  xiiIpcProcessMessageProtocol(xiiIpcChannel* pChannel);

  /// Destructor. Cleans up any queued messages and detaches from the channel.
  ~xiiIpcProcessMessageProtocol();

  /// Sends a reflected process message over the IPC channel.
  ///
  /// \param pMsg - Pointer to the message to send. Ownership is not transferred; the message can be safely destroyed after this call returns.
  ///
  /// \return True if the message was successfully queued for sending, false otherwise.
  bool Send(xiiProcessMessage* pMsg);

  /// Processes all pending incoming messages.
  ///
  /// This method dequeues all received messages and broadcasts them via m_MessageEvent.
  /// It is not re-entrant; calling it from within a message handler is not supported.
  ///
  /// \return True if any messages were processed, false otherwise.
  bool ProcessMessages();

  /// Waits for new messages to arrive and processes them.
  ///
  /// This method blocks until either a message is received or the specified timeout elapses.
  /// Once unblocked, it calls ProcessMessages() to handle all queued messages.
  ///
  /// \param timeout Maximum time to wait for a message. Defaults to zero (non-blocking).
  ///
  /// \return xiiResult::Success if messages were processed, xiiResult::Failure on timeout or error.
  xiiResult WaitForMessages(xiiTime timeout = xiiTime::MakeZero());

public:
  /// Event data structure for message notifications.
  struct Event
  {
    /// Pointer to the received message.
    const xiiProcessMessage* m_pMessage;

    /// Set to true within a message handler to interrupt message processing.
    ///
    /// If set, ProcessMessages() will return immediately without processing further messages.
    mutable bool m_bInterruptMessageProcessing = false;
  };

  /// Event fired when a new message is processed.
  ///
  /// This event is triggered from the thread calling ProcessMessages() or WaitForMessages().
  xiiEvent<const Event&> m_MessageEvent;

private:
  /// Adds a newly received message to the incoming queue.
  /// \param msg Unique pointer to the message to enqueue.
  void EnqueueMessage(xiiUniquePtr<xiiProcessMessage>&& msg);

  /// Removes and returns the next message from the incoming queue.
  /// \return Unique pointer to the next queued message, or nullptr if the queue is empty.
  xiiUniquePtr<xiiProcessMessage> PopMessage();

  /// Handles raw message data received from the IPC channel.
  /// \param data Byte array containing the serialized message.
  void ReceiveMessageData(xiiArrayPtr<const xiiUInt8> data);

private:
  xiiIpcChannel* m_pChannel = nullptr; ///< Pointer to the associated IPC channel.

  xiiMutex m_IncomingQueueMutex; ///< Mutex protecting access to the incoming message queue.

  /// Queue of incoming messages awaiting processing.
  xiiDeque<xiiUniquePtr<xiiProcessMessage>> m_IncomingQueue;
};
