/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

xiiActionDescriptorHandle xiiStandardMenus::s_hMenuProject;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuFile;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuEdit;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuPanels;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuScene;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuAsset;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuView;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuTools;
xiiActionDescriptorHandle xiiStandardMenus::s_hMenuHelp;
xiiActionDescriptorHandle xiiStandardMenus::s_hCheckForUpdates;
xiiActionDescriptorHandle xiiStandardMenus::s_hReportProblem;

void xiiStandardMenus::RegisterActions()
{
  s_hMenuProject     = XII_REGISTER_MENU("G.Project");
  s_hMenuFile        = XII_REGISTER_MENU("G.File");
  s_hMenuEdit        = XII_REGISTER_MENU("G.Edit");
  s_hMenuPanels      = XII_REGISTER_DYNAMIC_MENU("G.Panels", xiiApplicationPanelsMenuAction, "");
  s_hMenuScene       = XII_REGISTER_MENU("G.Scene");
  s_hMenuAsset       = XII_REGISTER_MENU("G.Asset");
  s_hMenuView        = XII_REGISTER_MENU("G.View");
  s_hMenuTools       = XII_REGISTER_MENU("G.Tools");
  s_hMenuHelp        = XII_REGISTER_MENU("G.Help");
  s_hCheckForUpdates = XII_REGISTER_ACTION_1("Help.CheckForUpdates", xiiActionScope::Global, "Help", "", xiiHelpActions, xiiHelpActions::ButtonType::CheckForUpdates);
  s_hReportProblem   = XII_REGISTER_ACTION_1("Help.ReportProblem", xiiActionScope::Global, "Help", "", xiiHelpActions, xiiHelpActions::ButtonType::ReportProblem);
}

void xiiStandardMenus::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hMenuProject);
  xiiActionManager::UnregisterAction(s_hMenuFile);
  xiiActionManager::UnregisterAction(s_hMenuEdit);
  xiiActionManager::UnregisterAction(s_hMenuPanels);
  xiiActionManager::UnregisterAction(s_hMenuScene);
  xiiActionManager::UnregisterAction(s_hMenuAsset);
  xiiActionManager::UnregisterAction(s_hMenuView);
  xiiActionManager::UnregisterAction(s_hMenuTools);
  xiiActionManager::UnregisterAction(s_hMenuHelp);
  xiiActionManager::UnregisterAction(s_hCheckForUpdates);
  xiiActionManager::UnregisterAction(s_hReportProblem);
}

void xiiStandardMenus::MapActions(xiiStringView sMapping, const xiiBitflags<xiiStandardMenuTypes>& menus)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "'{0}' does not exist", sMapping);

  xiiActionMapDescriptor md;

  if (menus.IsAnySet(xiiStandardMenuTypes::Project))
    pMap->MapAction(s_hMenuProject, "", -10000.0f);

  if (menus.IsAnySet(xiiStandardMenuTypes::File))
    pMap->MapAction(s_hMenuFile, "", 1.0f);

  if (menus.IsAnySet(xiiStandardMenuTypes::Edit))
    pMap->MapAction(s_hMenuEdit, "", 2.0f);

  if (menus.IsAnySet(xiiStandardMenuTypes::Scene))
    pMap->MapAction(s_hMenuScene, "", 3.0f);

  if (menus.IsAnySet(xiiStandardMenuTypes::Asset))
    pMap->MapAction(s_hMenuAsset, "", 4.0f);

  if (menus.IsAnySet(xiiStandardMenuTypes::View))
    pMap->MapAction(s_hMenuView, "", 5.0f);

  if (menus.IsAnySet(xiiStandardMenuTypes::Tools))
    pMap->MapAction(s_hMenuTools, "", 6.0f);

  if (menus.IsAnySet(xiiStandardMenuTypes::Panels))
    pMap->MapAction(s_hMenuPanels, "", 7.0f);

  if (menus.IsAnySet(xiiStandardMenuTypes::Help))
  {
    pMap->MapAction(s_hMenuHelp, "", 8.0f);
    pMap->MapAction(s_hReportProblem, "G.Help", 3.0f);
    pMap->MapAction(s_hCheckForUpdates, "G.Help", 10.0f);
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
  /// Returns true if a is less than b
  XII_ALWAYS_INLINE bool Less(const xiiDynamicMenuAction::Item& p1, const xiiDynamicMenuAction::Item& p2) const { return p1.m_sDisplay < p2.m_sDisplay; }

  /// Returns true if a is equal to b
  XII_ALWAYS_INLINE bool Equal(const xiiDynamicMenuAction::Item& p1, const xiiDynamicMenuAction::Item& p2) const
  {
    return p1.m_sDisplay == p2.m_sDisplay;
  }
};


void xiiApplicationPanelsMenuAction::GetEntries(xiiDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  for (auto* pPanel : xiiQtApplicationPanel::GetAllApplicationPanels())
  {
    xiiDynamicMenuAction::Item item;
    item.m_sDisplay   = pPanel->windowTitle().toUtf8().data();
    item.m_UserValue  = pPanel;
    item.m_Icon       = pPanel->icon();
    item.m_CheckState = pPanel->isClosed() ? xiiDynamicMenuAction::Item::CheckMark::Unchecked : xiiDynamicMenuAction::Item::CheckMark::Checked;

    out_entries.PushBack(item);
  }

  // make sure the panels appear in alphabetical order in the menu
  xiiComparePanels cp;
  out_entries.Sort<xiiComparePanels>(cp);
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

xiiHelpActions::xiiHelpActions(const xiiActionContext& context, xiiStringView sName, ButtonType button) :
  xiiButtonAction(context, sName, false, "")
{
  m_ButtonType = button;

  if (button == ButtonType::ReportProblem)
  {
    SetIconPath(":/EditorFramework/Icons/GitHub-Light.svg");
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
