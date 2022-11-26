#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>


xiiQtFilePropertyWidget::xiiQtFilePropertyWidget() :
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
    QAction* pDocAction = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open File"), this, SLOT(OnOpenFile())) /*->setEnabled(!m_pWidget->text().isEmpty())*/;
    pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()));

    connect(pMenu, &QMenu::aboutToShow, pMenu, [=]() { pDocAction->setEnabled(!m_pWidget->text().isEmpty()); });

    m_pButton->setMenu(pMenu);
  }

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
}

void xiiQtFilePropertyWidget::OnInit()
{
  auto pAttr = m_pProp->GetAttributeByType<xiiFileBrowserAttribute>();
  XII_ASSERT_DEV(pAttr != nullptr, "xiiQtFilePropertyWidget was created without a xiiFileBrowserAttribute!");

  if (!xiiStringUtils::IsNullOrEmpty(pAttr->GetCustomAction()))
  {
    m_pButton->menu()->addAction(QIcon(), xiiTranslate(pAttr->GetCustomAction()), this, SLOT(OnCustomAction()));
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
    on_TextFinished_triggered();
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

  m_pWidget->setText(res.Get<xiiString>().GetData());
  on_TextFinished_triggered();
}

void xiiQtFilePropertyWidget::OnOpenFile()
{
  xiiString sPath = m_pWidget->text().toUtf8().data();
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  if (!xiiQtUiServices::OpenFileInDefaultProgram(sPath))
    xiiQtUiServices::MessageBoxInformation(xiiFmt("File could not be opened:\n{0}\nCheck that the file exists, that a program is associated "
                                                  "with this file type and that access to this file is not denied.",
                                                  sPath));
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

  QString sResult = QFileDialog::getOpenFileName(this, pFileAttribute->GetDialogTitle(), sStartDir.GetData(), pFileAttribute->GetTypeFilter(), nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sResult.isEmpty())
    return;

  sFile     = sResult.toUtf8().data();
  sStartDir = sFile;

  if (!xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile))
  {
    xiiQtUiServices::GetSingleton()->MessageBoxInformation("The selected file is not under any data directory.\nPlease select another file "
                                                           "or copy it into one of the project's data directories.");
    return;
  }

  m_pWidget->setText(sFile.GetData());
  on_TextFinished_triggered();
}
