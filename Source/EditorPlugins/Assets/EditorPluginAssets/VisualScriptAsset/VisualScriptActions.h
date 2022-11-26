#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiVisualScriptActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hCategory;
  static xiiActionDescriptorHandle s_hPickDebugTarget;
};

class xiiVisualScriptAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptAction, xiiButtonAction);

public:
  enum class ActionType
  {
    PickDebugTarget,
  };

  xiiVisualScriptAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiVisualScriptAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  ActionType m_Type;
};
