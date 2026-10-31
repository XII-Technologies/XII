/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>
#include <ToolsFoundation/FileSystem/Declarations.h>

#include <atomic>
#include <tuple>

class xiiUpdateTask;
class xiiTask;
class xiiAssetDocumentManager;
class xiiDirectoryWatcher;
class xiiProcessTask;
struct xiiFileStats;
class xiiAssetProcessorLog;
class xiiFileSystemWatcher;
class xiiAssetTableWriter;
struct xiiFileChangedEvent;
class xiiFileSystemModel;


#if 0 // Define to enable extensive curator profile scopes
#  define CURATOR_PROFILE(szName) XII_PROFILE_SCOPE(szName)

#else
#  define CURATOR_PROFILE(Name)

#endif

/// Custom mutex that allows to profile the time in the curator lock.
class xiiCuratorMutex : public xiiMutex
{
public:
  void Lock()
  {
    CURATOR_PROFILE("xiiCuratorMutex");
    xiiMutex::Lock();
  }

  void Unlock() { xiiMutex::Unlock(); }
};

struct XII_EDITORFRAMEWORK_DLL xiiAssetInfo
{
  xiiAssetInfo() = default;
  void Update(xiiUniquePtr<xiiAssetInfo>& rhs);

  xiiAssetDocumentManager* GetManager() { return static_cast<xiiAssetDocumentManager*>(m_pDocumentTypeDescriptor->m_pManager); }

  enum TransformState : xiiUInt8
  {
    Unknown = 0,
    UpToDate,
    NeedsImport,
    NeedsTransform,
    NeedsThumbnail,
    TransformError,
    MissingTransformDependency,
    MissingThumbnailDependency,
    MissingPackageDependency,
    CircularDependency,
    COUNT,
  };

  xiiUInt64                    m_LastStateUpdate = 0; ///< Changes every time m_TransformState is modified. Used to detect stale computations done outside the lock.
  xiiAssetExistanceState::Enum m_ExistanceState  = xiiAssetExistanceState::FileAdded;
  TransformState               m_TransformState  = TransformState::Unknown;
  xiiUInt64                    m_AssetHash       = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  xiiUInt64                    m_ThumbHash       = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  xiiUInt64                    m_PackageHash     = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.

  xiiDynamicArray<xiiLogEntry> m_LogEntries;

  const xiiAssetDocumentTypeDescriptor* m_pDocumentTypeDescriptor = nullptr;
  xiiDataDirPath                        m_Path;

  xiiUniquePtr<xiiAssetDocumentInfo> m_Info;

  xiiSet<xiiString> m_MissingTransformDeps;
  xiiSet<xiiString> m_MissingThumbnailDeps;
  xiiSet<xiiString> m_MissingPackageDeps;
  xiiSet<xiiString> m_CircularDependencies;

  xiiSet<xiiUuid> m_SubAssets; ///< Main asset uses the same GUID as this (see m_Info), but is NOT stored in m_SubAssets

private:
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAssetInfo);
};

/// Information about an asset or sub-asset.
struct XII_EDITORFRAMEWORK_DLL xiiSubAsset
{
  xiiStringView GetName() const;
  void          GetSubAssetIdentifier(xiiStringBuilder& out_sPath) const;

  xiiAssetExistanceState::Enum m_ExistanceState = xiiAssetExistanceState::FileAdded;
  xiiAssetInfo*                m_pAssetInfo     = nullptr;
  xiiTime                      m_LastAccess;
  bool                         m_bMainAsset = true;

  xiiSubAssetData m_Data;
};



struct xiiAssetCuratorEvent
{
  enum class Type
  {
    AssetAdded,
    AssetRemoved,
    AssetMoved,
    AssetUpdated,
    AssetListReset,
    ActivePlatformChanged,
    FileSystemCheckStarted,
  };

  xiiUuid            m_AssetGuid;
  const xiiSubAsset* m_pInfo;
  Type               m_Type;
};

class XII_EDITORFRAMEWORK_DLL xiiAssetCurator
{
  XII_DECLARE_SINGLETON(xiiAssetCurator);

public:
  xiiAssetCurator();
  ~xiiAssetCurator();

  /// \name Setup
  ///@{

  /// Starts init task. Need to call WaitForInitialize to finish before loading docs.
  xiiStatus StartInitialize(const xiiApplicationFileSystemConfig& cfg);
  /// Waits for init task to finish.
  xiiStatus WaitForInitialize();
  /// True after the initial asset scan. Independent of rendering-process readiness.
  bool IsReady() const { return m_bReady.load(); }
  void Deinitialize();

  void MainThreadTick(bool bTopLevel);

  ///@}
  /// \name Asset Platform Configurations
  ///@{

public:
  /// The main platform on which development happens. E.g. "Default".
  ///
  /// TODO: review this concept
  const xiiPlatformProfile* GetDevelopmentAssetProfile() const;

  /// The currently active target platform for asset processing.
  const xiiPlatformProfile* GetActiveAssetProfile() const;

  /// Returns the index of the currently active asset platform configuration
  xiiUInt32 GetActiveAssetProfileIndex() const;

  /// Returns xiiInvalidIndex if no config with the given name exists. Name comparison is case insensitive.
  xiiUInt32 FindAssetProfileByName(const char* szPlatform);

  xiiUInt32 GetNumAssetProfiles() const;

  /// Always returns a valid config. E.g. even if xiiInvalidIndex is passed in, it will fall back to the default config (at index 0).
  const xiiPlatformProfile* GetAssetProfile(xiiUInt32 uiIndex) const;

  /// Always returns a valid config. E.g. even if xiiInvalidIndex is passed in, it will fall back to the default config (at index 0).
  xiiPlatformProfile* GetAssetProfile(xiiUInt32 uiIndex);

  /// Adds a new profile. The name should be set afterwards to a unique name.
  xiiPlatformProfile* CreateAssetProfile();

  /// Deletes the given asset profile, if possible.
  ///
  /// The function fails when the given profile is the main profile (at index 0),
  /// or it is the currently active profile.
  xiiResult DeleteAssetProfile(xiiPlatformProfile* pProfile);

  /// Switches the currently active asset target platform.
  ///
  /// Broadcasts xiiAssetCuratorEvent::Type::ActivePlatformChanged on change.
  void SetActiveAssetProfileByIndex(xiiUInt32 uiIndex, bool bForceReevaluation = false);

  /// Saves the current asset configurations. Returns failure if the output file could not be written to.
  xiiResult SaveAssetProfiles();

  /// Reloads changed profile definitions from disk, preserving the active profile by name.
  /// Must be called between asset operations on the main thread. Stops background hashing before
  /// replacing profiles; all previously obtained profile pointers are invalidated on change.
  /// Invalid input leaves the current definitions and active profile unchanged.
  xiiResult ReloadAssetProfiles();

  /// Persists and publishes a complete profile collection in the supplied order. Index zero must
  /// be Default. The input is cloned; ownership is retained by the caller. Must be called on the
  /// main thread between asset operations. Stops processor jobs and hashing during publication.
  /// A failed file installation rolls back previously installed files and leaves live profiles
  /// unchanged. Rollback failures are reported with retained backup paths. This is not a crash-safe
  /// transaction across files. Successful publication invalidates all existing profile pointers.
  xiiStatus ApplyAssetProfiles(xiiArrayPtr<const xiiPlatformProfile* const> profiles, xiiUInt32 uiActiveProfile);

  /// Saves all runtime profile files, stopping at the first write failure.
  xiiResult SaveRuntimeProfiles();

private:
  void ClearAssetProfiles();
  void SetupDefaultAssetProfiles();
  void PublishAssetProfiles(xiiHybridArray<xiiPlatformProfile*, 8>& profiles, xiiUInt64 uiHash, xiiUInt32 uiActiveProfile);

  xiiHybridArray<xiiPlatformProfile*, 8> m_AssetProfiles;
  xiiUInt64                              m_uiLoadedAssetProfilesHash = 0;
  bool                                   m_bAssetProfilesLoaded      = false;

  ///@}
  /// \name High Level Functions
  ///@{

public:
  xiiDateTime GetLastFullTransformDate() const;
  void        StoreFullTransformDate();

  /// Transforms core collections after project readiness and before restoring user documents.
  xiiStatus TransformCoreAssets();

  /// Transforms all assets and writes lookup tables, using the active profile if none is given.
  xiiStatus          TransformAllAssets(xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile = nullptr);
  xiiTransformStatus TransformAsset(const xiiUuid& assetGuid, xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile = nullptr);
  xiiTransformStatus TransformAsset(xiiStringView sPathOrGuid, xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile = nullptr);
  bool               IsAssetBeingTransformed(const xiiUuid& assetGuid) const;
  xiiTransformStatus CreateThumbnail(const xiiUuid& assetGuid);

  /// Plans a deterministic dependency-first resave of the selected path subtree. Empty prefix selects
  /// all assets. Includes transform, thumbnail and package references; external dependencies are already
  /// supplied by their existing metadata. Cycles fail with an empty output and no document writes.
  xiiStatus GetAssetResaveOrder(xiiStringView sPrefixPath, xiiDynamicArray<xiiUuid>& out_order) const;
  /// Main-thread operation. Reports cancellation and save failures. Keeps existing documents open;
  /// opens and closes other documents through their managers. Does not hold the curator lock while saving.
  xiiStatus ResaveAllAssets(xiiStringView sPrefixPath);

  /// Some assets are not automatically updated by the asset dependency detection (mainly Collections) because of their transitive data dependencies.
  /// So we must update them when the user does something 'significant' like doing TransformAllAssets or a scene export.
  void TransformAssetsForSceneExport(const xiiPlatformProfile* pAssetProfile = nullptr);

  /// Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  xiiResult WriteAssetTables(const xiiPlatformProfile* pAssetProfile = nullptr, bool bForce = false);

  ///@}
  /// \name Asset Access
  ///@{
  using xiiLockedSubAsset = xiiLockedObject<xiiMutex, const xiiSubAsset>;

  /// Tries to find the asset information for an asset identified through a string.
  ///
  /// The string may be an asset GUID, absolute, rooted, data-directory-relative or parent-relative path,
  /// optionally followed by |SubAssetName. Ambiguous references fail.
  /// If no asset can be found, an empty/invalid xiiAssetInfo is returned.
  /// Exhaustive search permits a unique path suffix; it never chooses an arbitrary closest match.
  const xiiLockedSubAsset FindSubAsset(xiiStringView sPathOrGuid, bool bExhaustiveSearch = false) const;

  /// Resolves the same references as FindSubAsset, with an actionable failure diagnostic.
  /// Returns the persistent sub-asset identity (not its owner's GUID). Clears out_guid on failure.
  xiiStatus ResolveAsset(xiiStringView sPathOrGuid, xiiUuid& out_guid, bool bExhaustiveSearch = false) const;

  /// Same as GetAssteInfo, but wraps the return value into a xiiLockedSubAsset struct
  const xiiLockedSubAsset GetSubAsset(const xiiUuid& assetGuid) const;

  using xiiLockedSubAssetTable = xiiLockedObject<xiiMutex, const xiiHashTable<xiiUuid, xiiSubAsset>>;

  /// Returns the table of all known assets in a locked structure
  const xiiLockedSubAssetTable GetKnownSubAssets() const;

  using xiiLockedAssetTable = xiiLockedObject<xiiMutex, const xiiHashTable<xiiUuid, xiiAssetInfo*>>;

  /// Returns the table of all known assets in a locked structure
  const xiiLockedAssetTable GetKnownAssets() const;

  /// Computes the combined hash for the asset and its dependencies. Returns 0 if anything went wrong.
  xiiUInt64 GetAssetDependencyHash(xiiUuid assetGuid);

  /// Computes the combined hash for the asset and its references. Returns 0 if anything went wrong.
  xiiUInt64 GetAssetReferenceHash(xiiUuid assetGuid);

  /// Evaluates the requested profile, defaulting to the active profile. Non-active evaluations do not
  /// read or replace the active-profile state cache. The profile must remain alive and unchanged during evaluation.
  xiiAssetInfo::TransformState IsAssetUpToDate(const xiiUuid& assetGuid, const xiiPlatformProfile* pAssetProfile, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor, xiiUInt64& out_uiAssetHash, xiiUInt64& out_uiThumbHash, xiiUInt64& out_uiPackageHash, bool bForce = false);
  /// Returns the number of assets in the system and how many are in what transform state
  void GetAssetTransformStats(xiiUInt32& out_uiNumAssets, xiiHybridArray<xiiUInt32, xiiAssetInfo::TransformState::COUNT>& out_count);

  /// Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  xiiString FindDataDirectoryForAsset(xiiStringView sAbsoluteAssetPath) const;

  /// Uses knowledge about all existing files on disk to find the best match for a file. Very slow.
  ///
  /// \param sFile
  ///   File name (may include a path) to search for. Will be modified both on success and failure to give a 'reasonable' result.
  xiiResult FindBestMatchForFile(xiiStringBuilder& ref_sFile, xiiArrayPtr<xiiString> allowedFileExtensions) const;

  /// Finds all transform, thumbnail and package uses of an asset or sub-asset.
  ///
  /// \param assetGuid
  ///   The asset to find use cases for.
  /// \param uses
  ///   List of assets that use 'assetGuid'. Any previous content of the set is not removed.
  /// \param transitive
  ///   If set, will also find indirect uses of the asset.
  void FindAllUses(xiiUuid assetGuid, xiiSet<xiiUuid>& ref_uses, bool bTransitive) const;

  /// Returns all assets that use a file for transform. Use this to e.g. figure which assets still reference a .tga file in the project.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \param ref_uses List of assets that use 'sAbsolutePath'. Any previous content of the set is not removed.
  void FindAllUses(xiiStringView sPathOrGuid, xiiSet<xiiUuid>& ref_uses) const;

  /// Returns whether a file is referenced, i.e. used for transforming an asset. Use this to e.g. figure out whether a .tga file is still in use by any asset.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \return True, if at least one asset references the given file.
  bool IsReferenced(xiiStringView sPathOrGuid) const;


  ///@}
  /// \name Manual and Automatic Change Notification
  ///@{

  /// Allows to tell the system of a new or changed file, that might be of interest to the Curator.
  void NotifyOfFileChange(xiiStringView sAbsolutePath);
  /// Allows to tell the system to re-evaluate an assets status.
  void NotifyOfAssetChange(const xiiUuid& assetGuid);
  void UpdateAssetLastAccessTime(const xiiUuid& assetGuid);

  /// Checks file system for any changes. Call in case the file system watcher does not pick up certain changes.
  void CheckFileSystem();

  /// GUI-thread request for an interactive rescan. Repeated requests coalesce.
  /// The existing file-access task pool scans; the GUI tick publishes completion.
  void QueueCheckFileSystem();
  bool IsCheckingFileSystem() const { return m_bCheckingFileSystem; }

  void NeedsReloadResources(const xiiUuid& assetGuid);

  void InvalidateAssetsWithTransformState(xiiAssetInfo::TransformState state);


  ///@}

  /// \name Utilities
  ///@{

  /// Generates one transitive hull for all the dependencies that are enabled. The set will contain dependencies that are reachable via any combination of enabled reference types.
  /// Resolves asset paths and sub-assets to their owners for traversal. Unresolved references remain
  /// in the output as leaves. Existing output does not suppress traversal. Cycles are visited once per owner.
  void GenerateTransitiveHull(const xiiStringView sAssetOrPath, xiiSet<xiiString>& inout_deps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false, bool bIncludePackageDeps = false) const;

  /// Generates one inverse transitive hull for all the types dependencies that are enabled. The set will contain inverse dependencies that can reach the given asset (pAssetInfo) via any combination of the enabled reference types. As only assets can have dependencies, the inverse hull is always just asset GUIDs.
  void GenerateInverseTransitiveHull(const xiiAssetInfo* pAssetInfo, xiiSet<xiiUuid>& inout_inverseDeps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false, bool bIncludePackageDeps = false) const;

  /// Generates a DGML graph of all transform and thumbnail dependencies.
  /// Writes all dependency categories, retaining missing references as leaf nodes. Returns write failures.
  xiiResult WriteDependencyDGML(const xiiUuid& guid, xiiStringView sOutputFile) const;

  ///@}

public:
  xiiEvent<const xiiAssetCuratorEvent&> m_Events;

private:
  /// \name Processing
  ///@{

  xiiTransformStatus ProcessAsset(xiiAssetInfo* pAssetInfo, const xiiPlatformProfile* pAssetProfile, xiiBitflags<xiiTransformFlags> transformFlags);
  xiiStatus          ResaveAsset(const xiiUuid& assetGuid);
  /// Returns the asset info for the asset with the given GUID or nullptr if no such asset exists.
  xiiAssetInfo*       GetAssetInfo(const xiiUuid& assetGuid);
  const xiiAssetInfo* GetAssetInfo(const xiiUuid& assetGuid) const;

  xiiSubAsset* GetSubAssetInternal(const xiiUuid& assetGuid);

  /// Returns the asset info for the asset with the given (stringyfied) GUID or nullptr if no such asset exists.
  xiiAssetInfo* GetAssetInfo(const xiiString& sAssetGuid);

  void OnFileChangedEvent(const xiiFileChangedEvent& e);

  ///@}
  /// \name Update Task
  ///@{

  void RestartUpdateTask();
  void ShutdownUpdateTask();

  bool GetNextAssetToUpdate(xiiUuid& out_guid, xiiStringBuilder& out_sAbsPath);
  void OnUpdateTaskFinished(const xiiSharedPtr<xiiTask>& pTask);
  void RunNextUpdateTask();

  ///@}
  /// \name Asset Hashing and Status Updates (AssetUpdates.cpp)
  ///@{

  struct AssetHashResult
  {
    xiiAssetInfo::TransformState m_State           = xiiAssetInfo::Unknown;
    xiiUInt64                    m_uiAssetHash     = 0;
    xiiUInt64                    m_uiThumbnailHash = 0;
    xiiUInt64                    m_uiPackageHash   = 0;
  };
  struct AssetHashContext
  {
    // Shared only within one forced or non-active-profile evaluation, including parallel references.
    xiiHashTable<xiiUuid, AssetHashResult> m_Results;
    bool                                   m_bCacheActiveProfile = true;
  };
  bool                         AddAssetHash(xiiString& sPath, bool bIsReference, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, xiiUInt64& out_PackageHash, bool bForce, const xiiPlatformProfile* pAssetProfile, AssetHashContext* pContext);
  xiiAssetInfo::TransformState HashAsset(xiiUInt64 uiSettingsHash, const xiiHybridArray<xiiString, 16>& assetTransformDeps, const xiiHybridArray<xiiString, 16>& assetThumbnailDeps, const xiiHybridArray<xiiString, 16>& assetPackageDeps, xiiSet<xiiString>& missingTransformDeps, xiiSet<xiiString>& missingThumbnailDeps, xiiSet<xiiString>& missingPackageDeps, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, xiiUInt64& out_PackageHash, bool bForce, const xiiPlatformProfile* pAssetProfile, AssetHashContext* pContext);

  xiiResult EnsureAssetInfoUpdated(const xiiDataDirPath& absFilePath, const xiiFileStatus& stat, bool bForce = false);
  void      TrackDependencies(xiiAssetInfo* pAssetInfo);
  void      UntrackDependencies(xiiAssetInfo* pAssetInfo);
  xiiResult CheckForCircularDependencies(xiiAssetInfo* pAssetInfo);
  void      UpdateTrackedFiles(const xiiUuid& assetGuid, const xiiSet<xiiString>& files, xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>>& inverseTracker, xiiSet<std::tuple<xiiUuid, xiiUuid>>& unresolved, bool bAdd);
  void      UpdateUnresolvedTrackedFiles(xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>>& inverseTracker, xiiSet<std::tuple<xiiUuid, xiiUuid>>& unresolved);
  xiiResult ReadAssetDocumentInfo(const xiiDataDirPath& absFilePath, const xiiFileStatus& stat, xiiUniquePtr<xiiAssetInfo>& assetInfo);
  void      UpdateSubAssets(xiiAssetInfo& assetInfo);

  void RemoveAssetTransformState(const xiiUuid& assetGuid);
  void InvalidateAssetTransformState(const xiiUuid& assetGuid);

  xiiAssetInfo::TransformState UpdateAssetTransformState(xiiUuid assetGuid, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, xiiUInt64& out_PackageHash, bool bForce, const xiiPlatformProfile* pAssetProfile = nullptr, AssetHashContext* pContext = nullptr);
  void                         UpdateAssetTransformState(const xiiUuid& assetGuid, xiiAssetInfo::TransformState state);
  void                         UpdateAssetTransformLog(const xiiUuid& assetGuid, xiiDynamicArray<xiiLogEntry>& logEntries);
  void                         SetAssetExistanceState(xiiAssetInfo& assetInfo, xiiAssetExistanceState::Enum state);

  ///@}
  /// \name Check File System Helper
  ///@{
  void        SetAllAssetStatusUnknown();
  void        WaitForFileSystemCheck();
  void        LoadCaches(xiiMap<xiiDataDirPath, xiiFileStatus, xiiCompareDataDirPath>& out_referencedFiles, xiiMap<xiiDataDirPath, xiiFileStatus::Status, xiiCompareDataDirPath>& out_referencedFolders);
  void        SaveCaches(const xiiMap<xiiDataDirPath, xiiFileStatus, xiiCompareDataDirPath>& referencedFiles, const xiiMap<xiiDataDirPath, xiiFileStatus::Status, xiiCompareDataDirPath>& referencedFolders);
  static void BuildFileExtensionSet(xiiSet<xiiString>& AllExtensions);

  ///@}
  /// \name Utilities
  ///@{

public:
  /// Deletes all files in all asset caches, except for the asset outputs that exceed the threshold.
  ///
  /// -> OutputReliability::Perfect -> deletes everything
  /// -> OutputReliability::Good -> keeps the 'Perfect' files
  /// -> OutputReliability::Unknown -> keeps the 'Good' and 'Perfect' files
  void ClearAssetCaches(xiiAssetDocumentManager::OutputReliability threshold);

  ///@}

private:
  friend class xiiUpdateTask;
  friend class xiiAssetProcessor;
  friend class xiiAssetDocument;
  friend class xiiProcessTask;

  mutable xiiCuratorMutex m_CuratorMutex; // Global lock
  std::atomic<bool>       m_bReady       = false;
  bool                    m_bInitialized = false;
  xiiTaskGroupID          m_InitializeCuratorTaskID;
  xiiTaskGroupID          m_CheckFileSystemTaskID; ///< GUI-thread owned; joined before file-model teardown.
  std::atomic<bool>       m_bCheckingFileSystem = false;

  xiiUInt32 m_uiActiveAssetProfile = 0;

  // Actual data stored in the curator
  xiiHashTable<xiiUuid, xiiAssetInfo*> m_KnownAssets;
  xiiHashTable<xiiUuid, xiiSubAsset>   m_KnownSubAssets;

  // Derived dependency lookup tables
  xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>> m_InverseTransformDeps;    // [Absolute path -> asset Guid]
  xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>> m_InverseThumbnailDeps;    // [Absolute path -> asset Guid]
  xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>> m_InversePackageDeps;      // [Absolute path -> asset Guid]
  xiiSet<std::tuple<xiiUuid, xiiUuid>>          m_UnresolvedTransformDeps; ///< If a dependency wasn't known yet when an asset info was loaded, it is put in here.
  xiiSet<std::tuple<xiiUuid, xiiUuid>>          m_UnresolvedThumbnailDeps;
  xiiSet<std::tuple<xiiUuid, xiiUuid>>          m_UnresolvedPackageDeps;

  // State caches
  xiiHashSet<xiiUuid> m_TransformState[xiiAssetInfo::TransformState::COUNT];
  xiiHashSet<xiiUuid> m_SubAssetChanged; ///< Flushed in main thread tick
  xiiHashSet<xiiUuid> m_TransformStateStale;
  xiiHashSet<xiiUuid> m_Updating;
  bool                m_bResavingAssets = false;

  // Serialized cache
  mutable xiiCuratorMutex                               m_CachedAssetsMutex; ///< Only locks m_CachedAssets
  xiiMap<xiiString, xiiUniquePtr<xiiAssetDocumentInfo>> m_CachedAssets;
  xiiMap<xiiString, xiiFileStatus>                      m_CachedFiles;

  // Immutable data after StartInitialize
  xiiApplicationFileSystemConfig    m_FileSystemConfig;
  xiiUniquePtr<xiiAssetTableWriter> m_pAssetTableWriter;
  xiiSet<xiiString>                 m_ValidAssetExtensions;

  // Update task
  bool                        m_bRunUpdateTask = false;
  xiiSharedPtr<xiiUpdateTask> m_pUpdateTask;
  xiiTaskGroupID              m_UpdateTaskGroup;
};

class xiiUpdateTask final : public xiiTask
{
public:
  xiiUpdateTask(xiiOnTaskFinishedCallback onTaskFinished);
  ~xiiUpdateTask();

private:
  xiiStringBuilder m_sAssetPath;

  virtual void Execute() override;
};
