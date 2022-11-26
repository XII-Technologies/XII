#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class XII_EDITORPLUGINJOLT_DLL xiiJoltActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static xiiActionDescriptorHandle s_hCategoryJolt;
  static xiiActionDescriptorHandle s_hProjectSettings;
};

class XII_EDITORPLUGINJOLT_DLL xiiJoltAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltAction, xiiButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
  };

  xiiJoltAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiJoltAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  ActionType m_Type;
};
