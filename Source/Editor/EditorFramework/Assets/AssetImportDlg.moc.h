#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetImportDlg.h>
#include <QDialog>

class xiiQtAssetImportDlg : public QDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  xiiQtAssetImportDlg(QWidget* pParent, xiiDynamicArray<xiiAssetDocumentGenerator::ImportGroupOptions>& ref_allImports);
  ~xiiQtAssetImportDlg();

private Q_SLOTS:
  void SelectedOptionChanged(int index);
  void on_ButtonImport_clicked();

private:
  void InitRow(xiiUInt32 uiRow);

  xiiDynamicArray<xiiAssetDocumentGenerator::ImportGroupOptions>& m_AllImports;
};
