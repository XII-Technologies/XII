#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>

XII_IMPLEMENT_SINGLETON(xiiQtAssetBrowserPanel);

xiiQtAssetBrowserPanel::xiiQtAssetBrowserPanel() :
  xiiQtApplicationPanel("Panel.AssetBrowser"), m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  m_pStatusBar = new QStatusBar(nullptr);
  m_pStatusBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  m_pStatusBar->setSizeGripEnabled(false);

  m_pCuratorControl = new xiiQtCuratorControl(nullptr);

  m_pStatusBar->addPermanentWidget(m_pCuratorControl);

  dockWidgetContents->layout()->addWidget(m_pStatusBar);
  setWidget(pDummy);

  setIcon(xiiQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/Asset16.png"));
  setWindowTitle(QString::fromUtf8(xiiTranslate("Panel.AssetBrowser")));

  XII_VERIFY(connect(AssetBrowserWidget, &xiiQtAssetBrowserWidget::ItemChosen, this, &xiiQtAssetBrowserPanel::SlotAssetChosen) != nullptr,
             "signal/slot connection failed");
  XII_VERIFY(connect(AssetBrowserWidget, &xiiQtAssetBrowserWidget::ItemSelected, this, &xiiQtAssetBrowserPanel::SlotAssetSelected) != nullptr,
             "signal/slot connection failed");
  XII_VERIFY(connect(AssetBrowserWidget, &xiiQtAssetBrowserWidget::ItemCleared, this, &xiiQtAssetBrowserPanel::SlotAssetCleared) != nullptr,
             "signal/slot connection failed");

  AssetBrowserWidget->RestoreState("AssetBrowserPanel2");
}

xiiQtAssetBrowserPanel::~xiiQtAssetBrowserPanel()
{
  AssetBrowserWidget->SaveState("AssetBrowserPanel2");
}

void xiiQtAssetBrowserPanel::SlotAssetChosen(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(sAssetPathAbsolute.toUtf8().data());
}

void xiiQtAssetBrowserPanel::SlotAssetSelected(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  m_LastSelected = guid;
}

void xiiQtAssetBrowserPanel::SlotAssetCleared()
{
  m_LastSelected.SetInvalid();
}
