/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class XII_EDITORFRAMEWORK_DLL xiiQuadViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hToggleViews;
  static xiiActionDescriptorHandle s_hSpawnView;
};

///
class XII_EDITORFRAMEWORK_DLL xiiQuadViewAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiQuadViewAction, xiiButtonAction);

public:
  enum class ButtonType
  {
    ToggleViews,
    SpawnView,
  };

  xiiQuadViewAction(const xiiActionContext& context, const char* szName, ButtonType button);
  ~xiiQuadViewAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  ButtonType m_ButtonType;
};
