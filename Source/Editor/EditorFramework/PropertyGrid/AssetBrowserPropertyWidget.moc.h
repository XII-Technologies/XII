#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/PropertyGrid/QtAssetLineEdit.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

#include <QLineEdit>
#include <QModelIndex>

/// *** Asset Browser ***

class XII_EDITORFRAMEWORK_DLL xiiQtAssetPropertyWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtAssetPropertyWidget();

  bool IsValidAssetType(const char* szAssetReference) const;

private Q_SLOTS:
  void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  void ThumbnailInvalidated(QString sPath, xiiUInt32 uiImageID);
  void OnOpenAssetDocument();
  void OnSelectInAssetBrowser();
  void OnOpenExplorer();
  void OnCopyAssetGuid();
  void OnCreateNewAsset();
  void OnClearReference();
  void OnShowMenu();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;
  virtual void showEvent(QShowEvent* event) override;
  void         FillAssetMenu(QMenu& menu);

protected:
  void UpdateThumbnail(const xiiUuid& guid, const char* szThumbnailPath);

  QPalette            m_Pal;
  QHBoxLayout*        m_pLayout;
  xiiQtAssetLineEdit* m_pWidget;
  QToolButton*        m_pButton;
  xiiUInt32           m_uiThumbnailID;
  xiiUuid             m_AssetGuid;
};
