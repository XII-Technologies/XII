#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetImportDlg.h>
#include <QDialog>

class xiiQtAssetImportDlg : public QDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  xiiQtAssetImportDlg(QWidget* pParent, xiiDynamicArray<xiiAssetDocumentGenerator::ImportData>& ref_allImports);
  ~xiiQtAssetImportDlg();

private Q_SLOTS:
  void SelectedOptionChanged(int index);
  void on_ButtonImport_clicked();
  void TableCellChanged(int row, int column);
  void BrowseButtonClicked(bool);

private:
  void InitRow(xiiUInt32 uiRow);
  void UpdateRow(xiiUInt32 uiRow);
  void QueryRow(xiiUInt32 uiRow);
  void UpdateAllRows();

  xiiDynamicArray<xiiAssetDocumentGenerator::ImportData>& m_AllImports;
};
