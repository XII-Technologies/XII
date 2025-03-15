#pragma once

#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class xiiScene2Document;

class xiiSceneLayerBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneLayerBase, xiiReflectedClass);

public:
  xiiSceneLayerBase();
  ~xiiSceneLayerBase();

public:
  mutable xiiScene2Document* m_pDocument = nullptr;
};

class xiiSceneLayer : public xiiSceneLayerBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneLayer, xiiSceneLayerBase);

public:
  xiiSceneLayer();
  ~xiiSceneLayer();

public:
  xiiUuid m_Layer;
};

class xiiSceneDocumentSettings : public xiiSceneDocumentSettingsBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneDocumentSettings, xiiSceneDocumentSettingsBase);

public:
  xiiSceneDocumentSettings();
  ~xiiSceneDocumentSettings();

public:
  xiiDynamicArray<xiiSceneLayerBase*> m_Layers;
  mutable xiiScene2Document*          m_pDocument = nullptr;
};

struct xiiScene2LayerEvent
{
  enum class Type
  {
    LayerAdded,
    LayerRemoved,
    LayerLoaded,
    LayerUnloaded,
    LayerVisible,
    LayerInvisible,
    ActiveLayerChanged,
  };

  Type    m_Type;
  xiiUuid m_layerGuid;
};

class XII_EDITORPLUGINSCENE_DLL xiiScene2Document : public xiiSceneDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScene2Document, xiiSceneDocument);

public:
  xiiScene2Document(xiiStringView sDocumentPath);
  ~xiiScene2Document();

  /// \name Scene Data Accessors
  ///@{

  const xiiDocumentObjectManager*                              GetSceneObjectManager() const { return m_pSceneObjectManager.Borrow(); }
  xiiDocumentObjectManager*                                    GetSceneObjectManager() { return m_pSceneObjectManager.Borrow(); }
  xiiSelectionManager*                                         GetSceneSelectionManager() const { return m_pSceneSelectionManager.Borrow(); }
  xiiCommandHistory*                                           GetSceneCommandHistory() const { return m_pSceneCommandHistory.Borrow(); }
  xiiObjectAccessorBase*                                       GetSceneObjectAccessor() const { return m_pSceneObjectAccessor.Borrow(); }
  const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>* GetSceneDocumentObjectMetaData() const { return m_pSceneDocumentObjectMetaData.Borrow(); }
  xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>*       GetSceneDocumentObjectMetaData() { return m_pSceneDocumentObjectMetaData.Borrow(); }
  const xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>*     GetSceneGameObjectMetaData() const { return m_pSceneGameObjectMetaData.Borrow(); }
  xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>*           GetSceneGameObjectMetaData() { return m_pSceneGameObjectMetaData.Borrow(); }

  ///@}
  /// \name Layer Functions
  ///@{

  xiiSelectionManager* GetLayerSelectionManager() const { return m_pLayerSelection.Borrow(); }

  xiiStatus CreateLayer(xiiStringView sName, xiiUuid& out_layerGuid);
  xiiStatus DeleteLayer(const xiiUuid& layerGuid);

  const xiiUuid& GetActiveLayer() const;
  xiiStatus      SetActiveLayer(const xiiUuid& layerGuid);

  bool      IsLayerLoaded(const xiiUuid& layerGuid) const;
  xiiStatus SetLayerLoaded(const xiiUuid& layerGuid, bool bLoaded);
  void      GetAllLayers(xiiDynamicArray<xiiUuid>& out_layerGuids);
  void      GetLoadedLayers(xiiDynamicArray<xiiSceneDocument*>& out_layers) const;

  bool      IsLayerVisible(const xiiUuid& layerGuid) const;
  xiiStatus SetLayerVisible(const xiiUuid& layerGuid, bool bVisible);

  const xiiDocumentObject* GetLayerObject(const xiiUuid& layerGuid) const;
  xiiSceneDocument*        GetLayerDocument(const xiiUuid& layerGuid) const;

  virtual xiiGameObjectDocument* GetRedirectedGameObjectDoc() override;

  bool IsAnyLayerModified() const;

  ///@}
  /// \name Base Class Functions
  ///@{

  virtual void                     InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void                     InitializeAfterLoadingAndSaving() override;
  virtual const xiiDocumentObject* GetSettingsObject() const override;
  virtual void                     HandleEngineMessage(const xiiEditorEngineDocumentMsg* pMsg) override;
  virtual xiiTaskGroupID           InternalSaveDocument(AfterSaveCallback callback) override;
  virtual void                     SendGameWorldToEngine() override;
  virtual xiiTransformStatus        InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& assetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  ///@}
  /// \name Selection Specific Functions
  ///@{

  void         PreventDoubleSelectionChange(bool b);
  virtual void UndoSelection() override;

  ///@}


public:
  mutable xiiEvent<const xiiScene2LayerEvent&> m_LayerEvents;

private:
  void LayerSelectionEventHandler(const xiiSelectionManagerEvent& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);
  void DocumentManagerEventHandler(const xiiDocumentManager::Event& e);
  void HandleObjectStateFromEngineMsg2(const xiiPushObjectStateMsgToEditor* pMsg);

  void UpdateLayers();
  void SendLayerVisibility();
  void LayerAdded(const xiiUuid& layerGuid, const xiiUuid& layerObjectGuid);
  void LayerRemoved(const xiiUuid& layerGuid);

private:
  friend class xiiSceneLayer;
  xiiCopyOnBroadcastEvent<const xiiDocumentObjectStructureEvent&>::Unsubscriber m_StructureEventSubscriber;
  xiiCopyOnBroadcastEvent<const xiiSelectionManagerEvent&>::Unsubscriber        m_LayerSelectionEventSubscriber;
  xiiEvent<const xiiCommandHistoryEvent&, xiiMutex>::Unsubscriber               m_CommandHistoryEventSubscriber;
  xiiCopyOnBroadcastEvent<const xiiDocumentManager::Event&>::Unsubscriber       m_DocumentManagerEventSubscriber;

  // This is used for a flattened list of the xiiSceneDocumentSettings hierarchy
  struct LayerInfo
  {
    xiiSceneDocument* m_pLayer = nullptr;
    xiiUuid           m_objectGuid;
    bool              m_bVisible = true;
  };

  // Scene document cache
  xiiUniquePtr<xiiDocumentObjectManager>                              m_pSceneObjectManager;
  mutable xiiUniquePtr<xiiCommandHistory>                             m_pSceneCommandHistory;
  mutable xiiUniquePtr<xiiSelectionManager>                           m_pSceneSelectionManager;
  mutable xiiUniquePtr<xiiObjectCommandAccessor>                      m_pSceneObjectAccessor;
  xiiUniquePtr<xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>> m_pSceneDocumentObjectMetaData;
  xiiUniquePtr<xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>>     m_pSceneGameObjectMetaData;

  // Layer state
  mutable xiiUniquePtr<xiiSelectionManager> m_pLayerSelection;
  xiiUuid                                   m_ActiveLayerGuid;
  xiiHashTable<xiiUuid, LayerInfo>          m_Layers;

  void ActiveLayerGameObjectEventHandler(const xiiGameObjectEvent& e);

  xiiEvent<const xiiGameObjectEvent&>::Unsubscriber m_ActiveLayerGoEvUnsubscriber;
};
