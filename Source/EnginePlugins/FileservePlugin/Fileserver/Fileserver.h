/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

class xiiRemoteMessage;

struct xiiFileserverEvent
{
  enum class Type
  {
    None,
    ServerStarted,
    ServerStopped,
    ClientConnected,
    ClientReconnected, // connected again after a disconnect
    ClientDisconnected,
    MountDataDir,
    MountDataDirFailed,
    UnmountDataDir,
    FileDownloadRequest,
    FileDownloading,
    FileDownloadFinished,
    FileDeleteRequest,
    FileUploadRequest,
    FileUploading,
    FileUploadFinished,
    AreYouThereRequest,
    LogCustomActivity,
  };

  Type                  m_Type             = Type::None;
  xiiUInt32             m_uiClientID       = 0;
  const char*           m_szName           = nullptr;
  const char*           m_szPath           = nullptr;
  const char*           m_szRedirectedPath = nullptr;
  xiiUInt32             m_uiSizeTotal      = 0;
  xiiUInt32             m_uiSentTotal      = 0;
  xiiFileserveFileState m_FileState        = xiiFileserveFileState::None;
};

/// A file server allows to serve files from a host PC to another process that is potentially on another device.
///
/// This is mostly useful for mobile devices, that do not have access to the data on the development machine.
/// Typically every change to a file would require packaging the app and deploying it to the device again.
/// Fileserve allows to only deploy a very lean application and instead get all asset data directly from a host PC.
/// This also allows to modify data on the PC and reload the data in the running application without delay.
///
/// A single file server can serve multiple clients. However, to mount "special directories" (see xiiFileSystem) the server
/// needs to know what local path to map them to (it uses the configuration on xiiFileSystem).
/// That means it cannot serve two clients that require different settings for the same special directory.
///
/// The port on which the server connects to clients can be configured through the command line option "-fs_port X"
class XII_FILESERVEPLUGIN_DLL xiiFileserver
{
  XII_DECLARE_SINGLETON(xiiFileserver);

public:
  xiiFileserver();

  /// Starts listening for client connections. Uses the configured port.
  void StartServer();

  /// Disconnects all clients.
  void StopServer();

  /// Has to be executed regularly to serve clients and keep the connection alive.
  bool UpdateServer();

  /// Whether the server was started.
  bool IsServerRunning() const;

  /// Overrides the current port setting. May only be called when the server is currently not running.
  void SetPort(xiiUInt16 uiPort);

  /// Returns the currently set port. If the command line option "-fs_port X" was used, this will return that value, otherwise the default is
  /// 1042.
  xiiUInt16 GetPort() const { return m_uiPort; }

  /// The server broadcasts events about its activity
  xiiEvent<const xiiFileserverEvent&> m_Events;

  /// Broadcasts to all clients that they should reload their resources
  void BroadcastReloadResourcesCommand();

  static xiiResult SendConnectionInfo(const char* szClientAddress, xiiUInt16 uiMyPort, const xiiArrayPtr<xiiStringBuilder>& myIPs, xiiTime timeout = xiiTime::MakeFromSeconds(10));

  using ClientMessageHandler = xiiDelegate<void(xiiFileserveClientContext&, xiiRemoteMessage&, xiiRemoteInterface&, xiiDelegate<void(const char*)>)>;

  void SetCustomMessageHandler(xiiUInt32 uiSystemID, ClientMessageHandler handler);

private:
  void                       NetworkEventHandler(const xiiRemoteEvent& e);
  xiiFileserveClientContext& DetermineClient(xiiRemoteMessage& msg);
  void                       NetworkMsgHandler(xiiRemoteMessage& msg);
  void                       UnknownNetworkMsgHandler(xiiRemoteMessage& msg);
  void                       HandleMountRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg);
  void                       HandleUnmountRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg);
  void                       HandleFileRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg);
  void                       HandleDeleteFileRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg);
  void                       HandleUploadFileHeader(xiiFileserveClientContext& client, xiiRemoteMessage& msg);
  void                       HandleUploadFileTransfer(xiiFileserveClientContext& client, xiiRemoteMessage& msg);
  void                       HandleUploadFileFinished(xiiFileserveClientContext& client, xiiRemoteMessage& msg);
  void                       LogCustomActivity(const char* szText);

  xiiHashTable<xiiUInt32, xiiFileserveClientContext> m_Clients;
  xiiUniquePtr<xiiRemoteInterface>                   m_pNetwork;
  xiiDynamicArray<xiiUInt8>                          m_SendToClient;   // ie. 'downloads' from server to client
  xiiDynamicArray<xiiUInt8>                          m_SentFromClient; // ie. 'uploads' from client to server
  xiiStringBuilder                                   m_sCurFileUpload;
  xiiUuid                                            m_FileUploadGuid;
  xiiUInt32                                          m_uiFileUploadSize;
  xiiUInt16                                          m_uiPort = 1042;
  xiiMap<xiiUInt32, ClientMessageHandler>            m_CustomMessageHandlers;
};
