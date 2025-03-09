#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CreateProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

xiiQtCreateProjectDlg::xiiQtCreateProjectDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

  Prev->setVisible(false);

  m_sTargetFolder = xiiApplicationServices::GetSingleton()->GetSampleProjectsFolder().GetData();

  xiiQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(xiiOSFile::GetApplicationDirectory());
  m_LocalPluginSet = xiiQtEditorApp::GetSingleton()->GetPluginBundles();
  m_LocalPluginSet.SetFromTemplate("General3D");

  Plugins->SetPluginSet(&m_LocalPluginSet);
  Plugins->SelectTemplate("General3D");

  ProjectTemplates->setResizeMode(QListView::ResizeMode::Adjust);
  ProjectTemplates->setIconSize(QSize(220, 220));
  ProjectTemplates->setItemAlignment(Qt::AlignHCenter | Qt::AlignBottom);

  UpdateUI();

  FillProjectTemplatesList();
}

xiiString xiiQtCreateProjectDlg::GetFullTargetPath() const
{
  xiiStringBuilder name = m_sTargetName;

  name.Trim();

  if (name.IsEmpty())
    return {};

  xiiStringBuilder path;
  path.SetPath(m_sTargetFolder, m_sTargetName);

  return path;
}

void xiiQtCreateProjectDlg::UpdateUI()
{
  xiiQtScopedBlockSignals _1(ProjectFolder);
  xiiQtScopedBlockSignals _2(ProjectName);

  ProjectFolder->setText(m_sTargetFolder.GetData());

  if (m_sProjectTemplate.IsEmpty())
    ChosenTemplate->setText("<none>");
  else
  {
    xiiStringBuilder tmp = m_sProjectTemplate;
    tmp.PathParentDirectory();
    tmp.TrimRight("/\\");

    ChosenTemplate->setText(xiiMakeQString(tmp.GetFileName()));
  }

  xiiString sFullPath = GetFullTargetPath();

  if (sFullPath.IsEmpty() || !sFullPath.IsAbsolutePath())
  {
    ResultPath->setText("<Choose a name and parent folder>");
    Next->setEnabled(false);
  }
  else if (xiiOSFile::ExistsDirectory(sFullPath))
  {
    // ResultPath->setColor(qRgb(255, 0, 0));
    ResultPath->setText("Directory already exists");
    Next->setEnabled(false);
  }
  else
  {
    // ResultPath->setColor(qRgb(0, 255, 0));
    ResultPath->setText(sFullPath.GetData());
    ResultPath2->setText(sFullPath.GetData());
    Next->setEnabled(!sFullPath.IsEmpty());
  }

  switch (m_State)
  {
    case State::Basics:
      StackedPages->setCurrentIndex(0);
      Prev->setVisible(false);
      Next->setText("Next >");
      break;

    case State::Templates:
      StackedPages->setCurrentIndex(2);
      Prev->setVisible(true);
      Next->setText("Next >");
      break;

    case State::Plugins:
      StackedPages->setCurrentIndex(1);
      Prev->setVisible(true);
      Next->setText("Next >");
      break;

    case State::Summary:
      StackedPages->setCurrentIndex(3);
      Prev->setVisible(true);
      Next->setText("Create");
      break;

    case State::Create:
      break;
  }
}

void xiiQtCreateProjectDlg::FindProjectTemplates(xiiDynamicArray<xiiString>& out_Projects)
{
  out_Projects.Clear();

  xiiStringBuilder sTemplatesFolder = xiiApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sTemplatesFolder.AppendPath("ProjectTemplates");

  xiiFileSystemIterator fsIt;
  fsIt.StartSearch(sTemplatesFolder, xiiFileSystemIteratorFlags::ReportFolders);

  xiiStringBuilder path;

  while (fsIt.IsValid())
  {
    fsIt.GetStats().GetFullPath(path);
    path.AppendPath("xiiProject");

    if (xiiOSFile::ExistsFile(path))
    {
      out_Projects.PushBack(path);
    }

    fsIt.Next();
  }
}

void xiiQtCreateProjectDlg::FillProjectTemplatesList()
{
  xiiHybridArray<xiiString, 32> templates;
  FindProjectTemplates(templates);

  ProjectTemplates->clear();

  xiiStringBuilder tmp, iconPath;

  xiiStringBuilder samplesIcon = xiiApplicationServices::GetSingleton()->GetApplicationDataFolder();
  samplesIcon.AppendPath("ProjectTemplates/Thumbnail.jpg");

  QIcon fallbackIcon;

  if (xiiOSFile::ExistsFile(samplesIcon))
  {
    fallbackIcon.addFile(samplesIcon.GetData());
  }

  {
    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setText("Blank Project");
    pItem->setData(Qt::UserRole, QString());

    pItem->setIcon(fallbackIcon);

    ProjectTemplates->addItem(pItem);

    pItem->setSelected(true);
  }

  for (const xiiString& path : templates)
  {
    tmp                 = path;
    const bool bIsLocal = tmp.TrimWordEnd("/xiiProject");
    // const bool bIsRemote = tmp.TrimWordEnd("/xiiRemoteProject");

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

    ProjectTemplates->addItem(pItem);
  }
}

void xiiQtCreateProjectDlg::on_BrowseFolder_clicked()
{
  QString sFile = QFileDialog::getExistingDirectory(QApplication::activeWindow(), "Choose Folder", m_sTargetFolder.GetData(), QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  m_sTargetFolder = sFile.toUtf8().data();

  UpdateUI();
}

void xiiQtCreateProjectDlg::on_ProjectName_textChanged(QString text)
{
  m_sTargetName = ProjectName->text().toUtf8().data();

  UpdateUI();
}

void xiiQtCreateProjectDlg::on_Prev_clicked()
{
  switch (m_State)
  {
    case State::Templates:
      m_State = State::Basics;
      break;

    case State::Plugins:
      m_State = State::Templates;
      break;

    case State::Summary:
      if (m_sProjectTemplate.IsEmpty())
        m_State = State::Plugins;
      else
        m_State = State::Templates;
      break;

    default:
      break;
  }

  UpdateUI();
}

void xiiQtCreateProjectDlg::on_Next_clicked()
{
  switch (m_State)
  {
    case State::Basics:
      m_State = State::Templates;
      break;

    case State::Templates:
    {
      m_sProjectTemplate = ProjectTemplates->currentItem()->data(Qt::UserRole).toString().toUtf8().data();

      if (m_sProjectTemplate.IsEmpty())
        m_State = State::Plugins;
      else
        m_State = State::Summary;

      break;
    }

    case State::Plugins:
      Plugins->SyncStateToSet();
      m_State = State::Summary;
      break;

    case State::Summary:
      m_State = State::Create;
      break;
    case State::Create:
      break;
  }

  UpdateUI();

  if (m_State == State::Create)
  {
    CreateProject();

    QDialog::accept();
  }
}

void xiiQtCreateProjectDlg::CreateProject()
{
  const xiiString sFullPath = GetFullTargetPath();

  if (xiiOSFile::CreateDirectoryStructure(sFullPath).Failed())
  {
    // TODO
  }

  if (m_sProjectTemplate.IsEmpty())
  {
    // set up plugin selection
    xiiStringBuilder path = sFullPath;
    path.AppendPath("Editor/PluginSelection.ddl");

    xiiFileWriter file;
    file.Open(path).AssertSuccess();

    xiiOpenDdlWriter ddl;
    ddl.SetOutputStream(&file);

    m_LocalPluginSet.WriteStateToDDL(ddl);
  }
  else
  {
    // copy over project template

    xiiStringBuilder srcFolder = m_sProjectTemplate;
    srcFolder.PathParentDirectory();
    if (xiiOSFile::CopyFolder(srcFolder, sFullPath).Failed())
    {
      // TODO
    }
  }
}
