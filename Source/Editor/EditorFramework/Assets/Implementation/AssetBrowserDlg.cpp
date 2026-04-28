/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

bool                         xiiQtAssetBrowserDlg::s_bShowItemsInSubFolder    = true;
bool                         xiiQtAssetBrowserDlg::s_bShowItemsInHiddenFolder = false;
bool                         xiiQtAssetBrowserDlg::s_bSortByRecentUse         = true;
xiiMap<xiiString, xiiString> xiiQtAssetBrowserDlg::s_TextFilter;
xiiMap<xiiString, xiiString> xiiQtAssetBrowserDlg::s_PathFilter;
xiiMap<xiiString, xiiString> xiiQtAssetBrowserDlg::s_TypeFilter;

void ClampWindowGeometryToScreens(QRect& windowGeometry)
{
  const QList<QScreen*> screens = QGuiApplication::screens();

  for (QScreen* screen : screens)
  {
    const QRect screenGeom = screen->availableGeometry();
    if (screenGeom.intersects(windowGeometry))
      return;
  }

  const QRect primaryGeom = QGuiApplication::primaryScreen()->availableGeometry();

  const QSize size = windowGeometry.size();
  windowGeometry.setLeft(xiiMath::Clamp(windowGeometry.left(), primaryGeom.left(), primaryGeom.right() - windowGeometry.width()));
  windowGeometry.setTop(xiiMath::Clamp(windowGeometry.top(), primaryGeom.top(), primaryGeom.bottom() - windowGeometry.height()));
  windowGeometry.setSize(size);
}

void xiiQtAssetBrowserDlg::Init(QWidget* pParent)
{
  setupUi(this);

  ButtonSelect->setEnabled(false);

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());

    QRect windowGeometry;
    windowGeometry.setTopLeft(Settings.value("WindowPosition", pos()).toPoint());
    windowGeometry.setSize(Settings.value("WindowSize", size()).toSize());
    ClampWindowGeometryToScreens(windowGeometry);

    move(windowGeometry.topLeft());
    resize(windowGeometry.size());
  }
  Settings.endGroup();

  AssetBrowserWidget->RestoreState("AssetBrowserDlg");
  AssetBrowserWidget->GetAssetBrowserFilter()->SetSortByRecentUse(s_bSortByRecentUse);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInSubFolders(s_bShowItemsInSubFolder);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInHiddenFolders(s_bShowItemsInHiddenFolder);

  if (!s_TextFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTextFilter(s_TextFilter[m_sVisibleFilters]);

  if (!s_PathFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetPathFilter(s_PathFilter[m_sVisibleFilters]);

  if (!s_TypeFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTypeFilter(s_TypeFilter[m_sVisibleFilters]);
}

xiiQtAssetBrowserDlg::xiiQtAssetBrowserDlg(QWidget* pParent, const xiiUuid& preselectedAsset, xiiStringView sVisibleFilters, xiiStringView sWindowTitle, xiiStringView sRequiredTag) :
  QDialog(pParent)
{
  {
    xiiStringBuilder                 temp = sVisibleFilters;
    xiiHybridArray<xiiStringView, 4> compTypes;
    temp.Split(false, compTypes, ";");
    xiiStringBuilder allFiltered = sVisibleFilters;

    for (const auto& descIt : xiiAssetDocumentManager::GetAllDocumentDescriptors())
    {
      const xiiDocumentTypeDescriptor* pType = descIt.Value();
      for (xiiStringView ct : compTypes)
      {
        if (pType->m_CompatibleTypes.Contains(ct))
        {
          allFiltered.Append(";", pType->m_sDocumentTypeName, ";");
        }
      }
    }

    m_sVisibleFilters = allFiltered;
    m_sRequiredTag    = sRequiredTag;
  }
  Init(pParent);

  AssetBrowserWidget->SetMode(xiiQtAssetBrowserWidget::Mode::AssetPicker);

  if (m_sVisibleFilters != ";;") // that's an empty filter list
  {
    AssetBrowserWidget->ShowOnlyTheseTypeFilters(m_sVisibleFilters);
  }

  AssetBrowserWidget->SetRequiredTag(m_sRequiredTag);

  AssetBrowserWidget->SetSelectedAsset(preselectedAsset);

  AssetBrowserWidget->SearchWidget->setFocus();

  if (!sWindowTitle.IsEmpty())
  {
    setWindowTitle(xiiMakeQString(sWindowTitle));
  }
}

xiiQtAssetBrowserDlg::xiiQtAssetBrowserDlg(QWidget* pParent, xiiStringView sWindowTitle, xiiStringView sPreselectedFileAbs, xiiStringView sFileExtensions) :
  QDialog(pParent)
{
  m_sVisibleFilters = sFileExtensions;

  Init(pParent);

  xiiStringBuilder title(sFileExtensions, ")");
  title.ReplaceAll(";", "; ");
  title.ReplaceAll("  ", " ");
  title.PrependFormat("{} (", sWindowTitle);
  setWindowTitle(xiiMakeQString(title));

  AssetBrowserWidget->SetMode(xiiQtAssetBrowserWidget::Mode::FilePicker);
  AssetBrowserWidget->UseFileExtensionFilters(sFileExtensions);

  xiiStringBuilder sParentRelPath = sPreselectedFileAbs;
  if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sParentRelPath))
  {
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTemporaryPinnedItem(sParentRelPath);
  }

  AssetBrowserWidget->SetSelectedFile(sPreselectedFileAbs);

  AssetBrowserWidget->SearchWidget->setFocus();
}

xiiQtAssetBrowserDlg::~xiiQtAssetBrowserDlg()
{
  s_bShowItemsInSubFolder         = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInSubFolders();
  s_bShowItemsInHiddenFolder      = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInHiddenFolders();
  s_bSortByRecentUse              = AssetBrowserWidget->GetAssetBrowserFilter()->GetSortByRecentUse();
  s_TextFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTextFilter();
  s_PathFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetPathFilter();
  s_TypeFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTypeFilter();

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    Settings.setValue("WindowGeometry", saveGeometry());
    Settings.setValue("WindowPosition", pos());
    Settings.setValue("WindowSize", size());
  }
  Settings.endGroup();

  AssetBrowserWidget->SaveState("AssetBrowserDlg");
}

void xiiQtAssetBrowserDlg::on_AssetBrowserWidget_ItemSelected(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, xiiUInt8 uiAssetBrowserItemFlags)
{
  m_SelectedAssetGuid          = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  const xiiBitflags<xiiAssetBrowserItemFlags> flags = (xiiAssetBrowserItemFlags::Enum)uiAssetBrowserItemFlags;

  ButtonSelect->setEnabled(flags.IsAnySet(xiiAssetBrowserItemFlags::Asset | xiiAssetBrowserItemFlags::SubAsset | xiiAssetBrowserItemFlags::File));
}

void xiiQtAssetBrowserDlg::on_AssetBrowserWidget_ItemChosen(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, xiiUInt8 uiAssetBrowserItemFlags)
{
  m_SelectedAssetGuid          = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  accept();
}

void xiiQtAssetBrowserDlg::on_AssetBrowserWidget_ItemCleared()
{
  ButtonSelect->setEnabled(false);
}

void xiiQtAssetBrowserDlg::on_ButtonSelect_clicked()
{
  accept();
}
