#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginScene/EnginePluginSceneDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>

class xiiObjectSelectionMsgToEngine;
class xiiRenderContext;
class xiiGameStateBase;
class xiiGameModeMsgToEngine;
class xiiWorldSettingsMsgToEngine;
class xiiObjectsForDebugVisMsgToEngine;
struct xiiVisualScriptComponentActivityEvent;
class xiiGridSettingsMsgToEngine;
class xiiSimulationSettingsMsgToEngine;
struct xiiResourceManagerEvent;
class xiiExposedDocumentObjectPropertiesMsgToEngine;
class xiiViewRedrawMsgToEngine;
class xiiWorldWriter;
class xiiDeferredFileWriter;
class xiiLayerContext;
struct xiiGameApplicationExecutionEvent;

class XII_ENGINEPLUGINSCENE_DLL xiiSceneContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneContext, xiiEngineProcessDocumentContext);

public:
  xiiSceneContext();
  ~xiiSceneContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiDeque<xiiGameObjectHandle>& GetSelection() const { return m_Selection; }
  const xiiDeque<xiiGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }
  bool                                 GetRenderSelectionOverlay() const { return m_bRenderSelectionOverlay; }
  bool                                 GetRenderShapeIcons() const { return m_bRenderShapeIcons; }
  bool                                 GetRenderSelectionBoxes() const { return m_bRenderSelectionBoxes; }
  float                                GetGridDensity() const { return xiiMath::Abs(m_fGridDensity); }
  bool                                 IsGridInGlobalSpace() const { return m_fGridDensity >= 0.0f; }
  xiiTransform                         GetGridTransform() const { return m_GridTransform; }

  xiiGameStateBase* GetGameState() const;
  bool              IsPlayTheGameActive() const { return GetGameState() != nullptr; }

  xiiUInt32                       RegisterLayer(xiiLayerContext* pLayer);
  void                            UnregisterLayer(xiiLayerContext* pLayer);
  void                            AddLayerIndexTag(const xiiEntityMsgToEngine& msg, xiiWorldRttiConverterContext& context, const xiiTag& layerTag);
  const xiiArrayPtr<const xiiTag> GetInvisibleLayerTags() const;

  xiiEngineProcessDocumentContext*           GetActiveDocumentContext();
  const xiiEngineProcessDocumentContext*     GetActiveDocumentContext() const;
  xiiWorldRttiConverterContext&              GetActiveContext();
  const xiiWorldRttiConverterContext&        GetActiveContext() const;
  xiiWorldRttiConverterContext*              GetContextForLayer(const xiiUuid& layerGuid);
  xiiArrayPtr<xiiWorldRttiConverterContext*> GetAllContexts();

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual xiiStatus                    ExportDocument(const xiiExportDocumentMsgToEngine* pMsg) override;
  void                                 ExportExposedParameters(const xiiWorldWriter& ww, xiiDeferredFileWriter& file) const;

  virtual bool                UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;
  virtual void                OnThumbnailViewContextCreated() override;
  virtual void                OnDestroyThumbnailViewContext() override;
  virtual void                UpdateDocumentContext() override;
  virtual xiiGameObjectHandle ResolveStringToGameObjectHandle(const void* pString, xiiComponentHandle hThis, const char* szProperty) const override;

private:
  struct TagGameObject
  {
    xiiGameObjectHandle m_hObject;
    xiiTag              m_Tag;
  };

  void AddAmbientLight(bool bSetEditorTag, bool bForce);
  void RemoveAmbientLight();

  void HandleViewRedrawMsg(const xiiViewRedrawMsgToEngine* pMsg);
  void HandleSelectionMsg(const xiiObjectSelectionMsgToEngine* pMsg);
  void HandleGameModeMsg(const xiiGameModeMsgToEngine* pMsg);
  void HandleSimulationSettingsMsg(const xiiSimulationSettingsMsgToEngine* msg);
  void HandleGridSettingsMsg(const xiiGridSettingsMsgToEngine* msg);
  void HandleWorldSettingsMsg(const xiiWorldSettingsMsgToEngine* msg);
  void HandleObjectsForDebugVisMsg(const xiiObjectsForDebugVisMsgToEngine* pMsg);
  void ComputeHierarchyBounds(xiiGameObject* pObj, xiiBoundingBoxSphere& bounds);
  void HandleExposedPropertiesMsg(const xiiExposedDocumentObjectPropertiesMsgToEngine* pMsg);
  void HandleSceneGeometryMsg(const xiiExportSceneGeometryMsgToEngine* pMsg);
  void HandlePullObjectStateMsg(const xiiPullObjectStateMsgToEngine* pMsg);
  void AnswerObjectStatePullRequest(const xiiViewRedrawMsgToEngine* pMsg);
  void HandleActiveLayerChangedMsg(const xiiActiveLayerChangedMsgToEngine* pMsg);
  void HandleTagMsgToEngineMsg(const xiiObjectTagMsgToEngine* pMsg);
  void HandleLayerVisibilityChangedMsgToEngineMsg(const xiiLayerVisibilityChangedMsgToEngine* pMsg);

  void DrawSelectionBounds(const xiiViewHandle& hView);

  void UpdateInvisibleLayerTags();
  void InsertSelectedChildren(const xiiGameObject* pObject);
  void QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);
  void OnSimulationEnabled();
  void OnSimulationDisabled();
  void OnPlayTheGameModeStarted(const xiiTransform* pStartPosition);

  void OnVisualScriptActivity(const xiiVisualScriptComponentActivityEvent& e);
  void OnResourceManagerEvent(const xiiResourceManagerEvent& e);
  void GameApplicationEventHandler(const xiiGameApplicationExecutionEvent& e);

  bool         m_bUpdateAllLocalBounds = false;
  bool         m_bRenderSelectionOverlay;
  bool         m_bRenderShapeIcons;
  bool         m_bRenderSelectionBoxes;
  float        m_fGridDensity;
  xiiTransform m_GridTransform;

  xiiDeque<xiiGameObjectHandle>            m_Selection;
  xiiDeque<xiiGameObjectHandle>            m_SelectionWithChildren;
  xiiSet<xiiGameObjectHandle>              m_SelectionWithChildrenSet;
  xiiGameObjectHandle                      m_hSkyLight;
  xiiGameObjectHandle                      m_hDirectionalLight;
  xiiDynamicArray<xiiExposedSceneProperty> m_ExposedSceneProperties;

  xiiPushObjectStateMsgToEditor m_PushObjectStateMsg;

  xiiUuid                                        m_ActiveLayer;
  xiiDynamicArray<xiiLayerContext*>              m_Layers;
  xiiDynamicArray<xiiWorldRttiConverterContext*> m_Contexts;

  // We use tags in the form of Layer_4 (Layer_Scene for the scene itself) to not pollute the tag registry with hundreds of unique tags. The tags do not need to be unique across documents so we can just use the layer index but that requires the Tags to be recomputed whenever we remove / add layers.
  // By caching the guids we do not need to send another message each time a layer is loaded as we send also guids of unloaded layers.
  xiiTag                     m_LayerTag;
  xiiHybridArray<xiiUuid, 1> m_InvisibleLayers;
  bool                       m_bInvisibleLayersDirty = true;
  xiiHybridArray<xiiTag, 1>  m_InvisibleLayerTags;

  xiiDynamicArray<TagGameObject> m_ObjectsToTag;
};
