/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <FileservePlugin/FileservePluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Interfaces/RemoteToolingInterface.h>
#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

namespace xiiDataDirectory
{
  class FileserveType;
}

namespace xiiFileserveTransfer
{
  enum class Operation : xiiUInt8;
  class Request;
} // namespace xiiFileserveTransfer

/// Singleton that represents the client side part of a fileserve connection
///
/// Whether the fileserve plugin will be enabled is controled by xiiFileserveClient::s_bEnableFileserve
/// By default this is on, but if switched off, the fileserve client functionality will be disabled.
/// xiiFileserveClient will also switch its functionality off, if the command line argument "-fs_off" is specified.
/// If a program knows that it always wants to switch file serving off, it should either simply not load the plugin at all,
/// or it can inject that command line argument through xiiCommandLineUtils. This should be done before application startup
/// and especially before any data directories get mounted.
///
/// The timeout for connecting to the server can be configured through the command line option "-fs_timeout seconds"
/// The server to connect to can be configured through command line option "-fs_server address".
/// The default address is "localhost:1042".
class XII_FILESERVEPLUGIN_DLL xiiFileserveClient : public xiiRemoteToolingInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiFileserveClient, xiiRemoteToolingInterface);

public:
  xiiFileserveClient();
  ~xiiFileserveClient();

  /// xiiRemoteToolingInterface

  bool      IsShaderCompilationAvailable() const override;
  xiiResult CompileShader(const xiiRemoteShaderCompileRequest& request, xiiStringBuilder& out_sLog) override;

  /// Can be called at startup to go through multiple sources and search for a valid server address
  ///
  /// Ie. checks the command line, xiiFileserve.txt in different directories, etc.
  /// For every potential IP it checks whether a fileserve connection could be established (e.g. tries to connect and
  /// checks whether the server answers). If a valid connection is found, the IP is stored internally and XII_SUCCESS is returned.
  /// Call GetServerConnectionAddress() to retrieve the address.
  ///
  /// \param timeout Specifies the timeout for checking whether a server can be reached.
  xiiResult SearchForServerAddress(xiiTime timeout = xiiTime::MakeFromSeconds(5));

  /// Waits for a Fileserver application to try to connect to this device and send its own information.
  ///
  /// This can be used when a device has no proper way to know the IP through which to connect to a Fileserver.
  /// Instead the device opens a server connection itself, and waits for the other side to try to connect to it.
  /// This typically means that a human has to manually input this device's IP on the host PC into the Fileserve application,
  /// thus enabling the exchange of connection information.
  /// Once this has happened, this function stores the valid server IP internally and returns with success.
  /// A subsequent call to EnsureConnected() should then succeed.
  xiiResult WaitForServerInfo(xiiTime timeout = xiiTime::MakeFromSeconds(60.0 * 5));

  /// Stores the current connection info to a text file in the user data folder.
  xiiResult SaveCurrentConnectionInfoToDisk() const;

  /// Allows to disable the file serving functionality. Should be called before mounting data directories.
  ///
  /// Also achieved through the command line argument "-fs_off"
  static void DisabledFileserveClient() { s_bEnableFileserve = false; }

  /// Returns the address through which the Fileserve client tried to connect with the server last.
  const char* GetServerConnectionAddress() { return m_sServerConnectionAddress; }

  /// Can be called to ensure a fileserve connection. Otherwise automatically called when a data directory is mounted.
  ///
  /// The timeout defines how long the code will wait for a connection.
  /// Positive numbers are a regular timeout.
  /// Zero or durations above 120 seconds use the 120-second connection limit.
  /// A negative number means to either wait that time, or whatever was specified through the command-line.
  /// The timeout can be specified with the command line switch "-fs_timeout X" (in seconds).
  xiiResult EnsureConnected(xiiTime timeout = xiiTime::MakeFromSeconds(-5));

  /// Needs to be called regularly to update the network. By default this is automatically called when the global event
  /// 'GameApp_UpdatePlugins' is fired, which is done by xiiGameApplication.
  void UpdateClient();

  /// Adds an address that should be tried for connecting with the server.
  void AddServerAddressToTry(xiiStringView sAddress);

private:
  friend class xiiDataDirectory::FileserveType;

  /// True by default, can
  static bool s_bEnableFileserve;

  struct FileCacheStatus
  {
    xiiInt64  m_TimeStamp = 0;
    xiiUInt64 m_FileHash  = 0;
    xiiTime   m_LastCheck;
  };

  struct DataDir
  {
    xiiString m_sRootName;
    xiiString m_sPathOnClient;
    xiiString m_sMountPoint;
    bool      m_bMounted = false;

    xiiMap<xiiString, FileCacheStatus> m_CacheStatus;
  };

  void             DeleteFile(xiiUInt16 uiDataDir, xiiStringView sFile);
  xiiUInt16        MountDataDirectory(xiiStringView sDataDir, xiiStringView sRootName);
  void             UnmountDataDirectory(xiiUInt16 uiDataDir);
  static void      ComputeDataDirMountPoint(xiiStringView sDataDir, xiiStringBuilder& out_sMountPoint);
  void             BuildPathInCache(const char* szFile, const char* szMountPoint, xiiStringBuilder* out_pAbsPath, xiiStringBuilder* out_pFullPathMeta) const;
  void             GetFullDataDirCachePath(const char* szDataDir, xiiStringBuilder& out_sFullPath, xiiStringBuilder& out_sFullPathMeta) const;
  void             ProcessNetworkEvents(xiiTime wait = {});
  void             NetworkMsgHandler(xiiFileserveTransfer::Operation operation, xiiArrayPtr<const xiiUInt8> data);
  void             HandleFileTransferMsg(xiiArrayPtr<const xiiUInt8> data);
  void             HandleFileTransferFinishedMsg(xiiArrayPtr<const xiiUInt8> data);
  xiiResult        SendRequest(const xiiFileserveTransfer::Request& request, xiiTime deadline);
  xiiResult        SendMount(xiiUInt16 uiDataDir, const DataDir& dir);
  static xiiResult WriteMetaFile(xiiStringView sCachedMetaFile, xiiInt64 iFileTimeStamp, xiiUInt64 uiFileHash);
  xiiResult        CommitDownload(xiiStringView sCachedFile, xiiUInt64 uiHash);
  void             DiscardDownload();
  xiiResult        DownloadFile(xiiUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir, xiiStringBuilder* out_pFullPath);
  void             DetermineCacheStatus(xiiUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const;
  xiiResult        UploadFile(xiiUInt16 uiDataDirID, xiiStringView sFile, xiiOSFile& ref_file);
  void             InvalidateFileCache(xiiUInt16 uiDataDirID, xiiStringView sFile, xiiUInt64 uiHash);
  void             InvalidateAllCache();
  static xiiResult TryReadFileserveConfig(const char* szFile, xiiStringBuilder& out_Result);
  xiiResult        TryConnectWithFileserver(const char* szAddress, xiiTime timeout) const;
  void             FillFileStatusCache(const char* szFile);
  void             ShutdownConnection();
  void             ClearState();

  mutable xiiMutex                               m_Mutex;
  mutable xiiString                              m_sServerConnectionAddress;
  xiiString                                      m_sFileserveCacheFolder;
  xiiString                                      m_sFileserveCacheMetaFolder;
  bool                                           m_bDownloading              = false;
  bool                                           m_bFailedToConnect          = false;
  bool                                           m_bWaitingForUploadFinished = false;
  bool                                           m_bUploadSucceeded          = false;
  xiiUuid                                        m_UploadRequest;
  xiiUInt32                                      m_uiUploadAcknowledged = 0;
  xiiUuid                                        m_CurFileRequestGuid;
  xiiStringBuilder                               m_sCurFileRequest;
  xiiUniquePtr<xiiNetworkHost>                   m_pNetwork;
  xiiNetworkChannel                              m_Channel;
  xiiDelegate<void(xiiArrayPtr<const xiiUInt8>)> m_ShaderResponse;
  xiiOSFile                                      m_DownloadFile;
  xiiString                                      m_sDownloadStagingPath;
  xiiUInt32                                      m_uiDownloadSize        = 0;
  xiiUInt32                                      m_uiDownloadReceived    = 0;
  xiiUInt16                                      m_uiDownloadDataDir     = 0xffff;
  bool                                           m_bDownloadSucceeded    = false;
  bool                                           m_bDownloadStaged       = false;
  bool                                           m_bShaderCompiling      = false;
  bool                                           m_bForceDownloadDataDir = false;
  bool                                           m_bReloadResources      = false;
  xiiHybridArray<xiiString, 4>                   m_TryServerAddresses;

  xiiMap<xiiString, xiiUInt16> m_FileDataDir;
  xiiHybridArray<DataDir, 8>   m_MountedDataDirs;
};
