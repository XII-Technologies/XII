#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <QDesktopServices>

XII_IMPLEMENT_SINGLETON(xiiQtSettingsTab);

xiiString xiiQtSettingsTab::GetWindowIcon() const
{
  return ""; //:/GuiFoundation/XII-Logo.svg";
}

xiiString xiiQtSettingsTab::GetDisplayNameShort() const
{
  return "";
}

void xiiQtEditorApp::ShowSettingsDocument()
{
  xiiQtSettingsTab* pSettingsTab = xiiQtSettingsTab::GetSingleton();

  if (pSettingsTab == nullptr)
  {
    pSettingsTab = new xiiQtSettingsTab();
  }
}

void xiiQtEditorApp::CloseSettingsDocument()
{
  xiiQtSettingsTab* pSettingsTab = xiiQtSettingsTab::GetSingleton();

  if (pSettingsTab != nullptr)
  {
    pSettingsTab->CloseDocumentWindow();
  }
}

xiiQtSettingsTab::xiiQtSettingsTab() :
  xiiQtDocumentWindow("Settings"), m_SingletonRegistrar(this)
{
  xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
  xiiActionContext           context;
  context.m_sMapping  = "SettingsTabMenuBar";
  context.m_pDocument = nullptr;
  pMenuBar->SetActionContext(context);

  FinishWindowCreation();
}

xiiQtSettingsTab::~xiiQtSettingsTab() = default;

bool xiiQtSettingsTab::InternalCanCloseWindow()
{
  // if this is the last window, prevent closing it
  return xiiQtDocumentWindow::GetAllDocumentWindows().GetCount() > 1;
}

void xiiQtSettingsTab::InternalCloseDocumentWindow()
{
  // make sure this instance isn't used anymore
  UnregisterSingleton();
}
