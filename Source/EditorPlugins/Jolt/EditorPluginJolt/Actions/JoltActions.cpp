#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiJoltActions::s_hCategoryJolt;
xiiActionDescriptorHandle xiiJoltActions::s_hProjectSettings;

void xiiJoltActions::RegisterActions()
{
  s_hCategoryJolt    = XII_REGISTER_CATEGORY("Jolt");
  s_hProjectSettings = XII_REGISTER_ACTION_1("Jolt.Settings.Project", xiiActionScope::Document, "Jolt", "", xiiJoltAction, xiiJoltAction::ActionType::ProjectSettings);
}

void xiiJoltActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCategoryJolt);
  xiiActionManager::UnregisterAction(s_hProjectSettings);
}

void xiiJoltActions::MapMenuActions()
{
  /// \todo Is there a way to integrate into ALL document types in a specific menu (ie. project settings)
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap("EditorPluginScene_Scene2MenuBar");
  XII_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryJolt, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "Menu.Editor/ProjectCategory/Menu.ProjectSettings/Jolt", 1.0f);
}

xiiJoltAction::xiiJoltAction(const xiiActionContext& context, const char* szName, ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      // SetIconPath(":/EditorPluginScene/Icons/GizmoNone24.png"); /// \todo Icon
      break;
  }
}

xiiJoltAction::~xiiJoltAction() {}

void xiiJoltAction::Execute(const xiiVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    xiiQtJoltProjectSettingsDlg dlg(nullptr);
    if (dlg.exec() == QDialog::Accepted)
    {
      xiiToolsProject::BroadcastConfigChanged();
    }
  }
}
