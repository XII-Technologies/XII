/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiEditorEngineConnection;
class xiiEditorEngineSyncObject;
class xiiAssetDocumentManager;
class xiiPlatformProfile;
class QImage;

/// Describes whether the asset document on the editor side also needs a rendering context on the engine side
enum class xiiAssetDocEngineConnection : xiiUInt8
{
  None,               ///< Use this when the document is fully self-contained and any UI is handled by Qt only. This is very common for 'data only' assets and everything that can't be visualized in 3D.
  Simple,             ///< Use this when the asset should be visualized in 3D. This requires a 'context' to be set up on the engine side that implements custom rendering. This is the most common type for anything that can be visualized in 3D, though can also be used for 2D data.
  FullObjectMirroring ///< In this mode the entire object hierarchy on the editor side is automatically synchronized over to an engine context. This is only needed for complex documents, such as scenes and prefabs.
};

/// Frequently needed asset document states, to prevent code duplication
struct xiiCommonAssetUiState
{
  enum Enum : xiiUInt32
  {
    Pause           = XII_BIT(0),
    Restart         = XII_BIT(1),
    Loop            = XII_BIT(2),
    SimulationSpeed = XII_BIT(3),
    Grid            = XII_BIT(4),
    Visualizers     = XII_BIT(5),
  };

  Enum   m_State;
  double m_fValue = 0;
};

class XII_EDITORFRAMEWORK_DLL xiiAssetDocument : public xiiDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetDocument, xiiDocument);

public:
  /// The thumbnail info containing the hash of the file is appended to assets.
  /// The serialized size of this class can't change since it is found by seeking to the end of the file.
  class XII_EDITORFRAMEWORK_DLL ThumbnailInfo
  {
  public:
    xiiResult Deserialize(xiiStreamReader& inout_reader);
    xiiResult Serialize(xiiStreamWriter& inout_writer) const;

    /// Checks whether the stored file contains the same hash.
    bool IsThumbnailUpToDate(xiiUInt64 uiExpectedHash, xiiUInt16 uiVersion) const { return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion); }

    /// Sets the asset file hash
    void SetFileHashAndVersion(xiiUInt64 uiHash, xiiUInt16 v)
    {
      m_uiHash    = uiHash;
      m_uiVersion = v;
    }

    /// Returns the serialized size of the thumbnail info.
    /// Used to seek to the end of the file and find the thumbnail info struct.
    constexpr xiiUInt32 GetSerializedSize() const { return 19; }

  private:
    xiiUInt64 m_uiHash     = 0;
    xiiUInt16 m_uiVersion  = 0;
    xiiUInt16 m_uiReserved = 0;
  };

  xiiAssetDocument(xiiStringView sDocumentPath, xiiDocumentObjectManager* pObjectManager, xiiAssetDocEngineConnection engineConnectionType);
  ~xiiAssetDocument();

  /// \name Asset Functions
  ///@{

  xiiAssetDocumentManager*    GetAssetDocumentManager() const;
  const xiiAssetDocumentInfo* GetAssetDocumentInfo() const;

  xiiBitflags<xiiAssetDocumentFlags> GetAssetFlags() const;

  const xiiAssetDocumentTypeDescriptor* GetAssetDocumentTypeDescriptor() const
  {
    return static_cast<const xiiAssetDocumentTypeDescriptor*>(GetDocumentTypeDescriptor());
  }

  /// Transforms an asset.
  ///   Typically not called manually but by the curator which takes care of dependencies first.
  ///
  /// If xiiTransformFlags::ForceTransform is set, it will try to transform the asset, ignoring whether the transform is up to date.
  /// If xiiTransformFlags::TriggeredManually is set, transform produced changes will be saved back to the document.
  /// If xiiTransformFlags::BackgroundProcessing is set and transforming the asset would require re-saving it, nothing is done.
  xiiTransformStatus TransformAsset(xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile = nullptr);

  /// Updates the thumbnail of the asset.
  ///   Should never be called manually. Called only by the curator which takes care of dependencies first.
  xiiTransformStatus CreateThumbnail();

  /// Returns the RTTI type version of this asset document type. E.g. when the algorithm to transform an asset changes,
  /// Increase the RTTI version. This will ensure that assets get re-transformed, even though their settings and dependencies might not have changed.
  xiiUInt16 GetAssetTypeVersion() const;

  ///@}
  /// \name IPC Functions
  ///@{

  enum class EngineStatus
  {
    Unsupported,  ///< This document does not have engine IPC.
    Disconnected, ///< Engine process crashed or not started yet.
    Initializing, ///< Document is being initialized on the engine process side.
    Loaded,       ///< Any message sent after this state is reached will work on a fully loaded document.
  };

  /// Returns the current state of the engine process side of this document.
  EngineStatus GetEngineStatus() const { return m_EngineStatus; }

  /// Waits for GetEngineStatus to return Loaded or returns a failure reason.
  xiiStatus WaitForEngineStatusLoaded() const;

  /// Passed into xiiEngineProcessDocumentContext::Initialize on the engine process side. Allows the document to provide additional data to the engine process during context creation.
  virtual xiiVariant GetCreateEngineMetaData() const { return xiiVariant(); }

  /// Sends a message to the corresponding xiiEngineProcessDocumentContext on the engine process.
  bool SendMessageToEngine(xiiEditorEngineDocumentMsg* pMessage) const;

  /// Handles all messages received from the corresponding xiiEngineProcessDocumentContext on the engine process.
  virtual void HandleEngineMessage(const xiiEditorEngineDocumentMsg* pMsg);

  /// Returns the xiiEditorEngineConnection for this document.
  xiiEditorEngineConnection* GetEditorEngineConnection() const { return m_pEngineConnection; }

  /// Registers a sync object for this document. It will be mirrored to the xiiEngineProcessDocumentContext on the engine process.
  void AddSyncObject(xiiEditorEngineSyncObject* pSync) const;

  /// Removes a previously registered sync object. It will be removed on the engine process side.
  void RemoveSyncObject(xiiEditorEngineSyncObject* pSync) const;

  /// Returns the sync object registered under the given guid.
  xiiEditorEngineSyncObject* FindSyncObject(const xiiUuid& guid) const;

  /// Returns the first sync object registered with the given type.
  xiiEditorEngineSyncObject* FindSyncObject(const xiiRTTI* pType) const;

  /// Sends messages to sync all sync objects to the engine process side.
  void SyncObjectsToEngine() const;

  /// Sends a message that the document has been opened or closed. Resends all document data.
  ///
  /// Calling this will always clear the existing document on the engine side and reset the state to the editor state.
  void SendDocumentOpenMessage(bool bOpen);


  ///@}

  xiiEvent<const xiiEditorEngineDocumentMsg*> m_ProcessMessageEvent;

protected:
  void EngineConnectionEventHandler(const xiiEditorEngineProcessConnection::Event& e);

  /// \name Hash Functions
  ///@{

  /// Computes the hash from all document objects
  xiiUInt64 GetDocumentHash() const;

  /// Computes the hash for one document object and combines it with the given hash
  void GetChildHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const;

  /// Computes the hash for transform relevant meta data of the given document object and combines it with the given hash.
  virtual void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const {}

  ///@}
  /// \name Reimplemented Base Functions
  ///@{

  /// Overrides the base function to call UpdateAssetDocumentInfo() to update the settings hash
  virtual xiiTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override;

  /// Implements auto transform on save
  virtual void InternalAfterSaveDocument() override;

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;

  ///@}
  /// \name Asset Functions
  ///@{

  /// Override this to add custom data (e.g. additional file dependencies) to the info struct.
  ///
  /// \note ALWAYS call the base function! It automatically fills out references that it can determine.
  ///       In most cases that is already sufficient.
  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const;

  /// Override this and write the transformed file for the given szOutputTag into the given stream.
  ///
  /// The stream already contains the xiiAssetFileHeader. This is the function to prefer when the asset can be written
  /// directly from the editor process. AssetHeader is already written to the stream, but provided as reference.
  ///
  /// \param stream Data stream to write the asset to.
  /// \param szOutputTag Either empty for the default output or matches one of the tags defined in xiiAssetDocumentInfo::m_Outputs.
  /// \param szPlatform Platform for which is the output is to be created. Default is 'Default'.
  /// \param AssetHeader Header already written to the stream, provided for reference.
  /// \param transformFlags flags that affect the transform process, see xiiTransformFlags.
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) = 0;

  /// Only override this function, if the transformed file for the given szOutputTag must be written from another process.
  ///
  /// szTargetFile is where the transformed asset should be written to. The overriding function must ensure to first
  /// write \a AssetHeader to the file, to make it a valid asset file or provide a custom xiiAssetDocumentManager::IsOutputUpToDate function.
  /// See xiiTransformFlags for definition of transform flags.
  virtual xiiTransformStatus InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags);

  xiiStatus RemoteExport(const xiiAssetFileHeader& header, xiiStringView sOutputTarget) const;

  ///@}
  /// \name Thumbnail Functions
  ///@{

  /// Override this function to generate a thumbnail. Only called if GetAssetFlags returns xiiAssetDocumentFlags::SupportsThumbnail.
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo);

  /// Returns the full path to the jpg file in which the thumbnail for this asset is supposed to be
  xiiString GetThumbnailFilePath(xiiStringView sSubAssetName = xiiStringView()) const;

  /// Should be called after manually changing the thumbnail, such that the system will reload it
  void InvalidateAssetThumbnail(xiiStringView sSubAssetName = xiiStringView()) const;

  /// Requests the engine side to render a thumbnail, will call SaveThumbnail on success.
  xiiStatus RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo, xiiArrayPtr<xiiStringView> viewExclusionTags /*= xiiStringView("SkyLight")*/) const;
  xiiStatus RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo) const
  {
    xiiStringView defVal("SkyLight");
    return RemoteCreateThumbnail(thumbnailInfo, {&defVal, 1});
  }

  /// Saves the given image as the new thumbnail for the asset
  xiiStatus SaveThumbnail(const xiiImage& img, const ThumbnailInfo& thumbnailInfo) const;

  /// Saves the given image as the new thumbnail for the asset
  xiiStatus SaveThumbnail(const QImage& img, const ThumbnailInfo& thumbnailInfo) const;

  /// Appends an asset header containing the thumbnail hash to the file. Each thumbnail is appended by it to check up-to-date state.
  void AppendThumbnailInfo(xiiStringView sThumbnailFile, const ThumbnailInfo& thumbnailInfo) const;

  ///@}
  /// \name Common Asset States
  ///@{

public:
  /// Override this to handle a change to a common asset state differently.
  ///
  /// By default an on-off flag for every state is tracked, but nothing else.
  /// Also this automatically broadcasts the m_CommonAssetUiChangeEvent event.
  virtual void SetCommonAssetUiState(xiiCommonAssetUiState::Enum state, double value);

  /// Override this to return custom values for a common asset state.
  virtual double GetCommonAssetUiState(xiiCommonAssetUiState::Enum state) const;

  /// Used to broadcast state change events for common asset states.
  xiiEvent<const xiiCommonAssetUiState&> m_CommonAssetUiChangeEvent;

protected:
  xiiUInt32 m_uiCommonAssetStateFlags = 0;

  ///@}

protected:
  /// Adds all prefab dependencies to the xiiAssetDocumentInfo object. Called automatically by UpdateAssetDocumentInfo()
  void AddPrefabDependencies(const xiiDocumentObject* pObject, xiiAssetDocumentInfo* pInfo) const;

  /// Crawls through all asset properties of pObject and adds all string properties that have a xiiAssetBrowserAttribute as a dependency to
  /// pInfo. Automatically called by UpdateAssetDocumentInfo()
  void AddReferences(const xiiDocumentObject* pObject, xiiAssetDocumentInfo* pInfo, bool bInsidePrefab) const;

protected:
  xiiUniquePtr<xiiIPCObjectMirrorEditor> m_pMirror;

  virtual xiiDocumentInfo* CreateDocumentInfo() override;

  xiiTransformStatus DoTransformAsset(const xiiPlatformProfile* pAssetProfile, xiiBitflags<xiiTransformFlags> transformFlags);

  EngineStatus                m_EngineStatus;
  xiiAssetDocEngineConnection m_EngineConnectionType = xiiAssetDocEngineConnection::None;

  xiiEditorEngineConnection* m_pEngineConnection;

  mutable xiiHashTable<xiiUuid, xiiEditorEngineSyncObject*> m_AllSyncObjects;
  mutable xiiDeque<xiiEditorEngineSyncObject*>              m_SyncObjects;

  mutable xiiHybridArray<xiiUuid, 32> m_DeletedObjects;
};
