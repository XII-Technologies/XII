/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

xiiQtDataDirsDlg::xiiQtDataDirsDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

  m_Config     = xiiQtEditorApp::GetSingleton()->GetFileSystemConfig();
  m_iSelection = -1;
  FillList();
}

void xiiQtDataDirsDlg::FillList()
{
  if (m_Config.m_DataDirs.IsEmpty())
    m_iSelection = -1;

  if (m_iSelection != -1)
    m_iSelection = xiiMath::Clamp<xiiInt32>(m_iSelection, 0, m_Config.m_DataDirs.GetCount() - 1);

  ListDataDirs->blockSignals(true);

  ListDataDirs->clear();

  for (auto dd : m_Config.m_DataDirs)
  {
    QListWidgetItem* pItem = new QListWidgetItem(ListDataDirs);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable /*| Qt::ItemFlag::ItemIsUserCheckable*/);

    QString sPath = QString::fromUtf8(dd.m_sDataDirSpecialPath.GetData());

    pItem->setText(sPath);
    ListDataDirs->addItem(pItem);

    if (dd.m_bHardCodedDependency)
    {
      QColor col = QColor::fromString("Orange");
      pItem->setForeground(col);
      pItem->setToolTip("This data directory is a hard dependency and cannot be removed.");
      pItem->setData(Qt::UserRole + 1, false); // can remove ?
    }
    else
    {
      pItem->setData(Qt::UserRole + 1, true); // can remove ?
    }
  }

  ListDataDirs->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  if (m_iSelection == -1)
    ListDataDirs->clearSelection();
  else
    ListDataDirs->item(m_iSelection)->setSelected(true);


  ListDataDirs->blockSignals(false);

  on_ListDataDirs_itemSelectionChanged();
}

void xiiQtDataDirsDlg::on_ButtonOK_clicked()
{
  if (m_Config.CreateDataDirStubFiles().Failed())
  {
    xiiQtUiServices::MessageBoxWarning("Failed to create all data dir stub files ('DataDir.xiiManifest'). Please review the selected "
                                       "folders, some might not be accessible. See the log for more details.");
    return;
  }

  xiiQtEditorApp::GetSingleton()->SetFileSystemConfig(m_Config);
  accept();
}

void xiiQtDataDirsDlg::on_ButtonCancel_clicked()
{
  reject();
}

void xiiQtDataDirsDlg::on_ButtonUp_clicked()
{
  xiiMath::Swap(m_Config.m_DataDirs[m_iSelection - 1], m_Config.m_DataDirs[m_iSelection]);
  --m_iSelection;

  FillList();
}

void xiiQtDataDirsDlg::on_ButtonDown_clicked()
{
  xiiMath::Swap(m_Config.m_DataDirs[m_iSelection], m_Config.m_DataDirs[m_iSelection + 1]);
  ++m_iSelection;

  FillList();
}

void xiiQtDataDirsDlg::on_ButtonAdd_clicked()
{
  static QString sPreviousFolder;
  if (sPreviousFolder.isEmpty())
  {
    sPreviousFolder = QString::fromUtf8(xiiToolsProject::GetSingleton()->GetProjectFile().GetData());
  }

  QString sFolder = QFileDialog::getExistingDirectory(this, QLatin1String("Select Directory"), sPreviousFolder, QFileDialog::Option::ShowDirsOnly | QFileDialog::Option::DontResolveSymlinks);

  if (sFolder.isEmpty())
    return;

  sPreviousFolder = sFolder;

  xiiStringBuilder sRootPath = xiiFileSystem::GetSdkRootDirectory();

  xiiStringBuilder sRelPath = sFolder.toUtf8().data();
  sRelPath.MakeRelativeTo(sRootPath).IgnoreResult();
  sRelPath.Prepend(">sdk/");
  sRelPath.MakeCleanPath();

  xiiApplicationFileSystemConfig::DataDirConfig dd;
  dd.m_sDataDirSpecialPath = sRelPath;
  dd.m_bWritable           = false;
  m_Config.m_DataDirs.PushBack(dd);

  m_iSelection = m_Config.m_DataDirs.GetCount() - 1;

  FillList();
}

void xiiQtDataDirsDlg::on_ButtonRemove_clicked()
{
  m_Config.m_DataDirs.RemoveAtAndCopy(m_iSelection);

  FillList();
}

void xiiQtDataDirsDlg::on_ListDataDirs_itemSelectionChanged()
{
  if (ListDataDirs->selectedItems().isEmpty())
    m_iSelection = -1;
  else
    m_iSelection = ListDataDirs->selectionModel()->selectedIndexes()[0].row();

  const bool bCanRemove = m_iSelection >= 0 && ListDataDirs->item(m_iSelection)->data(Qt::UserRole + 1).toBool();

  ButtonRemove->setEnabled(bCanRemove);
  ButtonUp->setEnabled(m_iSelection > 0);
  ButtonDown->setEnabled(m_iSelection != -1 && m_iSelection < (xiiInt32)m_Config.m_DataDirs.GetCount() - 1);
}

void xiiQtDataDirsDlg::on_ButtonOpenFolder_clicked()
{
  if (m_iSelection < 0)
    return;

  xiiStringBuilder sPath;
  xiiFileSystem::ResolveSpecialDirectory(m_Config.m_DataDirs[m_iSelection].m_sDataDirSpecialPath, sPath).IgnoreResult();

  xiiQtUiServices::OpenInExplorer(sPath.GetData(), true);
}

void xiiQtDataDirsDlg::on_ListDataDirs_itemDoubleClicked(QListWidgetItem* pItem)
{
  on_ButtonOpenFolder_clicked();
}
