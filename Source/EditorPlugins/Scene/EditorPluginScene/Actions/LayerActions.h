/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <GuiFoundation/Action/BaseActions.h>

class xiiScene2Document;
struct xiiScene2LayerEvent;

///
class XII_EDITORPLUGINSCENE_DLL xiiLayerActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapContextMenuActions(xiiStringView sMapping);
  static void MapToolbarActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hLayerCategory;
  static xiiActionDescriptorHandle s_hCreateLayer;
  static xiiActionDescriptorHandle s_hDeleteLayer;
  static xiiActionDescriptorHandle s_hSaveLayer;
  static xiiActionDescriptorHandle s_hSaveActiveLayer;
  static xiiActionDescriptorHandle s_hLayerLoaded;
  static xiiActionDescriptorHandle s_hLayerVisible;
  static xiiActionDescriptorHandle s_hSwitchOnSelection;
};

///
class XII_EDITORPLUGINSCENE_DLL xiiLayerAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLayerAction, xiiButtonAction);

public:
  enum class ActionType : xiiUInt32
  {
    CreateLayer = 0U,
    DeleteLayer,
    SaveLayer,
    SaveActiveLayer,
    LayerLoaded,
    LayerVisible,
    SwitchOnSelection,
  };

  xiiLayerAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiLayerAction();

  static void  ToggleLayerLoaded(xiiScene2Document* pM_pSceneDocument, xiiUuid layerGuid);
  virtual void Execute(const xiiVariant& value) override;

private:
  void    LayerEventHandler(const xiiScene2LayerEvent& e);
  void    DocumentEventHandler(const xiiDocumentEvent& e);
  void    UpdateEnableState();
  xiiUuid GetCurrentSelectedLayer() const;

private:
  xiiScene2Document* m_pSceneDocument;
  ActionType         m_Type;
};
