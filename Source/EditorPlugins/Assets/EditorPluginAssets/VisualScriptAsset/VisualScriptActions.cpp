#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptActions.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiVisualScriptActions::s_hCategory;
xiiActionDescriptorHandle xiiVisualScriptActions::s_hPickDebugTarget;

void xiiVisualScriptActions::RegisterActions()
{
  s_hCategory        = XII_REGISTER_CATEGORY("VisualScriptCategory");
  s_hPickDebugTarget = XII_REGISTER_ACTION_1(
    "VisScript.PickDebugTarget", xiiActionScope::Window, "Visual Script", "", xiiVisualScriptAction, xiiVisualScriptAction::ActionType::PickDebugTarget);
}

void xiiVisualScriptActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCategory);
  xiiActionManager::UnregisterAction(s_hPickDebugTarget);
}

void xiiVisualScriptActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const char* szSubPath = "VisualScriptCategory";

  pMap->MapAction(s_hPickDebugTarget, szSubPath, 1.0f);
}

xiiVisualScriptAction::xiiVisualScriptAction(const xiiActionContext& context, const char* szName, xiiVisualScriptAction::ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::PickDebugTarget:
      SetIconPath(":/EditorPluginAssets/PickTarget16.png");
      break;
  }
}

xiiVisualScriptAction::~xiiVisualScriptAction() {}

void xiiVisualScriptAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::PickDebugTarget:
      static_cast<xiiQtVisualScriptAssetDocumentWindow*>(GetContext().m_pWindow)->PickDebugTarget();
      return;
  }
}
