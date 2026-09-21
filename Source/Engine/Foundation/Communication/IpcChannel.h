/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

class xiiIpcChannel;
class xiiMessageLoop;

/// Event data for xiiIpcChannel::m_Events
struct XII_FOUNDATION_DLL xiiIpcChannelEvent
{
  using StorageType = xiiUInt8;

  enum Type : StorageType
  {
    Disconnected = 0U, ///< Server or client are in a dormant state.
    Connecting,        ///< The server is listening for clients or the client is trying to find the server.
    Connected,         ///< Client and server are connected to each other.
    NewMessages,       ///< Sent when a new messages have been received or when disconnected to wake up any thread waiting for messages.
  };

  xiiIpcChannelEvent() = default;

  xiiIpcChannelEvent(Type type, xiiIpcChannel* pChannel) :
    m_Type(type), m_pChannel(pChannel)
  {
  }

  Type           m_Type     = NewMessages;
  xiiIpcChannel* m_pChannel = nullptr;
};

/// Base class for a communication channel between processes.
///
///  The channel allows for byte blobs to be send back and forth between two processes.
///  A client should only try to connect to a server once the server has changed to ConnectionState::Connecting as this indicates the server is ready to be connected to.
///
///  Use xiiIpcChannel:::CreatePipeChannel to create an IPC pipe instance.
///  To send more complex messages across, you can create a xiiIpcProcessMessageProtocol on top of the channel.
class XII_FOUNDATION_DLL xiiIpcChannel
{
public:
  struct Mode
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Server,
      Client,
      Default = Server
    };
  };

  struct ConnectionState
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Disconnected,
      Connecting, ///< In case of the server, this state indicates that the server is ready to be connected to.
      Connected,
      Default = Disconnected
    };
  };

  virtual ~xiiIpcChannel();

  /// Creates an IPC communication channel using pipes.
  /// \param sAddress Name of the pipe, must be unique on a system and less than 200 characters.
  /// \param mode Whether to run in client or server mode.
  static xiiInternal::NewInstance<xiiIpcChannel> CreatePipeChannel(xiiStringView sAddress, Mode::Enum mode);

  static xiiInternal::NewInstance<xiiIpcChannel> CreateNetworkChannel(xiiStringView sAddress, Mode::Enum mode);


  /// Connects async. On success, m_Events will be broadcasted.
  void Connect();
  /// Disconnect async. On completion, m_Events will be broadcasted.
  void Disconnect();
  /// Returns whether we have a connection.
  bool IsConnected() const { return m_iConnectionState == ConnectionState::Connected; }
  /// Returns the current state of the connection.
  xiiEnum<ConnectionState> GetConnectionState() const { return xiiEnum<ConnectionState>(m_iConnectionState); }

  /// Sends a message. pMsg can be destroyed after the call.
  bool Send(xiiArrayPtr<const xiiUInt8> pData);

  using ReceiveCallback = xiiDelegate<void(xiiArrayPtr<const xiiUInt8> message)>;
  void SetReceiveCallback(ReceiveCallback callback);

  /// Block and wait for new messages and call ProcessMessages.
  xiiResult WaitForMessages(xiiTime timeout);

public:
  xiiEvent<const xiiIpcChannelEvent&, xiiMutex> m_Events; ///< Will be sent from any thread.

protected:
  xiiIpcChannel(xiiStringView sAddress, Mode::Enum mode);

  /// Override this and return true, if the surrounding infrastructure should call the 'Tick()' function.
  virtual bool RequiresRegularTick() { return false; }
  /// Can implement regular updates, e.g. for polling network state.
  virtual void Tick() {}

  /// Called on worker thread after Connect was called.
  virtual void InternalConnect() = 0;
  /// Called on worker thread after Disconnect was called.
  virtual void InternalDisconnect() = 0;
  /// Called on worker thread to sent pending messages.
  virtual void InternalSend() = 0;
  /// Called by Send to determine whether the message loop need to be woken up.
  virtual bool NeedWakeup() const = 0;

  void SetConnectionState(xiiEnum<ConnectionState> state);
  /// Implementation needs to call this when new data has been received.
  ///  data can be invalidated after the function.
  void ReceiveData(xiiArrayPtr<const xiiUInt8> pData);
  void FlushPendingOperations();

private:
protected:
  enum Constants : xiiUInt32
  {
    HEADER_SIZE      = 8,                ///< Magic value and size xiiUint32
    MAGIC_VALUE      = 'USED',           ///< Magic value
    MAX_MESSAGE_SIZE = 1024 * 1024 * 16, ///< Arbitrary message size limit
  };

  friend class xiiMessageLoop;
  xiiThreadID m_ThreadId = 0;

  xiiAtomicInteger<ConnectionState::Enum> m_iConnectionState = ConnectionState::Disconnected;

  // Setup in ctor
  xiiString           m_sAddress;
  const xiiEnum<Mode> m_Mode;
  xiiMessageLoop*     m_pOwner = nullptr;

  // Mutex locked
  xiiMutex                                   m_OutputQueueMutex;
  xiiDeque<xiiContiguousMemoryStreamStorage> m_OutputQueue;

  // Only accessed from worker thread
  xiiDynamicArray<xiiUInt8> m_MessageAccumulator; ///< Message is assembled in here

  // Mutex locked
  xiiMutex        m_ReceiveCallbackMutex;
  ReceiveCallback m_ReceiveCallback;
  xiiThreadSignal m_IncomingMessages;
};
