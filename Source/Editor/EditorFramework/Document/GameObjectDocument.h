/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiAssetFileHeader;
class xiiGameObjectEditTool;
class xiiGameObjectDocument;

struct XII_EDITORFRAMEWORK_DLL TransformationChanges
{
  enum Enum
  {
    Translation = XII_BIT(0),
    Rotation    = XII_BIT(1),
    Scale       = XII_BIT(2),
    All         = 0xFF
  };
};

struct XII_EDITORFRAMEWORK_DLL xiiGameObjectEvent
{
  enum class Type
  {
    RenderSelectionOverlayChanged,
    RenderVisualizersChanged,
    RenderShapeIconsChanged,
    AddAmbientLightChanged,
    SimulationSpeedChanged,
    PickTransparentChanged,

    ActiveEditToolChanged,

    TriggerShowSelectionInScenegraph,
    TriggerFocusOnSelection_Hovered,
    TriggerFocusOnSelection_All,

    TriggerSnapSelectionPivotToGrid,
    TriggerSnapEachSelectedObjectToGrid,

    GameModeChanged,

    GizmoTransformMayBeInvalid, ///< Sent when a change was made that may affect the current gizmo / manipulator state (ie. objects have been moved)
  };

  Type m_Type;
};

struct xiiGameObjectDocumentEvent
{
  enum class Type
  {
    GameMode_Stopped,
    GameMode_StartingSimulate,
    GameMode_StartingPlay,
    GameMode_StartingExternal, ///< ie. scene is exported for xiiPlayer
  };

  Type                   m_Type;
  xiiGameObjectDocument* m_pDocument = nullptr;
};

class XII_EDITORFRAMEWORK_DLL xiiGameObjectMetaData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectMetaData, xiiReflectedClass);

public:
  enum ModifiedFlags
  {
    CachedName = XII_BIT(2),
    AllFlags   = 0xFFFFFFFF
  };

  xiiGameObjectMetaData() = default;

  xiiString m_CachedNodeName;
  QIcon     m_Icon;
};

struct XII_EDITORFRAMEWORK_DLL xiiSelectedGameObject
{
  const xiiDocumentObject* m_pObject;
  xiiVec3                  m_vLocalScaling;
  float                    m_fLocalUniformScaling;
  xiiTransform             m_GlobalTransform;
};

class XII_EDITORFRAMEWORK_DLL xiiGameObjectDocument : public xiiAssetDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectDocument, xiiAssetDocument);

public:
  xiiGameObjectDocument(xiiStringView sDocumentPath, xiiDocumentObjectManager* pObjectManager, xiiAssetDocEngineConnection engineConnectionType = xiiAssetDocEngineConnection::FullObjectMirroring);
  ~xiiGameObjectDocument();

  /// In case a document consists of multiple layers, this redirection is necessary to execute actions on the active layer.
  virtual xiiGameObjectDocument* GetRedirectedGameObjectDoc() { return this; }

  virtual xiiEditorInputContext* GetEditorInputContextOverride() override;

protected:
  void SubscribeGameObjectEventHandlers();
  void UnsubscribeGameObjectEventHandlers();

  void GameObjectDocumentEventHandler(const xiiGameObjectDocumentEvent& e);

  /// \name Gizmo
  ///@{
public:
  /// Makes an edit tool of the given type active. Allocates a new one, if necessary. Only works when SetEditToolConfigDelegate() is set.
  void SetActiveEditTool(const xiiRTTI* pEditToolType);

  /// Returns the currently active edit tool (nullptr for none).
  xiiGameObjectEditTool* GetActiveEditTool() const { return m_pActiveEditTool; }

  /// Checks whether an edit tool of the given type, or nullptr for none, is active.
  bool IsActiveEditTool(const xiiRTTI* pEditToolType) const;

  /// Needs to be called by some higher level code (usually the DocumentWindow) to react to newly created edit tools to configure them (call
  /// xiiGameObjectEditTool::ConfigureTool()).
  void SetEditToolConfigDelegate(xiiDelegate<void(xiiGameObjectEditTool*)> configDelegate);

  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

  void SetGizmoMoveParentOnly(bool bMoveParent);
  bool GetGizmoMoveParentOnly() const;

  /// Finds all objects that are selected at the top level, ie. none of their parents is selected.
  ///
  /// Additionally stores the current transformation. Useful to store this at the start of an operation
  /// to then do modifications on this base transformation every frame.
  void ComputeTopLevelSelectedGameObjects(xiiDeque<xiiSelectedGameObject>& out_selection);

  virtual void HandleEngineMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

private:
  void DeallocateEditTools();

  xiiDelegate<void(xiiGameObjectEditTool*)>      m_EditToolConfigDelegate;
  xiiGameObjectEditTool*                         m_pActiveEditTool = nullptr;
  xiiMap<const xiiRTTI*, xiiGameObjectEditTool*> m_CreatedEditTools;


  ///@}
  /// \name Actions
  ///@{

public:
  void TriggerShowSelectionInScenegraph() const;
  void TriggerFocusOnSelection(bool bAllViews) const;
  void TriggerSnapPivotToGrid() const;
  void TriggerSnapEachObjectToGrid() const;
  /// Moves the editor camera to the same position as the selected object
  void SnapCameraToObject();
  /// Moves the camera to the current picking position
  void MoveCameraHere();

  void ScheduleSendObjectSelection();

  /// Sends the current object selection, but only if it was modified or specifically tagged for resending with ScheduleSendObjectSelection().
  void SendObjectSelection();

  ///@}
  /// \name Settings
  ///@{
public:
  bool GetAddAmbientLight() const { return m_bAddAmbientLight; }
  void SetAddAmbientLight(bool b);

  float GetSimulationSpeed() const { return m_fSimulationSpeed; }
  void  SetSimulationSpeed(float f);

  bool GetPauseSimulation() const { return m_bPauseSimulation; }
  void SetPauseSimulation(bool b);

  void SetStepSimulation(bool b) { m_bStepSimulation = b; }
  bool GetStepSimulation() const { return m_bStepSimulation; }

  bool GetRenderSelectionOverlay() const { return m_CurrentMode.m_bRenderSelectionOverlay; }
  void SetRenderSelectionOverlay(bool b);

  bool GetRenderVisualizers() const { return m_CurrentMode.m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  bool GetRenderShapeIcons() const { return m_CurrentMode.m_bRenderShapeIcons; }
  void SetRenderShapeIcons(bool b);

  bool GetPickTransparent() const { return m_bPickTransparent; }
  void SetPickTransparent(bool b);

  /// Specifies which object is the 'active parent', which is the object under which newly created objects should be parented.
  void SetActiveParent(xiiUuid object);
  /// Returns the object under which newly created objects should be parented.
  ///
  /// \note The object may not exist anymore! So check with the ObjectManager first.
  xiiUuid GetActiveParent() const { return m_ActiveParent; }

private:
  xiiUuid m_ActiveParent = xiiUuid::MakeInvalid();

  ///@}
  /// \name Transform
  ///@{

public:
  /// Sets the new global transformation of the given object.
  /// The transformationChanges bitmask (of type TransformationChanges) allows to tell the system that, e.g. only translation has changed and thus
  /// some work can be spared.
  void SetGlobalTransform(const xiiDocumentObject* pObject, const xiiTransform& t, xiiUInt8 uiTransformationChanges) const;

  /// Same as SetGlobalTransform, except that all children will keep their current global transform (thus their local transforms are adjusted)
  void SetGlobalTransformParentOnly(const xiiDocumentObject* pObject, const xiiTransform& t, xiiUInt8 uiTransformationChanges) const;

  /// Returns a cached value for the global transform of the given object, if available. Otherwise it calls ComputeGlobalTransform().
  xiiTransform GetGlobalTransform(const xiiDocumentObject* pObject) const;

  /// Retrieves the local transform property values from the object and combines it into one xiiTransform
  static xiiTransform     QueryLocalTransform(const xiiDocumentObject* pObject);
  static xiiSimdTransform QueryLocalTransformSimd(const xiiDocumentObject* pObject);

  /// Computes the global transform of the parent and combines it with the local transform of the given object.
  /// This function does not return a cached value, but always computes it. It does update the internal cache for later reads though.
  xiiTransform ComputeGlobalTransform(const xiiDocumentObject* pObject) const;

  /// Traverses the pObject hierarchy up until it hits a xiiGameObject, then computes the global transform of that.
  virtual xiiResult ComputeObjectTransformation(const xiiDocumentObject* pObject, xiiTransform& out_result) const override;

  ///@}
  /// \name Node Names
  ///@{

  /// Generates a good name for pObject. Queries the "Name" property, child components and asset properties, if necessary.
  void DetermineNodeName(const xiiDocumentObject* pObject, const xiiUuid& prefabGuid, xiiStringBuilder& out_sResult, QIcon* out_pIcon = nullptr) const;

  /// Similar to DetermineNodeName() but prefers to return the last cached value from scene meta data. This is more efficient, but may give an
  /// outdated result.
  void QueryCachedNodeName(const xiiDocumentObject* pObject, xiiStringBuilder& out_sResult, xiiUuid* out_pPrefabGuid = nullptr, QIcon* out_pIcon = nullptr) const;

  /// Creates a full "path" to a scene object for display in UIs. No guarantee for uniqueness.
  void GenerateFullDisplayName(const xiiDocumentObject* pRoot, xiiStringBuilder& out_sFullPath) const;

  ///@}

public:
  mutable xiiEvent<const xiiGameObjectEvent&>                             m_GameObjectEvents;
  mutable xiiUniquePtr<xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>> m_GameObjectMetaData;

  static xiiEvent<const xiiGameObjectDocumentEvent&> s_GameObjectDocumentEvents;

protected:
  void InvalidateGlobalTransformValue(const xiiDocumentObject* pObject) const;
  /// Sends the current state of the scene to the engine process. This is typically done after scene load or when the world might have deviated
  /// on the engine side (after play the game etc.)
  virtual void SendGameWorldToEngine();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;

public:
  void SelectionManagerEventHandler(const xiiSelectionManagerEvent& e);
  void ObjectPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void ObjectStructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void ObjectEventHandler(const xiiDocumentObjectEvent& e);

protected:
  struct XII_EDITORFRAMEWORK_DLL GameModeData
  {
    bool m_bRenderSelectionOverlay;
    bool m_bRenderVisualizers;
    bool m_bRenderShapeIcons;
  };
  GameModeData m_CurrentMode;

private:
  bool m_bAddAmbientLight     = false;
  bool m_bGizmoWorldSpace     = true; // whether the gizmo is in local/global space mode
  bool m_bGizmoMoveParentOnly = false;
  bool m_bPickTransparent     = true;

  bool  m_bPauseSimulation = false;
  bool  m_bStepSimulation  = false;
  float m_fSimulationSpeed = 1.0f;

  using TransformTable = xiiHashTable<const xiiDocumentObject*, xiiSimdTransform, xiiHashHelper<const xiiDocumentObject*>, xiiAlignedAllocatorWrapper>;
  mutable TransformTable m_GlobalTransforms;

  // when new objects are created the engine sometimes needs to catch up creating sub-objects (e.g. for reference prefabs)
  // therefore when the selection is changed in the first frame, it might not be fully correct
  // by sending it a second time, we can fix that easily
  xiiInt8 m_iResendSelection = 0;

protected:
  xiiEventSubscriptionID m_SelectionManagerEventHandlerID;
  xiiEventSubscriptionID m_ObjectPropertyEventHandlerID;
  xiiEventSubscriptionID m_ObjectStructureEventHandlerID;
  xiiEventSubscriptionID m_ObjectEventHandlerID;
};
