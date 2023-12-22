#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_AssetBrowserDlg.h>
#include <QDialog>

class xiiQtAssetBrowserDlg : public QDialog, public Ui_AssetBrowserDlg
{
  Q_OBJECT

public:
  xiiQtAssetBrowserDlg(QWidget* pParent, const xiiUuid& preselectedAsset, xiiStringView sVisibleFilters);
  xiiQtAssetBrowserDlg(QWidget* pParent, xiiStringView sWindowTitle, xiiStringView sPreselectedFileAbs, xiiStringView sFileExtensions);
  ~xiiQtAssetBrowserDlg();

  xiiStringView GetSelectedAssetPathRelative() const { return m_sSelectedAssetPathRelative; }
  xiiStringView GetSelectedAssetPathAbsolute() const { return m_sSelectedAssetPathAbsolute; }
  const xiiUuid GetSelectedAssetGuid() const { return m_SelectedAssetGuid; }

private Q_SLOTS:
  void on_AssetBrowserWidget_ItemChosen(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, xiiUInt8 uiAssetBrowserItemFlags);
  void on_AssetBrowserWidget_ItemSelected(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, xiiUInt8 uiAssetBrowserItemFlags);
  void on_AssetBrowserWidget_ItemCleared();
  void on_ButtonSelect_clicked();

private:
  void Init(QWidget* pParent);

  xiiString m_sSelectedAssetPathRelative;
  xiiString m_sSelectedAssetPathAbsolute;
  xiiUuid   m_SelectedAssetGuid;
  xiiString m_sVisibleFilters;

  static bool                         s_bShowItemsInSubFolder;
  static bool                         s_bShowItemsInHiddenFolder;
  static bool                         s_bSortByRecentUse;
  static xiiMap<xiiString, xiiString> s_TextFilter;
  static xiiMap<xiiString, xiiString> s_PathFilter;
  static xiiMap<xiiString, xiiString> s_TypeFilter;
};
