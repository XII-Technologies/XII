/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_PickDocumentObjectDlg.h>
#include <QDialog>

class xiiDocumentObject;

class XII_GUIFOUNDATION_DLL xiiQtPickDocumentObjectDlg : public QDialog, public Ui_PickDocumentObjectDlg
{
  Q_OBJECT

public:
  struct Element
  {
    const xiiDocumentObject* m_pObject;
    xiiString                m_sDisplayName;
  };

  xiiQtPickDocumentObjectDlg(QWidget* pParent, const xiiArrayPtr<Element>& objects, const xiiUuid& currentObject);

  /// Stores the result that the user picked
  const xiiDocumentObject* m_pPickedObject = nullptr;

private Q_SLOTS:
  void on_ObjectTree_itemDoubleClicked(QTreeWidgetItem* pItem, int column);

private:
  void UpdateTable();

  xiiArrayPtr<Element> m_Objects;
  xiiUuid              m_CurrentObject;
};
