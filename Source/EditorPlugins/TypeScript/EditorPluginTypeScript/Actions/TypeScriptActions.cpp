#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorPluginTypeScript/Actions/TypeScriptActions.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiTypeScriptActions::s_hCategory;
xiiActionDescriptorHandle xiiTypeScriptActions::s_hEditScript;


void xiiTypeScriptActions::RegisterActions()
{
  s_hCategory   = XII_REGISTER_CATEGORY("TypeScriptCategory");
  s_hEditScript = XII_REGISTER_ACTION_1("TypeScript.Edit", xiiActionScope::Document, "TypeScripts", "Edit Script", xiiTypeScriptAction, xiiTypeScriptAction::ActionType::EditScript);
}

void xiiTypeScriptActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCategory);
  xiiActionManager::UnregisterAction(s_hEditScript);
}

void xiiTypeScriptActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "TypeScriptCategory";

  pMap->MapAction(s_hEditScript, szSubPath, 1.0f);
}

xiiTypeScriptAction::xiiTypeScriptAction(const xiiActionContext& context, const char* szName, xiiTypeScriptAction::ActionType type, float fSimSpeed) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<xiiTypeScriptAssetDocument*>(static_cast<const xiiTypeScriptAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::EditScript:
      SetIconPath(":/GuiFoundation/Icons/vscode16.png");
      break;
  }
}


void xiiTypeScriptAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::EditScript:
      m_pDocument->EditScript();
      return;
  }
}
