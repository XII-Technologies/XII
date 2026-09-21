/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>

/// Whether the remote interface is configured as a server or a client
enum class xiiRemoteMode
{
  None,   ///< Remote interface is shut down
  Server, ///< Remote interface acts as a server. Can connect with multiple clients
  Client  ///< Remote interface acts as a client. Can connect with exactly one server.
};

/// Mode for transmitting messages
///
/// Depending on the remote interface implementation, Unreliable may not be supported and revert to Reliable.
enum class xiiRemoteTransmitMode
{
  Reliable,   ///< Messages should definitely arrive at the target, if necessary they are send several times, until the target acknowledged it.
  Unreliable, ///< Messages are sent at most once, if they get lost, they are not resent. If it is known beforehand, that not receiver exists, they
              ///< are dropped without sending them at all.
};

/// Event type for connections
struct XII_FOUNDATION_DLL xiiRemoteEvent
{
  enum Type
  {
    ConnectedToClient,      ///< brief Sent whenever a new connection to a client has been established.
    ConnectedToServer,      ///< brief Sent whenever a connection to the server has been established.
    DisconnectedFromClient, ///< Sent every time the connection to a client is dropped
    DisconnectedFromServer, ///< Sent when the connection to the server has been lost
  };

  Type      m_Type;
  xiiUInt32 m_uiOtherAppID;
};

using xiiRemoteMessageHandler = xiiDelegate<void(xiiRemoteMessage&)>;

struct XII_FOUNDATION_DLL xiiRemoteMessageQueue
{
  xiiRemoteMessageHandler m_MessageHandler;
  /// Messages are pushed into this container on arrival.
  xiiDeque<xiiRemoteMessage> m_MessageQueueIn;
  /// To flush the message queue, m_MessageQueueIn and m_MessageQueueOut are swapped.
  /// Thus new messages can arrive while we execute the event handler for each element
  /// in this container and then clear it.
  xiiDeque<xiiRemoteMessage> m_MessageQueueOut;
};

class XII_FOUNDATION_DLL xiiRemoteInterface
{
public:
  virtual ~xiiRemoteInterface();

  /// Exposes the mutex that is internally used to secure multi-threaded access
  xiiMutex& GetMutex() const { return m_Mutex; }


  /// \name Connection
  ///@{

  /// Starts the remote interface as a server.
  ///
  /// \param uiConnectionToken Should be a unique sequence (e.g. 'XIIPZ') to identify the purpose of this connection.
  /// Only server and clients with the same token will accept connections.
  /// \param uiPort The port over which the connection should run.
  /// \param bStartUpdateThread If true, a thread is started that will regularly call UpdateNetwork() and UpdatePingToServer().
  /// If false, this has to be called manually in regular intervals.
  xiiResult StartServer(xiiUInt32 uiConnectionToken, xiiStringView sAddress, bool bStartUpdateThread = true);

  /// Starts the network interface as a client. Tries to connect to the given address.
  ///
  /// This function immediately returns and no connection is guaranteed.
  /// \param uiConnectionToken Same as for StartServer()
  /// \param szAddress Could be a network address "127.0.0.1" or "localhost" or some other name that identifies the target, e.g. a named pipe.
  /// \param bStartUpdateThread Same as for StartServer()
  ///
  /// If this function succeeds, it still might not be connected to a server.
  /// Use WaitForConnectionToServer() to enforce a connection.
  xiiResult ConnectToServer(xiiUInt32 uiConnectionToken, xiiStringView sAddress, bool bStartUpdateThread = true);

  /// Can only be called after ConnectToServer(). Updates the network in a loop until a connection is established, or the time has run out.
  ///
  /// A timeout of exactly zero means to wait indefinitely.
  xiiResult WaitForConnectionToServer(xiiTime timeout = xiiTime::MakeFromSeconds(10));

  /// Closes the connection in an orderly fashion
  void ShutdownConnection();

  /// Whether the client is connected to a server
  bool IsConnectedToServer() const { return m_uiConnectedToServerWithID != 0; }

  /// Whether the server is connected to any client
  bool IsConnectedToClients() const { return m_iConnectionsToClients > 0; }

  /// Whether the client or server is connected its counterpart
  bool IsConnectedToOther() const { return IsConnectedToServer() || IsConnectedToClients(); }

  /// Whether the remote interface is inactive, a client or a server
  xiiRemoteMode GetRemoteMode() const { return m_RemoteMode; }

  /// The address through which the connection was started
  const xiiString& GetServerAddress() const { return m_sServerAddress; }

  /// Returns the own (random) application ID used to identify this instance
  xiiUInt32 GetApplicationID() const { return m_uiApplicationID; }

  /// Returns the connection token used to identify compatible servers/clients
  xiiUInt32 GetConnectionToken() const { return m_uiConnectionToken; }

  ///@}

  /// \name Server Information
  ///@{

  /// For the client to display the name of the server
  // const xiiString& GetServerInfoName() const { return m_ServerInfoName; }

  /// For the client to display the IP of the server
  const xiiString& GetServerInfoIP() const { return m_sServerInfoIP; }

  /// Some random identifier, that allows to determine after a reconnect, whether the connected instance is still the same server
  xiiUInt32 GetServerID() const { return m_uiConnectedToServerWithID; }

  /// Returns the current ping to the server
  xiiTime GetPingToServer() const { return m_PingToServer; }

  ///@}

  /// \name Updating the Remote Interface
  ///@{

  /// If no update thread was spawned, this should be called to process messages
  void UpdateRemoteInterface();

  /// If no update thread was spawned, this should be called by clients to determine the ping
  void UpdatePingToServer();

  ///@}

  /// \name Sending Messages
  ///@{

  /// Sends a reliable message without any data.
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(xiiUInt32 uiSystemID, xiiUInt32 uiMsgID);

  /// Sends a message, appends the given array of data
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(xiiRemoteTransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const xiiArrayPtr<const xiiUInt8>& data);

  void Send(xiiRemoteTransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const xiiContiguousMemoryStreamStorage& data);

  /// Sends a message, appends the given array of data
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(xiiRemoteTransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const void* pData = nullptr, xiiUInt32 uiDataBytes = 0);

  /// Sends a xiiRemoteMessage
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(xiiRemoteTransmitMode tm, xiiRemoteMessage& ref_msg);

  ///@}

  /// \name Message Handling
  ///@{

  /// Registers a message handler that is executed for all incoming messages for the given system
  void SetMessageHandler(xiiUInt32 uiSystemID, xiiRemoteMessageHandler messageHandler);

  /// Registers a message handler that is executed for all incoming messages for systems for which there are no dedicated message handlers.
  void SetUnhandledMessageHandler(xiiRemoteMessageHandler messageHandler);

  /// Executes the message handler for all messages that have arrived for the given system
  xiiUInt32 ExecuteMessageHandlers(xiiUInt32 uiSystem);

  /// Executes all message handlers for all received messages
  xiiUInt32 ExecuteAllMessageHandlers();

  ///@}

  /// \name Events
  ///@{

  /// Broadcasts events about connections
  xiiEvent<const xiiRemoteEvent&> m_RemoteEvents;

  ///@}

protected:
  /// \name Implementation Details
  ///@{

  /// Derived classes have to implement this to start a network connection
  virtual xiiResult InternalCreateConnection(xiiRemoteMode mode, xiiStringView sServerAddress) = 0;

  /// Derived classes have to implement this to shutdown a network connection
  virtual void InternalShutdownConnection() = 0;

  /// Derived classes have to implement this to update
  virtual void InternalUpdateRemoteInterface() = 0;

  /// Derived classes have to implement this to get the ping to the server (client mode only)
  virtual xiiTime InternalGetPingToServer() = 0;

  /// Derived classes have to implement this to deliver messages to the server or client
  virtual xiiResult InternalTransmit(xiiRemoteTransmitMode tm, const xiiArrayPtr<const xiiUInt8>& data) = 0;

  /// Derived classes can override this to interpret an address differently
  virtual xiiResult DetermineTargetAddress(xiiStringView sConnectTo, xiiUInt32& out_IP, xiiUInt16& out_Port);

  /// Derived classes should update this when the information is available
  // xiiString m_ServerInfoName;
  /// Derived classes should update this when the information is available
  xiiString m_sServerInfoIP;

  /// Should be called by the implementation, when a server connection has been established
  void ReportConnectionToServer(xiiUInt32 uiServerID);
  /// Should be called by the implementation, when a client connection has been established
  void ReportConnectionToClient(xiiUInt32 uiApplicationID);
  /// Should be called by the implementation, when a server connection has been lost
  void ReportDisconnectedFromServer();
  /// Should be called by the implementation, when a client connection has been lost
  void ReportDisconnectedFromClient(xiiUInt32 uiApplicationID);
  /// Should be called by the implementation, when a message has arrived
  void ReportMessage(xiiUInt32 uiApplicationID, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const xiiArrayPtr<const xiiUInt8>& data);

  ///@}


private:
  void      StartUpdateThread();
  void      StopUpdateThread();
  xiiResult Transmit(xiiRemoteTransmitMode tm, const xiiArrayPtr<const xiiUInt8>& data);
  xiiResult CreateConnection(xiiUInt32 uiConnectionToken, xiiRemoteMode mode, xiiStringView sServerAddress, bool bStartUpdateThread);
  xiiUInt32 ExecuteMessageHandlersForQueue(xiiRemoteMessageQueue& queue);

  mutable xiiMutex                               m_Mutex;
  class xiiRemoteThread*                         m_pUpdateThread = nullptr;
  xiiRemoteMode                                  m_RemoteMode    = xiiRemoteMode::None;
  xiiString                                      m_sServerAddress;
  xiiTime                                        m_PingToServer;
  xiiUInt32                                      m_uiApplicationID           = 0; // sent when connecting to identify the sending instance
  xiiUInt32                                      m_uiConnectionToken         = 0;
  xiiUInt32                                      m_uiConnectedToServerWithID = 0;
  xiiInt32                                       m_iConnectionsToClients     = 0;
  xiiDynamicArray<xiiUInt8>                      m_TempSendBuffer;
  xiiHashTable<xiiUInt32, xiiRemoteMessageQueue> m_MessageQueues;
  xiiRemoteMessageHandler                        m_UnhandledMessageHandler;
};

/// The remote interface thread updates in regular intervals to keep the connection alive.
///
/// The thread does NOT call xiiRemoteInterface::ExecuteAllMessageHandlers(), so by default no message handlers are executed.
/// This has to be done manually by the application elsewhere.
class XII_FOUNDATION_DLL xiiRemoteThread : public xiiThread
{
public:
  xiiRemoteThread();

  xiiRemoteInterface* m_pRemoteInterface = nullptr;
  volatile bool       m_bKeepRunning     = true;

private:
  virtual xiiUInt32 Run();
};
