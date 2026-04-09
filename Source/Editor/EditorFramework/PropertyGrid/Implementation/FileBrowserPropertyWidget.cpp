#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/QtFileLineEdit.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

xiiQtFilePropertyWidget::xiiQtFilePropertyWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new xiiQtFileLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  XII_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

  {
    QMenu* pMenu = new QMenu();

    pMenu->setDefaultAction(pMenu->addAction(QIcon(), QLatin1String("Select File"), this, SLOT(on_BrowseFile_clicked())));
    QAction* pActionOpenFile = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open File"), this, SLOT(OnOpenFile()));
    QAction* pActionOpenWith = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open With..."), this, SLOT(OnOpenFileWith()));

    pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()));

    connect(pMenu, &QMenu::aboutToShow, pMenu, [=]() {
      pActionOpenFile->setEnabled(!m_pWidget->text().isEmpty());
      pActionOpenWith->setEnabled(!m_pWidget->text().isEmpty());
      //
    });

    m_pButton->setMenu(pMenu);
  }

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
}

bool xiiQtFilePropertyWidget::IsValidFileReference(xiiStringView sFile) const
{
  auto pAttr = m_pProp->GetAttributeByType<xiiFileBrowserAttribute>();

  xiiHybridArray<xiiStringView, 8> extensions;
  xiiStringView                    sTemp = pAttr->GetTypeFilter();
  sTemp.Split(false, extensions, ";");
  for (xiiStringView& ext : extensions)
  {
    ext.TrimWordStart("*.");
    if (sFile.GetFileExtension().IsEqual_NoCase(ext))
      return true;
  }

  return false;
}

void xiiQtFilePropertyWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  m_pWidget->setReadOnly(bReadOnly);
}

void xiiQtFilePropertyWidget::OnInit()
{
  auto pAttr = m_pProp->GetAttributeByType<xiiFileBrowserAttribute>();
  XII_ASSERT_DEV(pAttr != nullptr, "xiiQtFilePropertyWidget was created without a xiiFileBrowserAttribute!");

  if (!pAttr->GetCreateTitle().IsEmpty())
  {
    m_pButton->menu()->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DocumentAdd.svg")), QString("Create %1...").arg(xiiMakeQString(pAttr->GetCreateTitle())), this, SLOT(OnCreateFile()));
  }

  if (!pAttr->GetCustomAction().IsEmpty())
  {
    m_pButton->menu()->addAction(QIcon(), xiiMakeQString(xiiTranslate(pAttr->GetCustomAction())), this, SLOT(OnCustomAction()));
  }
}

void xiiQtFilePropertyWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);
  xiiQtScopedBlockSignals b2(m_pButton);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    xiiStringBuilder sText = value.ConvertTo<xiiString>();

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void xiiQtFilePropertyWidget::on_TextFinished_triggered()
{
  xiiStringBuilder sText = m_pWidget->text().toUtf8().data();

  BroadcastValueChanged(sText.GetData());
}

void xiiQtFilePropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
  {
    on_TextFinished_triggered();
  }
}

void xiiQtFilePropertyWidget::OnOpenExplorer()
{
  xiiString sPath = m_pWidget->text().toUtf8().data();
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  xiiQtUiServices::OpenInExplorer(sPath, true);
}

void xiiQtFilePropertyWidget::OnCustomAction()
{
  auto pAttr = m_pProp->GetAttributeByType<xiiFileBrowserAttribute>();

  if (pAttr->GetCustomAction() == nullptr)
    return;

  auto it = xiiDocumentManager::s_CustomActions.Find(pAttr->GetCustomAction());

  if (!it.IsValid())
    return;

  xiiVariant res = it.Value()(m_pGrid->GetDocument());

  if (!res.IsValid() || !res.IsA<xiiString>())
    return;

  m_pWidget->setText(xiiMakeQString(res.Get<xiiString>()));
  on_TextFinished_triggered();
}

void xiiQtFilePropertyWidget::OnOpenFile()
{
  xiiString sPath = m_pWidget->text().toUtf8().data();
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  if (!xiiQtUiServices::OpenFileInDefaultProgram(sPath))
  {
    xiiQtUiServices::MessageBoxInformation(xiiFmt("File could not be opened:\n{0}\nCheck that the file exists, that a program is associated with this file type and that access to this file is not denied.", sPath));
  }
}

void xiiQtFilePropertyWidget::OnOpenFileWith()
{
  xiiString sPath = m_pWidget->text().toUtf8().data();
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  xiiQtUiServices::OpenWith(sPath);
}

void xiiQtFilePropertyWidget::OnCreateFile()
{
  static QString sLastDir;
  if (sLastDir.isEmpty())
  {
    sLastDir = xiiToolsProject::GetSingleton()->GetProjectDirectory().GetData();
  }

  const xiiFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<xiiFileBrowserAttribute>();

  const QString sTitle = QString("Create %1").arg(xiiMakeQString(pFileAttribute->GetCreateTitle()));
  const QString sExt   = QString("%1 %2").arg(xiiMakeQString(pFileAttribute->GetCreateTitle())).arg(xiiMakeQString(pFileAttribute->GetTypeFilter()));

  QString sResult = QFileDialog::getSaveFileName(this, sTitle, sLastDir, sExt, nullptr);

  if (sResult.isEmpty())
    return;

  xiiStringBuilder sPath = sResult.toUtf8().data();

  if (!xiiOSFile::ExistsFile(sPath))
  {
    xiiOSFile file;
    file.Open(sPath, xiiFileOpenMode::Write).IgnoreResult();
  }

  if (!xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath))
    return;

  m_pWidget->setText(xiiMakeQString(sPath));
  on_TextFinished_triggered();
}

static xiiMap<xiiString, xiiString> s_StartDirs;

void xiiQtFilePropertyWidget::on_BrowseFile_clicked()
{
  xiiString                      sFile          = m_pWidget->text().toUtf8().data();
  const xiiFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<xiiFileBrowserAttribute>();

  auto& sStartDir = s_StartDirs[pFileAttribute->GetTypeFilter()];

  if (!sFile.IsEmpty())
  {
    xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sFile);

    xiiStringBuilder st = sFile;
    st                  = st.GetFileDirectory();

    sStartDir = st;
  }

  if (sStartDir.IsEmpty())
    sStartDir = xiiToolsProject::GetSingleton()->GetProjectFile();

  xiiQtAssetBrowserDlg dlg(this, pFileAttribute->GetDialogTitle(), sFile, pFileAttribute->GetTypeFilter());
  if (dlg.exec() == QDialog::Rejected)
    return;

  xiiStringView sResult = dlg.GetSelectedAssetPathRelative();

  if (sResult.IsEmpty())
    return;

  // the returned path is a "datadir parent relative path" and we must remove the first folder
  if (const char* nextSep = sResult.FindSubString("/"))
  {
    sResult.SetStartPosition(nextSep + 1);
  }

  sStartDir = sResult;

  m_pWidget->setText(xiiMakeQString(sResult));
  on_TextFinished_triggered();
}

//////////////////////////////////////////////////////////////////////////

xiiQtExternalFilePropertyWidget::xiiQtExternalFilePropertyWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  XII_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

  {
    QMenu* pMenu = new QMenu();

    pMenu->setDefaultAction(pMenu->addAction(QIcon(), QLatin1String("Select File"), this, SLOT(on_BrowseFile_clicked())));
    QAction* pActionOpenFile = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open File"), this, SLOT(OnOpenFile()));
    QAction* pActionOpenWith = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open With..."), this, SLOT(OnOpenFileWith()));
    pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()));

    connect(pMenu, &QMenu::aboutToShow, pMenu, [=]() {
      pActionOpenFile->setEnabled(!m_pWidget->text().isEmpty());
      pActionOpenWith->setEnabled(!m_pWidget->text().isEmpty());
      //
    });
    m_pButton->setMenu(pMenu);
  }

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
}

bool xiiQtExternalFilePropertyWidget::IsValidFileReference(xiiStringView sFile) const
{
  auto pAttr = m_pProp->GetAttributeByType<xiiExternalFileBrowserAttribute>();

  xiiHybridArray<xiiStringView, 8> extensions;
  xiiStringView                    sTemp = pAttr->GetTypeFilter();
  sTemp.Split(false, extensions, ";");
  for (xiiStringView& ext : extensions)
  {
    ext.TrimWordStart("*.");
    if (sFile.GetFileExtension().IsEqual_NoCase(ext))
      return true;
  }

  return false;
}

void xiiQtExternalFilePropertyWidget::OnInit()
{
  auto pAttr = m_pProp->GetAttributeByType<xiiExternalFileBrowserAttribute>();
  XII_ASSERT_DEV(pAttr != nullptr, "xiiQtFilePropertyWidget was created without a xiiExternalFileBrowserAttribute!");
}

void xiiQtExternalFilePropertyWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);
  xiiQtScopedBlockSignals b2(m_pButton);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    xiiStringBuilder sText = value.ConvertTo<xiiString>();

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void xiiQtExternalFilePropertyWidget::on_TextFinished_triggered()
{
  xiiStringBuilder sText = m_pWidget->text().toUtf8().data();

  BroadcastValueChanged(sText.GetData());
}

void xiiQtExternalFilePropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void xiiQtExternalFilePropertyWidget::OnOpenExplorer()
{
  xiiString sPath = m_pWidget->text().toUtf8().data();
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  xiiQtUiServices::OpenInExplorer(sPath, true);
}

void xiiQtExternalFilePropertyWidget::OnOpenFile()
{
  xiiString sPath = m_pWidget->text().toUtf8().data();
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  if (!xiiQtUiServices::OpenFileInDefaultProgram(sPath))
    xiiQtUiServices::MessageBoxInformation(xiiFmt("File could not be opened:\n{0}\nCheck that the file exists, that a program is associated with this file type and that access to this file is not denied.", sPath));
}

void xiiQtExternalFilePropertyWidget::OnOpenFileWith()
{
  xiiString sPath = m_pWidget->text().toUtf8().data();
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  xiiQtUiServices::OpenWith(sPath);
}

void xiiQtExternalFilePropertyWidget::on_BrowseFile_clicked()
{
  xiiString                              sFile          = m_pWidget->text().toUtf8().data();
  const xiiExternalFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<xiiExternalFileBrowserAttribute>();

  auto& sStartDir = s_StartDirs[pFileAttribute->GetTypeFilter()];

  if (sStartDir.IsEmpty())
  {
    sStartDir = sFile.GetFileDirectory();
  }

  if (sStartDir.IsEmpty())
  {
    sStartDir = xiiToolsProject::GetSingleton()->GetProjectFile();
  }

  QString sResult = QFileDialog::getOpenFileName(this, xiiMakeQString(pFileAttribute->GetDialogTitle()), sStartDir.GetData(), xiiMakeQString(pFileAttribute->GetTypeFilter()), nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sResult.isEmpty())
    return;

  sFile = sResult.toUtf8().data();

  // doesn't matter if this fails
  xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);

  sStartDir = sFile.GetFileDirectory();

  m_pWidget->setText(sResult);
  on_TextFinished_triggered();
}
