/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

  /// Creates a new object and attaches all currently selected objects to it.
  void GroupSelection();

  /// Changes the selection to the parent object.
  void SelectParentObject();

  /// Sets the last selected object as the 'active parent'.
  void SetSelectedAsActiveParent();
  /// Clears the 'active parent' object.
  void ClearActiveParent();

  /// Opens the Duplicate Special dialog
  void DuplicateSpecial();

  /// Opens the 'Delta Transform' dialog.
  void DeltaTransform();


  /// Moves all selected objects to the editor camera position
  void SnapObjectToCamera();


  /// Attaches all selected objects to the selected object
  void AttachToObject();

  /// Detaches all selected objects from their current parent
  void DetachFromParent();

  /// Puts the GUID of the single selected object into the clipboard
  void CopyReference();

  /// Creates a new empty object, either top-level (selection empty) or as a child of the selected item
  xiiStatus CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition, bool bComponentSelectionMenu);

  void DuplicateSelection();
  void ShowOrHideSelectedObjects(ShowOrHide action);
  void ShowOrHideAllObjects(ShowOrHide action);
  void HideUnselectedObjects();

  /// Whether this document represents a prefab or a scene
  bool IsPrefab() const { return m_DocumentType == DocumentType::Prefab; }

  /// Determines whether the given object is an editor prefab
  bool IsObjectEditorPrefab(const xiiUuid& object, xiiUuid* out_pPrefabAssetGuid = nullptr) const;

  /// Determines whether the given object is an engine prefab
  bool IsObjectEnginePrefab(const xiiUuid& object, xiiUuid* out_pPrefabAssetGuid = nullptr) const;

  /// Nested prefabs are not allowed
  virtual bool ArePrefabsAllowed() const override { return !IsPrefab(); }


  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_sMimeType) const override;
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType) override;
  bool         DuplicateSelectedObjects(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bSetSelected);
  bool         CopySelectedObjects(xiiAbstractObjectGraph& ref_graph, xiiMap<xiiUuid, xiiUuid>* out_pParents) const;
  bool         PasteAt(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, const xiiVec3& vPos);
  bool         PasteAtOrignalPosition(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph);

  virtual void UpdatePrefabs() override;

  /// Removes the link to the prefab template, making the editor prefab a simple object
  virtual void UnlinkPrefabs(xiiArrayPtr<const xiiDocumentObject*> selection) override;

  virtual xiiUuid ReplaceByPrefab(const xiiDocumentObject* pRootObject, xiiStringView sPrefabFile, const xiiUuid& prefabAsset, const xiiUuid& prefabSeed, bool bEnginePrefab) override;

  /// Reverts all selected editor prefabs to their original template state
  virtual xiiUuid RevertPrefab(const xiiDocumentObject* pObject) override;

  /// Converts all objects in the selection that are engine prefabs to their respective editor prefab representation
  virtual void ConvertToEditorPrefab(xiiArrayPtr<const xiiDocumentObject*> selection);
  /// Converts all objects in the selection that are editor prefabs to their respective engine prefab representation
  virtual void ConvertToEnginePrefab(xiiArrayPtr<const xiiDocumentObject*> selection);

  virtual xiiStatus CreatePrefabDocumentFromSelection(xiiStringView sFile, const xiiRTTI* pRootType, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB = {}, xiiDelegate<void(xiiDocumentObject*)> adjustNewNodesCB = {}, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {}) override;

  GameMode::Enum GetGameMode() const { return m_GameMode; }

  virtual bool CanEngineProcessBeRestarted() const override;

  void StartSimulateWorld();
  void TriggerGameModePlay(bool bUsePickedPositionAsStart);

  /// Stops the world simulation, if it is running. Returns true, when the simulation needed to be stopped.
  bool StopGameMode();

  void StepSimulation();
  void PauseSimulation();

  xiiTransformStatus ExportScene(bool bCreateThumbnail);
  void               ExportSceneGeometry(xiiStringView sFile, bool bOnlySelection, int iExtractionMode /* xiiWorldGeoExtractionUtil::ExtractionMode */, const xiiMat3& mTransform);

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

  xiiStatus CreateExposedProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index, xiiExposedSceneProperty& out_key) const;
  xiiStatus AddExposedParameter(xiiStringView sName, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index);
  xiiInt32  FindExposedParameter(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index);
  xiiStatus RemoveExposedParameter(xiiInt32 iIndex);
  ///@}

  /// \name Editor Camera
  ///@{

  /// Stores the current editor camera position in a user preference. Slot can be 0 to 9.
  ///
  /// Since the preference is stored on disk, this position can be restored in another session.
  void StoreFavoriteCamera(xiiUInt8 uiSlot);

  /// Applies the previously stored camera position from slot 0 to 9 to the current camera position.
  ///
  /// The camera will quickly interpolate to the stored position.
  void RestoreFavoriteCamera(xiiUInt8 uiSlot);

  /// Searches for a xiiCameraComponent with the 'EditorShortcut' property set to \a uiSlot and moves the editor camera to that position.
  xiiResult JumpToLevelCamera(xiiUInt8 uiSlot, bool bImmediate);

  /// Creates an object with a xiiCameraComponent at the current editor camera position and sets the 'EditorShortcut' property to \a uiSlot.
  xiiResult CreateLevelCamera(xiiUInt8 uiSlot);

  virtual xiiManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return xiiManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

  ///@}

  bool         CanUndoSelection() const;
  virtual void UndoSelection();

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

  xiiStatus RequestExportScene(xiiStringView sTargetFile, const xiiAssetFileHeader& header);

  virtual xiiTransformStatus InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  xiiTransformStatus         InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  void SyncObjectHiddenState();
  void SyncObjectHiddenState(xiiDocumentObject* pObject);

  /// Finds all objects that are actively being 'debugged' (or visualized) by the editor and thus should get the debug visualization flag in
  /// the runtime.
  void UpdateObjectDebugTargets();

  DocumentType m_DocumentType = DocumentType::Scene;

  GameMode::Enum m_GameMode;

  GameModeData m_GameModeData[3];

  // Local mirror for settings
  xiiDocumentObjectMirror m_ObjectMirror;
  xiiRttiConverterContext m_Context;

  //////////////////////////////////////////////////////////////////////////
protected:
  bool                                                                   m_bStoreSelectionChange  = true;
  xiiInt8                                                                m_iAllowSelectionChanges = -1;
  xiiCopyOnBroadcastEvent<const xiiSelectionManagerEvent&>::Unsubscriber m_SelectionHandlerUnsubscriber;
  void                                                                   SelectionManagerEventHandler(const xiiSelectionManagerEvent& e);

  struct SelectionHistory
  {
    xiiDynamicArray<xiiUuid> m_Objects;
    xiiUuid                  m_documentGuid;
  };

  xiiDeque<SelectionHistory> m_SelectionStack;

  //////////////////////////////////////////////////////////////////////////
  /// Communication with other document types
  virtual void OnInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender) override;
  void         GatherObjectsOfType(xiiDocumentObject* pRoot, xiiGatherObjectsOfTypeMsgInterDoc* pMsg) const;
};
