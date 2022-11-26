#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class XII_EDITORFRAMEWORK_DLL xiiGameObjectContextActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(const char* szMapping, const char* szPath);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hCategory;
  static xiiActionDescriptorHandle s_hPickContextScene;
  static xiiActionDescriptorHandle s_hPickContextObject;
  static xiiActionDescriptorHandle s_hClearContextObject;
};

class XII_EDITORFRAMEWORK_DLL xiiGameObjectContextAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectContextAction, xiiButtonAction);

public:
  enum class ActionType
  {
    PickContextScene,
    PickContextObject,
    ClearContextObject,
  };

  xiiGameObjectContextAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiGameObjectContextAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);
  void Update();

  ActionType m_Type;
};
