/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

xiiQtDashboardDlg::xiiQtDashboardDlg(QWidget* pParent, DashboardTab activeTab) :
  QDialog(pParent)
{
  setupUi(this);

  TabArea->tabBar()->hide();

  ProjectsTab->setFlat(true);
  SamplesTab->setFlat(true);
  DocumentationTab->setFlat(true);

  if (xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>())
  {
    LoadLastProject->setChecked(pPreferences->m_bLoadLastProjectAtStartup);
  }

  {
    SamplesList->setResizeMode(QListView::ResizeMode::Adjust);
    SamplesList->setIconSize(QSize(220, 220));
    SamplesList->setItemAlignment(Qt::AlignHCenter | Qt::AlignBottom);
  }

  FillRecentProjectsList();
  FillSampleProjectsList();

  ProjectsList->installEventFilter(this);

  if (ProjectsList->rowCount() > 0)
  {
    ProjectsList->setFocus();
    ProjectsList->clearSelection();
    ProjectsList->selectRow(0);
  }
  else
  {
    if (activeTab == DashboardTab::Projects)
    {
      activeTab = DashboardTab::Samples;
    }
  }

  SetActiveTab(activeTab);
}

void xiiQtDashboardDlg::SetActiveTab(DashboardTab activeTab)
{
  TabArea->setCurrentIndex((int)activeTab);

  ProjectsTab->setChecked(activeTab == DashboardTab::Projects);
  SamplesTab->setChecked(activeTab == DashboardTab::Samples);
  DocumentationTab->setChecked(activeTab == DashboardTab::Documentation);
}

void xiiQtDashboardDlg::FillRecentProjectsList()
{
  const auto& list = xiiQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList();

  ProjectsList->clear();
  ProjectsList->setColumnCount(2);
  ProjectsList->setRowCount(list.GetCount());

  xiiStringBuilder tmp;

  for (xiiUInt32 r = 0; r < list.GetCount(); ++r)
  {
    const auto& path = list[r];

    QTableWidgetItem* pItemProjectName = new QTableWidgetItem();
    QTableWidgetItem* pItemProjectPath = new QTableWidgetItem();

    pItemProjectName->setData(Qt::UserRole, path.m_File.GetData());

    tmp = path.m_File;
    tmp.MakeCleanPath();
    tmp.PathParentDirectory(1); // remove '/xiiProject'
    tmp.Trim("/");

    pItemProjectPath->setText(tmp.GetData());
    pItemProjectName->setText(tmp.GetFileName().GetStartPointer());

    ProjectsList->setItem(r, 0, pItemProjectName);
    ProjectsList->setItem(r, 1, pItemProjectPath);
  }

  ProjectsList->resizeColumnToContents(0);
}

void xiiQtDashboardDlg::FillSampleProjectsList()
{
  xiiHybridArray<xiiString, 32> samples;
  FindSampleProjects(samples);

  SamplesList->clear();

  xiiStringBuilder tmp, iconPath;

  xiiStringBuilder samplesIcon = xiiApplicationServices::GetSingleton()->GetSampleProjectsFolder();
  samplesIcon.AppendPath("Thumbnail.jpg");

  QIcon fallbackIcon;

  if (xiiOSFile::ExistsFile(samplesIcon))
  {
    fallbackIcon.addFile(samplesIcon.GetData());
  }

  for (const xiiString& path : samples)
  {
    tmp                  = path;
    const bool bIsLocal  = tmp.TrimWordEnd("/xiiProject");
    const bool bIsRemote = tmp.TrimWordEnd("/xiiRemoteProject");

    QIcon projectIcon;

    iconPath = tmp;
    iconPath.AppendPath("Thumbnail.jpg");

    if (xiiOSFile::ExistsFile(iconPath))
    {
      projectIcon.addFile(iconPath.GetData());
    }
    else
    {
      projectIcon = fallbackIcon;
    }

    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setText(tmp.GetFileName().GetStartPointer());
    pItem->setData(Qt::UserRole, path.GetData());

    pItem->setIcon(projectIcon);

    SamplesList->addItem(pItem);
  }
}

void xiiQtDashboardDlg::FindSampleProjects(xiiDynamicArray<xiiString>& out_Projects)
{
  out_Projects.Clear();

  const xiiString& sSampleProjects = xiiApplicationServices::GetSingleton()->GetSampleProjectsFolder();

  xiiFileSystemIterator fsIt;
  fsIt.StartSearch(sSampleProjects, xiiFileSystemIteratorFlags::ReportFoldersRecursive);

  xiiStringBuilder path;

  while (fsIt.IsValid())
  {
    fsIt.GetStats().GetFullPath(path);
    path.AppendPath("xiiProject");

    if (xiiOSFile::ExistsFile(path))
    {
      out_Projects.PushBack(path);

      // no need to go deeper
      fsIt.SkipFolder();
    }
    else
    {
      fsIt.GetStats().GetFullPath(path);
      path.AppendPath("xiiRemoteProject");

      if (xiiOSFile::ExistsFile(path))
      {
        out_Projects.PushBack(path);

        // no need to go deeper
        fsIt.SkipFolder();
      }
      else
      {
        fsIt.Next();
      }
    }
  }
}

void xiiQtDashboardDlg::on_ProjectsTab_clicked()
{
  SetActiveTab(DashboardTab::Projects);
}

void xiiQtDashboardDlg::on_SamplesTab_clicked()
{
  SetActiveTab(DashboardTab::Samples);
}

void xiiQtDashboardDlg::on_DocumentationTab_clicked()
{
  SetActiveTab(DashboardTab::Documentation);
}

void xiiQtDashboardDlg::on_NewProject_clicked()
{
  if (xiiQtEditorApp::GetSingleton()->GuiCreateProject(true))
  {
    accept();
  }
}

void xiiQtDashboardDlg::on_BrowseProject_clicked()
{
  if (xiiQtEditorApp::GetSingleton()->GuiOpenProject(true))
  {
    accept();
  }
}

void xiiQtDashboardDlg::on_ProjectsList_cellDoubleClicked(int row, int column)
{
  if (row < 0 || row >= ProjectsList->rowCount())
    return;

  QTableWidgetItem* pItem = ProjectsList->item(row, 0);

  QString sPath = pItem->data(Qt::UserRole).toString();

  if (xiiQtEditorApp::GetSingleton()->OpenProject(sPath.toUtf8().data(), true).Succeeded())
  {
    accept();
  }
}

void xiiQtDashboardDlg::on_OpenProject_clicked()
{
  on_ProjectsList_cellDoubleClicked(ProjectsList->currentRow(), 0);
}

void xiiQtDashboardDlg::on_OpenSample_clicked()
{
  on_SamplesList_itemDoubleClicked(SamplesList->currentItem());
}

void xiiQtDashboardDlg::on_LoadLastProject_stateChanged(int)
{
  if (xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>())
  {
    pPreferences->m_bLoadLastProjectAtStartup = LoadLastProject->isChecked();
  }
}

void xiiQtDashboardDlg::on_SamplesList_itemDoubleClicked(QListWidgetItem* pItem)
{
  if (pItem == nullptr)
    return;

  QString sPath = pItem->data(Qt::UserRole).toString().toUtf8().data();

  if (xiiQtEditorApp::GetSingleton()->OpenProject(sPath.toUtf8().data(), true).Succeeded())
  {
    accept();
  }
}

void xiiQtDashboardDlg::on_OpenDocs_clicked()
{
  QDesktopServices::openUrl(QUrl("https://www.xiitechnologies.com"));
}

void xiiQtDashboardDlg::on_OpenApiDocs_clicked()
{
  QDesktopServices::openUrl(QUrl("https://xii-technologies.github.io/APIDocumentation/"));
}

void xiiQtDashboardDlg::on_GitHubDiscussions_clicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/XII-Technologies/XII/discussions"));
}

void xiiQtDashboardDlg::on_ReportProblem_clicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/XII-Technologies/XII/issues"));
}

void xiiQtDashboardDlg::on_OpenDiscord_clicked()
{
  QDesktopServices::openUrl(QUrl("https://discord.gg/qvfdhRQpHB"));
}

void xiiQtDashboardDlg::on_OpenTwitter_clicked()
{
  QDesktopServices::openUrl(QUrl("https://twitter.com/XII"));
}

bool xiiQtDashboardDlg::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::KeyPress)
  {
    QKeyEvent* key = static_cast<QKeyEvent*>(e);

    if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return))
    {
      on_OpenProject_clicked();
      return true;
    }
  }

  return QObject::eventFilter(obj, e);
}
