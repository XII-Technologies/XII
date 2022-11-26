#pragma once

#include <EditorPluginTypeScript/EditorPluginTypeScriptDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiTypeScriptAssetDocument;
struct xiiTypeScriptAssetEvent;

class xiiTypeScriptActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hCategory;
  static xiiActionDescriptorHandle s_hEditScript;
};

class xiiTypeScriptAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptAction, xiiButtonAction);

public:
  enum class ActionType
  {
    EditScript,
  };

  xiiTypeScriptAction(const xiiActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);

  virtual void Execute(const xiiVariant& value) override;

private:
  xiiTypeScriptAssetDocument* m_pDocument = nullptr;
  ActionType                  m_Type;
};
