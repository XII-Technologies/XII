/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/////
class XII_EDITORPLUGINSCENE_DLL xiiSceneGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(xiiStringView sMapping);
  static void MapToolbarActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hGreyBoxingGizmo;
};
