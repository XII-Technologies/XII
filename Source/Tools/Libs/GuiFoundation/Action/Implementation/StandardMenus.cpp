#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

xiiActionDescriptorHandle xiiStandardMenus::s_hMenuFile;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuEdit;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuPanels;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuProject;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuScene;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuView;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuHelp;
xiiActionDescriptorHandle xiiStandardMenus::s_hCheckForUpdates;
xiiActionDescriptorHandle xiiStandardMenus::s_hReportProblem;

void xiiStandardMenus::RegisterActions()
{
  s_hMenuFile        = XII_REGISTER_MENU("Menu.File");
  s_hMenuEdit        = XII_REGISTER_MENU("Menu.Edit");
  s_hMenuPanels      = XII_REGISTER_DYNAMIC_MENU("Menu.Panels", xiiApplicationPanelsMenuAction, "");
  s_hMenuProject     = XII_REGISTER_MENU("Menu.Project");
  s_hMenuScene       = XII_REGISTER_MENU("Menu.Scene");
  s_hMenuView        = XII_REGISTER_MENU("Menu.View");
  s_hMenuHelp        = XII_REGISTER_MENU("Menu.Help");
  s_hCheckForUpdates = XII_REGISTER_ACTION_1("Help.CheckForUpdates", xiiActionScope::Global, "Help", "", xiiHelpActions, xiiHelpActions::ButtonType::CheckForUpdates);
  s_hReportProblem   = XII_REGISTER_ACTION_1("Help.ReportProblem", xiiActionScope::Global, "Help", "", xiiHelpActions, xiiHelpActions::ButtonType::ReportProblem);
}

void xiiStandardMenus::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hMenuFile);
  xiiActionManager::UnregisterAction(s_hMenuEdit);
  xiiActionManager::UnregisterAction(s_hMenuPanels);
  xiiActionManager::UnregisterAction(s_hMenuProject);
  xiiActionManager::UnregisterAction(s_hMenuScene);
  xiiActionManager::UnregisterAction(s_hMenuView);
  xiiActionManager::UnregisterAction(s_hMenuHelp);
  xiiActionManager::UnregisterAction(s_hCheckForUpdates);
  xiiActionManager::UnregisterAction(s_hReportProblem);
}

void xiiStandardMenus::MapActions(const char* szMapping, const xiiBitflags<xiiStandardMenuTypes>& Menus)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "'{0}' does not exist", szMapping);

  xiiActionMapDescriptor md;

  if (Menus.IsAnySet(xiiStandardMenuTypes::File))
    pMap->MapAction(s_hMenuFile, "", 1.0f);

  if (Menus.IsAnySet(xiiStandardMenuTypes::Edit))
    pMap->MapAction(s_hMenuEdit, "", 2.0f);

  if (Menus.IsAnySet(xiiStandardMenuTypes::Project))
    pMap->MapAction(s_hMenuProject, "", 3.0f);

  if (Menus.IsAnySet(xiiStandardMenuTypes::Scene))
    pMap->MapAction(s_hMenuScene, "", 4.0f);

  if (Menus.IsAnySet(xiiStandardMenuTypes::View))
    pMap->MapAction(s_hMenuView, "", 5.0f);

  if (Menus.IsAnySet(xiiStandardMenuTypes::Panels))
    pMap->MapAction(s_hMenuPanels, "", 6.0f);

  if (Menus.IsAnySet(xiiStandardMenuTypes::Help))
  {
    pMap->MapAction(s_hMenuHelp, "", 7.0f);
    pMap->MapAction(s_hReportProblem, "Menu.Help", 3.0f);
    pMap->MapAction(s_hCheckForUpdates, "Menu.Help", 10.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// xiiApplicationPanelsMenuAction
////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiApplicationPanelsMenuAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct xiiComparePanels
{
  /// \brief Returns true if a is less than b
  XII_ALWAYS_INLINE bool Less(const xiiDynamicMenuAction::Item& p1, const xiiDynamicMenuAction::Item& p2) const { return p1.m_sDisplay < p2.m_sDisplay; }

  /// \brief Returns true if a is equal to b
  XII_ALWAYS_INLINE bool Equal(const xiiDynamicMenuAction::Item& p1, const xiiDynamicMenuAction::Item& p2) const
  {
    return p1.m_sDisplay == p2.m_sDisplay;
  }
};


void xiiApplicationPanelsMenuAction::GetEntries(xiiHybridArray<xiiDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  for (auto* pPanel : xiiQtApplicationPanel::GetAllApplicationPanels())
  {
    xiiDynamicMenuAction::Item item;
    item.m_sDisplay   = pPanel->windowTitle().toUtf8().data();
    item.m_UserValue  = pPanel;
    item.m_Icon       = pPanel->icon();
    item.m_CheckState = pPanel->isClosed() ? xiiDynamicMenuAction::Item::CheckMark::Unchecked : xiiDynamicMenuAction::Item::CheckMark::Checked;

    out_Entries.PushBack(item);
  }

  // make sure the panels appear in alphabetical order in the menu
  xiiComparePanels cp;
  out_Entries.Sort<xiiComparePanels>(cp);
}

void xiiApplicationPanelsMenuAction::Execute(const xiiVariant& value)
{
  xiiQtApplicationPanel* pPanel = static_cast<xiiQtApplicationPanel*>(value.ConvertTo<void*>());
  if (pPanel->isClosed())
  {
    pPanel->toggleView(true);
    pPanel->EnsureVisible();
  }
  else
  {
    pPanel->toggleView(false);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHelpActions, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHelpActions::xiiHelpActions(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  if (button == ButtonType::ReportProblem)
  {
    SetIconPath(":/EditorFramework/Icons/GitHub-Light.png");
  }
}

xiiHelpActions::~xiiHelpActions() = default;

void xiiHelpActions::Execute(const xiiVariant& value)
{
  if (m_ButtonType == ButtonType::ReportProblem)
  {
    QDesktopServices::openUrl(QUrl("https://github.com/xiiEngine/xiiEngine/issues"));
  }
  if (m_ButtonType == ButtonType::CheckForUpdates)
  {
    xiiQtUiServices::CheckForUpdates();
  }
}
