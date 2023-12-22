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
  return ""; //:/GuiFoundation/XII-logo.svg";
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

  pSettingsTab->EnsureVisible();
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
  setCentralWidget(new QWidget());
  XII_ASSERT_DEV(centralWidget() != nullptr, "");

  setupUi(centralWidget());
  QMetaObject::connectSlotsByName(this);

  xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
  xiiActionContext           context;
  context.m_sMapping  = "SettingsTabMenuBar";
  context.m_pDocument = nullptr;
  pMenuBar->SetActionContext(context);

  FinishWindowCreation();

  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtSettingsTab::ToolsProjectEventHandler, this));
}

xiiQtSettingsTab::~xiiQtSettingsTab()
{
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtSettingsTab::ToolsProjectEventHandler, this));
}

void xiiQtSettingsTab::on_OpenScene_clicked()
{
  xiiQtAssetBrowserDlg dlg(this, xiiUuid(), "Scene");
  if (dlg.exec() == 0)
    return;

  xiiQtEditorApp::GetSingleton()->OpenDocument(dlg.GetSelectedAssetPathAbsolute(), xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList);
}

void xiiQtSettingsTab::on_OpenProject_clicked()
{
  xiiQtDashboardDlg dlg(nullptr, xiiQtDashboardDlg::DashboardTab::Samples);
  dlg.exec();
}

void xiiQtSettingsTab::on_GettingStarted_clicked()
{
  QDesktopServices::openUrl(QUrl("https://xiiengine.net/pages/getting-started/editor-overview.html"));
}

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

void xiiQtSettingsTab::ToolsProjectEventHandler(const xiiToolsProjectEvent& e)
{
  if (e.m_Type == xiiToolsProjectEvent::Type::ProjectClosed || e.m_Type == xiiToolsProjectEvent::Type::ProjectCreated || e.m_Type == xiiToolsProjectEvent::Type::ProjectOpened)
  {
    xiiStringBuilder txt = "<html><head/><body><p align=\"center\"><span style=\" font-size:18pt;\">Open Project:</span></p><p align=\"center\"><span style=\" font-size:18pt;\">None</span></p></body></html>";

    if (xiiToolsProject::GetSingleton()->IsProjectOpen())
    {
      txt.ReplaceAll("None", xiiToolsProject::GetSingleton()->GetProjectName(false));
      OpenScene->setVisible(true);
    }
    else
    {
      txt = "<html><head/><body><p align=\"center\"><span style=\" font-size:18pt;\">No Project Open</span></p></body></html>";
      OpenScene->setVisible(false);
    }

    ProjectLabel->setText(txt.GetData());
  }
}
