#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_EditDynamicEnumsDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class xiiDynamicStringEnum;

class XII_EDITORFRAMEWORK_DLL xiiQtEditDynamicEnumsDlg : public QDialog, public Ui_xiiQtEditDynamicEnumsDlg
{
public:
  Q_OBJECT

public:
  xiiQtEditDynamicEnumsDlg(xiiDynamicStringEnum* pEnum, QWidget* pParent);

  xiiInt32 GetSelectedItem() const { return m_iSelectedItem; }

private Q_SLOTS:
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_Buttons_clicked(QAbstractButton* button);
  void on_EnumValues_itemDoubleClicked(QListWidgetItem* item);

private:
  void FillList();
  bool EditItem(xiiString& item);

  bool                       m_bModified = false;
  xiiDynamicStringEnum*      m_pEnum     = nullptr;
  xiiDynamicArray<xiiString> m_Values;
  xiiInt32                   m_iSelectedItem = -1;
};
