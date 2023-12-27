#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>

class xiiExposedSceneProperty;
class xiiSceneDocumentSettingsBase;
class xiiPushObjectStateMsgToEditor;

struct GameMode
{
  enum Enum
  {
    Off,
    Simulate,
    Play,
  };
};

class XII_EDITORPLUGINSCENE_DLL xiiSceneDocument : public xiiGameObjectDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneDocument, xiiGameObjectDocument);

public:
  enum class DocumentType
  {
    Scene,
    Prefab,
    Layer
  };

public:
  xiiSceneDocument(xiiStringView sDocumentPath, DocumentType documentType);
  ~xiiSceneDocument();

  enum class ShowOrHide
  {
    Show,
    Hide
  };

  void GroupSelection();

  /// \brief Opens the Duplicate Special dialog
  void DuplicateSpecial();

  /// \brief Opens the 'Delta Transform' dialog.
  void DeltaTransform();


  /// \brief Moves all selected objects to the editor camera position
  void SnapObjectToCamera();


  /// \brief Attaches all selected objects to the selected object
  void AttachToObject();

  /// \brief Detaches all selected objects from their current parent
  void DetachFromParent();

  /// \brief Puts the GUID of the single selected object into the clipboard
  void CopyReference();

  /// \brief Creates a new empty object, either top-level (selection empty) or as a child of the selected item
  xiiStatus CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition);

  void DuplicateSelection();
  void ShowOrHideSelectedObjects(ShowOrHide action);
  void ShowOrHideAllObjects(ShowOrHide action);
  void HideUnselectedObjects();

  /// \brief Whether this document represents a prefab or a scene
  bool IsPrefab() const { return m_DocumentType == DocumentType::Prefab; }

  /// \brief Determines whether the given object is an editor prefab
  bool IsObjectEditorPrefab(const xiiUuid& object, xiiUuid* out_pPrefabAssetGuid = nullptr) const;

  /// \brief Determines whether the given object is an engine prefab
  bool IsObjectEnginePrefab(const xiiUuid& object, xiiUuid* out_pPrefabAssetGuid = nullptr) const;

  /// \brief Nested prefabs are not allowed
  virtual bool ArePrefabsAllowed() const override { return !IsPrefab(); }


  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_sMimeType) const override;
  virtual bool Paste(
    const xiiArrayPtr<PasteInfo>& info,
    const xiiAbstractObjectGraph& objectGraph,
    bool                          bAllowPickedPosition,
    xiiStringView                 sMimeType) override;
  bool DuplicateSelectedObjects(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bSetSelected);
  bool CopySelectedObjects(xiiAbstractObjectGraph& ref_graph, xiiMap<xiiUuid, xiiUuid>* out_pParents) const;
  bool PasteAt(const xiiArrayPtr<PasteInfo>& info, const xiiVec3& vPos);
  bool PasteAtOrignalPosition(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph);

  virtual void UpdatePrefabs() override;

  /// \brief Removes the link to the prefab template, making the editor prefab a simple object
  virtual void UnlinkPrefabs(const xiiDeque<const xiiDocumentObject*>& selection) override;

  virtual xiiUuid ReplaceByPrefab(
    const xiiDocumentObject* pRootObject,
    xiiStringView            sPrefabFile,
    const xiiUuid&           prefabAsset,
    const xiiUuid&           prefabSeed,
    bool                     bEnginePrefab) override;

  /// \brief Reverts all selected editor prefabs to their original template state
  virtual xiiUuid RevertPrefab(const xiiDocumentObject* pObject) override;

  /// \brief Converts all objects in the selection that are engine prefabs to their respective editor prefab representation
  virtual void ConvertToEditorPrefab(const xiiDeque<const xiiDocumentObject*>& selection);
  /// \brief Converts all objects in the selection that are editor prefabs to their respective engine prefab representation
  virtual void ConvertToEnginePrefab(const xiiDeque<const xiiDocumentObject*>& selection);

  virtual xiiStatus CreatePrefabDocumentFromSelection(xiiStringView sFile, const xiiRTTI* pRootType, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB = {}, xiiDelegate<void(xiiDocumentObject*)> adjustNewNodesCB = {}, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {}) override;

  GameMode::Enum GetGameMode() const { return m_GameMode; }

  virtual bool CanEngineProcessBeRestarted() const override;

  void StartSimulateWorld();
  void TriggerGameModePlay(bool bUsePickedPositionAsStart);

  /// Stops the world simulation, if it is running. Returns true, when the simulation needed to be stopped.
  bool StopGameMode();

  xiiTransformStatus ExportScene(bool bCreateThumbnail);
  void               ExportSceneGeometry(
                  const char*    szFile,
                  bool           bOnlySelection,
                  int            iExtractionMode /* xiiWorldGeoExtractionUtil::ExtractionMode */,
                  const xiiMat3& mTransform);

  virtual void HandleEngineMessage(const xiiEditorEngineDocumentMsg* pMsg) override;
  void         HandleGameModeMsg(const xiiGameModeMsgToEditor* pMsg);
  void         HandleObjectStateFromEngineMsg(const xiiPushObjectStateMsgToEditor* pMsg);

  void SendObjectMsg(const xiiDocumentObject* pObj, xiiObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const xiiDocumentObject* pObj, xiiObjectTagMsgToEngine* pMsg);

  /// \name Scene Settings
  ///@{

  virtual const xiiDocumentObject*    GetSettingsObject() const;
  const xiiSceneDocumentSettingsBase* GetSettingsBase() const;
  template <typename T>
  const T* GetSettings() const
  {
    return xiiDynamicCast<const T*>(GetSettingsBase());
  }

  xiiStatus CreateExposedProperty(
    const xiiDocumentObject*   pObject,
    const xiiAbstractProperty* pProperty,
    xiiVariant                 index,
    xiiExposedSceneProperty&   out_key) const;
  xiiStatus AddExposedParameter(const char* szName, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index);
  xiiInt32  FindExposedParameter(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index);
  xiiStatus RemoveExposedParameter(xiiInt32 iIndex);
  ///@}

  /// \name Editor Camera
  ///@{

  /// \brief Stores the current editor camera position in a user preference. Slot can be 0 to 9.
  ///
  /// Since the preference is stored on disk, this position can be restored in another session.
  void StoreFavoriteCamera(xiiUInt8 uiSlot);

  /// \brief Applies the previously stored camera position from slot 0 to 9 to the current camera position.
  ///
  /// The camera will quickly interpolate to the stored position.
  void RestoreFavoriteCamera(xiiUInt8 uiSlot);

  /// \brief Searches for an xiiCameraComponent with the 'EditorShortcut' property set to \a uiSlot and moves the editor camera to that position.
  xiiResult JumpToLevelCamera(xiiUInt8 uiSlot, bool bImmediate);

  /// \brief Creates an object with an xiiCameraComponent at the current editor camera position and sets the 'EditorShortcut' property to \a uiSlot.
  xiiResult CreateLevelCamera(xiiUInt8 uiSlot);

  virtual xiiManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return xiiManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

  ///@}

protected:
  void SetGameMode(GameMode::Enum mode);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void UpdatePrefabObject(xiiDocumentObject* pObject, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, xiiStringView sBasePrefab) override;
  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;

  template <typename Func>
  void ApplyRecursive(const xiiDocumentObject* pObject, Func f)
  {
    f(pObject);

    for (auto pChild : pObject->GetChildren())
    {
      ApplyRecursive<Func>(pChild, f);
    }
  }

protected:
  void EnsureSettingsObjectExist();
  void DocumentObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>::EventData& e);
  void EngineConnectionEventHandler(const xiiEditorEngineProcessConnection::Event& e);
  void ToolsProjectEventHandler(const xiiToolsProjectEvent& e);

  xiiStatus RequestExportScene(const char* szTargetFile, const xiiAssetFileHeader& header);

  virtual xiiTransformStatus InternalTransformAsset(const char* szTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  xiiTransformStatus         InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  void SyncObjectHiddenState();
  void SyncObjectHiddenState(xiiDocumentObject* pObject);

  /// \brief Finds all objects that are actively being 'debugged' (or visualized) by the editor and thus should get the debug visualization flag in
  /// the runtime.
  void UpdateObjectDebugTargets();

  DocumentType m_DocumentType = DocumentType::Scene;

  GameMode::Enum m_GameMode;

  GameModeData m_GameModeData[3];

  // Local mirror for settings
  xiiDocumentObjectMirror m_ObjectMirror;
  xiiRttiConverterContext m_Context;

  //////////////////////////////////////////////////////////////////////////
  /// Communication with other document types
  virtual void OnInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender) override;
  void         GatherObjectsOfType(xiiDocumentObject* pRoot, xiiGatherObjectsOfTypeMsgInterDoc* pMsg) const;
};
