/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER) && XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/Configuration/Singleton.h>
#  include <Foundation/Threading/LockedObject.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <ToolsFoundation/FileSystem/DataDirPath.h>
#  include <ToolsFoundation/FileSystem/Declarations.h>

class xiiFileSystemWatcher;
struct xiiFileSystemWatcherEvent;
struct xiiFileStats;

/// Event fired by xiiFileSystemModel::m_FolderChangedEvents
struct XII_TOOLSFOUNDATION_DLL xiiFolderChangedEvent
{
  enum class Type
  {
    None,
    FolderAdded,
    FolderRemoved,
    ModelReset, ///< Model was initialized or deinitialized.
  };

  xiiFolderChangedEvent() = default;
  xiiFolderChangedEvent(const xiiDataDirPath& file, Type type);

  xiiDataDirPath m_Path;
  Type           m_Type = Type::None;
};

/// Event fired by xiiFileSystemModel::m_FileChangedEvents
struct XII_TOOLSFOUNDATION_DLL xiiFileChangedEvent
{
  enum class Type
  {
    None,
    FileAdded,
    FileChanged,
    DocumentLinked,
    DocumentUnlinked,
    FileRemoved,
    ModelReset ///< Model was initialized or deinitialized.
  };

  xiiFileChangedEvent() = default;
  xiiFileChangedEvent(const xiiDataDirPath& file, xiiFileStatus status, Type type);

  xiiDataDirPath m_Path;
  xiiFileStatus  m_Status;
  Type           m_Type = Type::None;
};

/// A subsystem for tracking all files in a xiiApplicationFileSystemConfig.
///
/// Once Initialize is called with the xiiApplicationFileSystemConfig to track, the current state should be updated by calling CheckFileSystem() on a worker thread. This will trigger m_FolderChangedEvents and m_FileChangedEvents for all files / folders found in the data directories present in the config. Any future changes will be picked up by the xiiFileSystemWatcher created in Initialize.
/// For the system to work, the MainThreadTick function needs to be called at regular (e.g. frame) intervals.
/// The model also caches file hashes as well as allows files to be linked to document GUIDs for fast lookups.
class XII_TOOLSFOUNDATION_DLL xiiFileSystemModel
{
  XII_DECLARE_SINGLETON(xiiFileSystemModel);

public:
  using FilesMap   = xiiMap<xiiDataDirPath, xiiFileStatus, xiiCompareDataDirPath>;
  using FoldersMap = xiiMap<xiiDataDirPath, xiiFileStatus::Status, xiiCompareDataDirPath>;

  using LockedFiles   = xiiLockedObject<xiiMutex, const FilesMap>;
  using LockedFolders = xiiLockedObject<xiiMutex, const FoldersMap>;

public:
  /// Return true if the two paths point to the same file on disk. On different platforms the same strings can produce different results. This function assumes both paths are absolute and cleaned via xiiStringBuilder::MakeCleanPath.
  static bool IsSameFile(const xiiStringView sAbsolutePathA, const xiiStringView sAbsolutePathB);

  /// Computes the hash of the given file. Optionally passes the data stream through into another stream writer.
  static xiiUInt64 HashFile(xiiStreamReader& ref_inputStream, xiiStreamWriter* pPassThroughStream);

public:
  /// \name Setup
  ///@{

  xiiFileSystemModel();
  ~xiiFileSystemModel();

  /// Initializes the model for the given file system config.
  /// \param fileSystemConfig All data directories in this config will be tracked by the model.
  /// \param referencedFiles Restores the previous state of the file model. E.g. cached on disk. If the xiiFileStatus::Status is xiiFileStatus::Status::Unknown m_FileChangedEvents is guaranteed to be fired once the file is checked again, e.g. via CheckFileSystem or NotifyOfChange.
  /// \param referencedFolders Restores the previous state of the folder model. E.g. cached on disk.
  void Initialize(const xiiApplicationFileSystemConfig& fileSystemConfig, FilesMap&& referencedFiles, FoldersMap&& referencedFolders);

  /// Deinitialize the model.
  /// \param out_pReferencedFiles If set, filled with the current state of the file model so it can be cached, e.g. by storing it on disk.
  /// \param out_pReferencedFolders If set, filled with the current state of the folder model so it can be cached, e.g. by storing it on disk.
  void Deinitialize(FilesMap* out_pReferencedFiles = nullptr, FoldersMap* out_pReferencedFolders = nullptr);

  /// Needs to be called every frame to restart background tasks.
  void MainThreadTick();

  const xiiApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  xiiArrayPtr<const xiiString>          GetDataDirectoryRoots() const { return m_DataDirRoots.GetArrayPtr(); }

  ///@}
  /// \name File / Folder Access
  ///@{

  /// Returns all files in the model.
  /// \return Returns the files and also a lock to the model.
  const LockedFiles GetFiles() const;

  /// Returns all folders in the model.
  /// \return Returns the folders and also a lock to the model.
  const LockedFolders GetFolders() const;

  /// Searches for a file in the model.
  /// \param sPath Absolute or relative path to a file to be searched for.
  /// \param stat Contains the current state of the file in the model if found.
  /// \return Returns XII_SUCCESS if the file was found.
  xiiResult FindFile(xiiStringView sPath, xiiFileStatus& out_stat) const;

  /// Searches for the first file in the model that satisfies the given visitor function.
  /// \param visitor Called for every file in the model. If this functions returns true, the search is canceled and the function returns XII_SUCCESS.
  /// \return Returns XII_SUCCESS if the visitor returned true for a file.
  xiiResult FindFile(xiiDelegate<bool(const xiiDataDirPath&, const xiiFileStatus&)> visitor) const;

  ///@}
  /// \name File / Folder Updates
  ///@{

  /// Force checking the filesystem for changes to the given file or folder.
  /// This function will handle file add/remove/change as well as folder add/remove. If an existing folder should be checked for changes, use CheckFolder instead.
  /// \param sAbsolutePath File or folder to check for changes.
  void NotifyOfChange(xiiStringView sAbsolutePath);

  /// Check an existing folder recursively for changes.
  /// \param sAbsolutePath Absolute path to an existing folder in the model.
  void CheckFolder(xiiStringView sAbsolutePath);

  /// Updates all files and folders in the model by iterating over all data directories. This is very expensive and should be done on a worker thread.
  void CheckFileSystem();

  ///@}
  /// \name File Meta Operations
  ///@{

  /// Links a document Id to the given file. This allows for fast lookups whether a file is also a document.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param documentId The Id of the document that should be linked to the file.
  /// \return Returns XII_SUCCESS if the file existed in the model and could be linked.
  xiiResult LinkDocument(xiiStringView sAbsolutePath, const xiiUuid& documentId);

  /// Unlinks a document from a file
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \return Returns XII_SUCCESS if the file existed.
  xiiResult UnlinkDocument(xiiStringView sAbsolutePath);

  /// Creates a file reader to the given file. Will also link the document and hash it in a file-system-atomic operation.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param callback Called once the file was opened and hashed. The xiiFileStatus contains the up to date info for the file, including hash.
  /// \return Returns XII_SUCCESS if the file existed and could be opened. Returns XII_FAILURE if the file is not in the model or the file can't be opened for read access. On read failure, the file will be marked as locked.
  xiiResult ReadDocument(xiiStringView sAbsolutePath, const xiiDelegate<void(const xiiFileStatus&, xiiStreamReader&)>& callback);

  /// Returns an up-to-date hash for the given file. Will trigger m_FileChangedEvents if the file has been modified since the last check. Hashes are cached so in the best case this will just check the timestamp on disk against the model and then return the cached hash. This function will also work on files outside of the data directories.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param out_stat Contains the up to date info for the file, including hash.
  /// \return Returns XII_SUCCESS if the file existed and could be opened. On failure, the file will be marked as locked.
  xiiResult HashFile(xiiStringView sAbsolutePath, xiiFileStatus& out_stat);

  ///@}

public:
  xiiCopyOnBroadcastEvent<const xiiFolderChangedEvent&, xiiMutex> m_FolderChangedEvents;
  xiiCopyOnBroadcastEvent<const xiiFileChangedEvent&, xiiMutex>   m_FileChangedEvents;

private:
  void SetAllStatusUnknown();
  void RemoveStaleFileInfos();

  void          OnAssetWatcherEvent(const xiiFileSystemWatcherEvent& e);
  xiiFileStatus HandleSingleFile(xiiDataDirPath absolutePath, bool bRecurseIntoFolders);
  xiiFileStatus HandleSingleFile(xiiDataDirPath absolutePath, const xiiFileStats& FileStat, bool bRecurseIntoFolders);

  void RemoveFileOrFolder(const xiiDataDirPath& absolutePath, bool bRecurseIntoFolders);

  void MarkFileLocked(xiiStringView sAbsolutePath);

  void FireFileChangedEvent(const xiiDataDirPath& file, xiiFileStatus fileStatus, xiiFileChangedEvent::Type type);
  void FireFolderChangedEvent(const xiiDataDirPath& file, xiiFolderChangedEvent::Type type);

private:
  // Immutable data after Initialize
  xiiApplicationFileSystemConfig     m_FileSystemConfig;
  xiiDynamicArray<xiiString>         m_DataDirRoots;
  xiiUniquePtr<xiiFileSystemWatcher> m_pWatcher;
  xiiEventSubscriptionID             m_WatcherSubscription = {};

  // Actual file system data
  mutable xiiMutex m_FilesMutex;
  xiiAtomicBool    m_bInitialized = false;

  FilesMap                         m_ReferencedFiles;   // Absolute path to stat map
  FoldersMap                       m_ReferencedFolders; // Absolute path to status map
  xiiSet<xiiString>                m_LockedFiles;
  xiiMap<xiiString, xiiFileStatus> m_TransiendFiles; // Absolute path to stat for files outside the data directories.
};

#endif
