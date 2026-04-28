/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ModifiedDocumentsDlg.h>
#include <QDialog>
#include <ToolsFoundation/Document/Document.h>

class XII_GUIFOUNDATION_DLL xiiQtModifiedDocumentsDlg : public QDialog, public Ui_DocumentList
{
public:
  Q_OBJECT

public:
  xiiQtModifiedDocumentsDlg(QWidget* pParent, const xiiHybridArray<xiiDocument*, 32>& modifiedDocs);


private Q_SLOTS:
  void on_ButtonSaveSelected_clicked();
  void on_ButtonDontSave_clicked();
  void SlotSaveDocument();
  void SlotSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

private:
  xiiResult SaveDocument(xiiDocument* pDoc);

  xiiHybridArray<xiiDocument*, 32> m_ModifiedDocs;
};
