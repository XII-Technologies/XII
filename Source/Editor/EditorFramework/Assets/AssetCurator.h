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

/// \brief Custom mutex that allows to profile the time in the curator lock.
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
    CircularDependency,
    COUNT,
  };

  xiiUInt8                     m_LastStateUpdate = 0; ///< Changes every time m_TransformState is modified. Used to detect stale computations done outside the lock.
  xiiAssetExistanceState::Enum m_ExistanceState  = xiiAssetExistanceState::FileAdded;
  TransformState               m_TransformState  = TransformState::Unknown;
  xiiUInt64                    m_AssetHash       = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  xiiUInt64                    m_ThumbHash       = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.

  xiiDynamicArray<xiiLogEntry> m_LogEntries;

  const xiiAssetDocumentTypeDescriptor* m_pDocumentTypeDescriptor = nullptr;
  xiiDataDirPath                        m_Path;

  xiiUniquePtr<xiiAssetDocumentInfo> m_Info;

  xiiSet<xiiString> m_MissingTransformDeps;
  xiiSet<xiiString> m_MissingThumbnailDeps;
  xiiSet<xiiString> m_CircularDependencies;

  xiiSet<xiiUuid> m_SubAssets; ///< Main asset uses the same GUID as this (see m_Info), but is NOT stored in m_SubAssets

private:
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAssetInfo);
};

/// \brief Information about an asset or sub-asset.
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

  /// \brief Starts init task. Need to call WaitForInitialize to finish before loading docs.
  void StartInitialize(const xiiApplicationFileSystemConfig& cfg);
  /// \brief Waits for init task to finish.
  void WaitForInitialize();
  void Deinitialize();

  void MainThreadTick(bool bTopLevel);

  ///@}
  /// \name Asset Platform Configurations
  ///@{

public:
  /// \brief The main platform on which development happens. E.g. "PC".
  ///
  /// TODO: review this concept
  const xiiPlatformProfile* GetDevelopmentAssetProfile() const;

  /// \brief The currently active target platform for asset processing.
  const xiiPlatformProfile* GetActiveAssetProfile() const;

  /// \brief Returns the index of the currently active asset platform configuration
  xiiUInt32 GetActiveAssetProfileIndex() const;

  /// \brief Returns xiiInvalidIndex if no config with the given name exists. Name comparison is case insensitive.
  xiiUInt32 FindAssetProfileByName(const char* szPlatform);

  xiiUInt32 GetNumAssetProfiles() const;

  /// \brief Always returns a valid config. E.g. even if xiiInvalidIndex is passed in, it will fall back to the default config (at index 0).
  const xiiPlatformProfile* GetAssetProfile(xiiUInt32 uiIndex) const;

  /// \brief Always returns a valid config. E.g. even if xiiInvalidIndex is passed in, it will fall back to the default config (at index 0).
  xiiPlatformProfile* GetAssetProfile(xiiUInt32 uiIndex);

  /// \brief Adds a new profile. The name should be set afterwards to a unique name.
  xiiPlatformProfile* CreateAssetProfile();

  /// \brief Deletes the given asset profile, if possible.
  ///
  /// The function fails when the given profile is the main profile (at index 0),
  /// or it is the currently active profile.
  xiiResult DeleteAssetProfile(xiiPlatformProfile* pProfile);

  /// \brief Switches the currently active asset target platform.
  ///
  /// Broadcasts xiiAssetCuratorEvent::Type::ActivePlatformChanged on change.
  void SetActiveAssetProfileByIndex(xiiUInt32 uiIndex, bool bForceReevaluation = false);

  /// \brief Saves the current asset configurations. Returns failure if the output file could not be written to.
  xiiResult SaveAssetProfiles();

  void SaveRuntimeProfiles();

private:
  void      ClearAssetProfiles();
  void      SetupDefaultAssetProfiles();
  xiiResult LoadAssetProfiles();
  void      ComputeAllDocumentManagerAssetProfileHashes();

  xiiHybridArray<xiiPlatformProfile*, 8> m_AssetProfiles;

  ///@}
  /// \name High Level Functions
  ///@{

public:
  xiiDateTime GetLastFullTransformDate() const;
  void        StoreFullTransformDate();

  /// \brief Transforms all assets and writes the lookup tables. If the given platform is empty, the active platform is used.
  xiiStatus          TransformAllAssets(xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile = nullptr);
  void               ResaveAllAssets();
  xiiTransformStatus TransformAsset(const xiiUuid& assetGuid, xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile = nullptr);
  xiiTransformStatus CreateThumbnail(const xiiUuid& assetGuid);

  /// Some assets are not automatically updated by the asset dependency detection (mainly Collections) because of their transitive data dependencies.
  /// So we must update them when the user does something 'significant' like doing TransformAllAssets or a scene export.
  void TransformAssetsForSceneExport(const xiiPlatformProfile* pAssetProfile = nullptr);

  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  xiiResult WriteAssetTables(const xiiPlatformProfile* pAssetProfile = nullptr, bool bForce = false);

  ///@}
  /// \name Asset Access
  ///@{
  using xiiLockedSubAsset = xiiLockedObject<xiiMutex, const xiiSubAsset>;

  /// \brief Tries to find the asset information for an asset identified through a string.
  ///
  /// The string may be a stringyfied asset GUID or a relative or absolute path. The function will try all possibilities.
  /// If no asset can be found, an empty/invalid xiiAssetInfo is returned.
  /// If bExhaustiveSearch is set the function will go through all known assets and find the closest match.
  const xiiLockedSubAsset FindSubAsset(xiiStringView sPathOrGuid, bool bExhaustiveSearch = false) const;

  /// \brief Same as GetAssteInfo, but wraps the return value into a xiiLockedSubAsset struct
  const xiiLockedSubAsset GetSubAsset(const xiiUuid& assetGuid) const;

  using xiiLockedSubAssetTable = xiiLockedObject<xiiMutex, const xiiHashTable<xiiUuid, xiiSubAsset>>;

  /// \brief Returns the table of all known assets in a locked structure
  const xiiLockedSubAssetTable GetKnownSubAssets() const;

  using xiiLockedAssetTable = xiiLockedObject<xiiMutex, const xiiHashTable<xiiUuid, xiiAssetInfo*>>;

  /// \brief Returns the table of all known assets in a locked structure
  const xiiLockedAssetTable GetKnownAssets() const;

  /// \brief Computes the combined hash for the asset and its dependencies. Returns 0 if anything went wrong.
  xiiUInt64 GetAssetDependencyHash(xiiUuid assetGuid);

  /// \brief Computes the combined hash for the asset and its references. Returns 0 if anything went wrong.
  xiiUInt64 GetAssetReferenceHash(xiiUuid assetGuid);

  xiiAssetInfo::TransformState IsAssetUpToDate(const xiiUuid& assetGuid, const xiiPlatformProfile* pAssetProfile, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor, xiiUInt64& out_uiAssetHash, xiiUInt64& out_uiThumbHash, bool bForce = false);
  /// \brief Returns the number of assets in the system and how many are in what transform state
  void GetAssetTransformStats(xiiUInt32& out_uiNumAssets, xiiHybridArray<xiiUInt32, xiiAssetInfo::TransformState::COUNT>& out_count);

  /// \brief Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  xiiString FindDataDirectoryForAsset(xiiStringView sAbsoluteAssetPath) const;

  /// \brief Uses knowledge about all existing files on disk to find the best match for a file. Very slow.
  ///
  /// \param sFile
  ///   File name (may include a path) to search for. Will be modified both on success and failure to give a 'reasonable' result.
  xiiResult FindBestMatchForFile(xiiStringBuilder& ref_sFile, xiiArrayPtr<xiiString> allowedFileExtensions) const;

  /// \brief Finds all uses, either as references or dependencies to a given asset.
  ///
  /// Technically this finds all references and dependencies to this asset but in practice there are no uses of transform dependencies between assets right now so the result is a list of references and can be referred to as such.
  ///
  /// \param assetGuid
  ///   The asset to find use cases for.
  /// \param uses
  ///   List of assets that use 'assetGuid'. Any previous content of the set is not removed.
  /// \param transitive
  ///   If set, will also find indirect uses of the asset.
  void FindAllUses(xiiUuid assetGuid, xiiSet<xiiUuid>& ref_uses, bool bTransitive) const;

  /// \brief Returns all assets that use a file for transform. Use this to e.g. figure which assets still reference a .tga file in the project.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \param ref_uses List of assets that use 'sAbsolutePath'. Any previous content of the set is not removed.
  void FindAllUses(xiiStringView sAbsolutePath, xiiSet<xiiUuid>& ref_uses) const;

  /// \brief Returns whether a file is referenced, i.e. used for transforming an asset. Use this to e.g. figure out whether a .tga file is still in use by any asset.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \return True, if at least one asset references the given file.
  bool IsReferenced(xiiStringView sAbsolutePath) const;


  ///@}
  /// \name Manual and Automatic Change Notification
  ///@{

  /// \brief Allows to tell the system of a new or changed file, that might be of interest to the Curator.
  void NotifyOfFileChange(xiiStringView sAbsolutePath);
  /// \brief Allows to tell the system to re-evaluate an assets status.
  void NotifyOfAssetChange(const xiiUuid& assetGuid);
  void UpdateAssetLastAccessTime(const xiiUuid& assetGuid);

  /// \brief Checks file system for any changes. Call in case the file system watcher does not pick up certain changes.
  void CheckFileSystem();

  void NeedsReloadResources(const xiiUuid& assetGuid);

  void InvalidateAssetsWithTransformState(xiiAssetInfo::TransformState state);


  ///@}

  /// \name Utilities
  ///@{

  /// \brief Generates one transitive hull for all the dependencies that are enabled. The set will contain dependencies that are reachable via any combination of enabled reference types.
  void GenerateTransitiveHull(const xiiStringView sAssetOrPath, xiiSet<xiiString>& inout_deps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false, bool bIncludePackageDeps = false) const;

  /// \brief Generates one inverse transitive hull for all the types dependencies that are enabled. The set will contain inverse dependencies that can reach the given asset (pAssetInfo) via any combination of the enabled reference types. As only assets can have dependencies, the inverse hull is always just asset GUIDs.
  void GenerateInverseTransitiveHull(const xiiAssetInfo* pAssetInfo, xiiSet<xiiUuid>& inout_inverseDeps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false) const;

  /// \brief Generates a DGML graph of all transform and thumbnail dependencies.
  void WriteDependencyDGML(const xiiUuid& guid, xiiStringView sOutputFile) const;

  ///@}

public:
  xiiEvent<const xiiAssetCuratorEvent&> m_Events;

private:
  /// \name Processing
  ///@{

  xiiTransformStatus ProcessAsset(xiiAssetInfo* pAssetInfo, const xiiPlatformProfile* pAssetProfile, xiiBitflags<xiiTransformFlags> transformFlags);
  xiiStatus          ResaveAsset(xiiAssetInfo* pAssetInfo);
  /// \brief Returns the asset info for the asset with the given GUID or nullptr if no such asset exists.
  xiiAssetInfo*       GetAssetInfo(const xiiUuid& assetGuid);
  const xiiAssetInfo* GetAssetInfo(const xiiUuid& assetGuid) const;

  xiiSubAsset* GetSubAssetInternal(const xiiUuid& assetGuid);

  /// \brief Returns the asset info for the asset with the given (stringyfied) GUID or nullptr if no such asset exists.
  xiiAssetInfo* GetAssetInfo(const xiiString& sAssetGuid);

  void OnFileChangedEvent(const xiiFileChangedEvent& e);

  /// \brief Some assets are vital for the engine to run. Each data directory can contain a [DataDirName].xiiCollectionAsset
  ///   that has all its references transformed before any other documents are loaded.
  void ProcessAllCoreAssets();

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

  xiiAssetInfo::TransformState HashAsset(
    xiiUInt64                            uiSettingsHash,
    const xiiHybridArray<xiiString, 16>& assetTransformDeps,
    const xiiHybridArray<xiiString, 16>& assetThumbnailDeps,
    xiiSet<xiiString>&                   missingTransformDeps,
    xiiSet<xiiString>&                   missingThumbnailDeps,
    xiiUInt64&                           out_AssetHash,
    xiiUInt64&                           out_ThumbHash,
    bool                                 bForce);
  bool AddAssetHash(xiiString& sPath, bool bIsReference, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, bool bForce);

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

  xiiAssetInfo::TransformState UpdateAssetTransformState(xiiUuid assetGuid, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, bool bForce);
  void                         UpdateAssetTransformState(const xiiUuid& assetGuid, xiiAssetInfo::TransformState state);
  void                         UpdateAssetTransformLog(const xiiUuid& assetGuid, xiiDynamicArray<xiiLogEntry>& logEntries);
  void                         SetAssetExistanceState(xiiAssetInfo& assetInfo, xiiAssetExistanceState::Enum state);

  ///@}
  /// \name Check File System Helper
  ///@{
  void        SetAllAssetStatusUnknown();
  void        LoadCaches(xiiMap<xiiDataDirPath, xiiFileStatus, xiiCompareDataDirPath>& out_referencedFiles, xiiMap<xiiDataDirPath, xiiFileStatus::Status, xiiCompareDataDirPath>& out_referencedFolders);
  void        SaveCaches(const xiiMap<xiiDataDirPath, xiiFileStatus, xiiCompareDataDirPath>& referencedFiles, const xiiMap<xiiDataDirPath, xiiFileStatus::Status, xiiCompareDataDirPath>& referencedFolders);
  static void BuildFileExtensionSet(xiiSet<xiiString>& AllExtensions);

  ///@}
  /// \name Utilities
  ///@{

public:
  /// \brief Deletes all files in all asset caches, except for the asset outputs that exceed the threshold.
  ///
  /// -> OutputReliability::Perfect -> deletes everything
  /// -> OutputReliability::Good -> keeps the 'Perfect' files
  /// -> OutputReliability::Unknown -> keeps the 'Good' and 'Perfect' files
  void ClearAssetCaches(xiiAssetDocumentManager::OutputReliability threshold);

  ///@}

private:
  friend class xiiUpdateTask;
  friend class xiiAssetProcessor;
  friend class xiiProcessTask;

  mutable xiiCuratorMutex m_CuratorMutex; // Global lock
  xiiTaskGroupID          m_InitializeCuratorTaskID;

  xiiUInt32 m_uiActiveAssetProfile = 0;

  // Actual data stored in the curator
  xiiHashTable<xiiUuid, xiiAssetInfo*> m_KnownAssets;
  xiiHashTable<xiiUuid, xiiSubAsset>   m_KnownSubAssets;

  // Derived dependency lookup tables
  xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>> m_InverseTransformDeps;    // [Absolute path -> asset Guid]
  xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>> m_InverseThumbnailDeps;    // [Absolute path -> asset Guid]
  xiiSet<std::tuple<xiiUuid, xiiUuid>>          m_UnresolvedTransformDeps; ///< If a dependency wasn't known yet when an asset info was loaded, it is put in here.
  xiiSet<std::tuple<xiiUuid, xiiUuid>>          m_UnresolvedThumbnailDeps;

  // State caches
  xiiHashSet<xiiUuid> m_TransformState[xiiAssetInfo::TransformState::COUNT];
  xiiHashSet<xiiUuid> m_SubAssetChanged; ///< Flushed in main thread tick
  xiiHashSet<xiiUuid> m_TransformStateStale;
  xiiHashSet<xiiUuid> m_Updating;

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
