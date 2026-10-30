/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/ui_ShortcutEditorPage.h>
#include <QWidget>

struct xiiActionDescriptor;

class XII_GUIFOUNDATION_DLL xiiQtShortcutEditorPage : public QWidget, public Ui_ShortcutEditor
{
public:
  Q_OBJECT

public:
  xiiQtShortcutEditorPage(QWidget* pParent);
  ~xiiQtShortcutEditorPage();

  void      UpdateTable();
  xiiResult ApplyChanges();

private Q_SLOTS:
  void SlotSelectionChanged();
  void on_KeyEditor_editingFinished();

  void UpdateKeyEdit();

  void on_KeyEditor_keySequenceChanged(const QKeySequence& keySequence);
  void on_ButtonAssign_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonReset_clicked();
  void on_Search_textChanged(const QString& sText);

private:
  xiiHybridArray<xiiString, 32>            m_Shortcuts;
  xiiInt32                                 m_iSelectedAction;
  xiiHybridArray<xiiActionDescriptor*, 32> m_ActionDescs;
};
