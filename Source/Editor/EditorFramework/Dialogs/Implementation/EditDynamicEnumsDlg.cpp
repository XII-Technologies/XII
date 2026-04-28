/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditDynamicEnumsDlg.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

xiiQtEditDynamicEnumsDlg::xiiQtEditDynamicEnumsDlg(xiiDynamicStringEnum* pEnum, QWidget* pParent) :
  QDialog(pParent)
{
  m_pEnum  = pEnum;
  m_Values = m_pEnum->GetAllValidValues();

  setupUi(this);

  FillList();
}

void xiiQtEditDynamicEnumsDlg::FillList()
{
  EnumValues->blockSignals(true);

  EnumValues->clear();

  for (const auto& s : m_Values)
  {
    EnumValues->addItem(s.GetData());
  }

  EnumValues->blockSignals(false);

  // on_EnumValues_itemSelectionChanged();
}

bool xiiQtEditDynamicEnumsDlg::EditItem(xiiString& item)
{
  bool    ok       = false;
  QString newValue = QInputDialog::getText(this, "Edit Value", "Value:", QLineEdit::Normal, item.GetData(), &ok);

  if (!ok || newValue.isEmpty())
    return false;

  m_bModified = true;
  item        = newValue.toUtf8().data();

  return true;
}

void xiiQtEditDynamicEnumsDlg::on_ButtonAdd_clicked()
{
  m_Values.PushBack("New Item");

  if (!EditItem(m_Values.PeekBack()))
  {
    m_Values.PopBack();
    return;
  }

  FillList();

  EnumValues->setCurrentRow((xiiInt32)m_Values.GetCount() - 1);

  // EnumValues->setCurrentIndex(QModelIndex((xiiInt32)m_Values.GetCount() - 1, 0));
}

void xiiQtEditDynamicEnumsDlg::on_ButtonRemove_clicked()
{
  const xiiInt32 idx = EnumValues->currentIndex().row();

  if (idx < 0 || idx >= (xiiInt32)m_Values.GetCount())
    return;

  // if (QMessageBox::question(this, "Remove Item?", QString("Remove item '%1' ?").arg(m_Values[idx].GetData()), QMessageBox::Yes | QMessageBox::No,
  // QMessageBox::Yes) != QMessageBox::Yes)
  //  return;

  m_bModified = true;
  m_Values.RemoveAtAndCopy(idx);

  FillList();
  EnumValues->setCurrentRow(xiiMath::Min(idx, (xiiInt32)m_Values.GetCount() - 1));
}

void xiiQtEditDynamicEnumsDlg::on_Buttons_clicked(QAbstractButton* button)
{
  if (button == Buttons->button(QDialogButtonBox::Cancel))
  {
    reject();
    return;
  }

  if (button == Buttons->button(QDialogButtonBox::Ok))
  {
    if (m_bModified)
    {
      if (QMessageBox::question(this, "Edit Values?", "Modifying these values cannot be undone.\n\nApply changes?",
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
      {
        return;
      }

      m_pEnum->Clear();
      for (const auto& s : m_Values)
      {
        m_pEnum->AddValidValue(s);
      }
      m_pEnum->SortValues();
      m_pEnum->SaveToStorage();

      if (auto* pItem = EnumValues->currentItem())
      {
        const auto& values = m_pEnum->GetAllValidValues();
        m_iSelectedItem    = values.IndexOf(pItem->text().toUtf8().data());
        m_iSelectedItem    = xiiMath::Min(m_iSelectedItem, (xiiInt32)values.GetCount() - 1);
      }
    }

    accept();
    return;
  }
}

void xiiQtEditDynamicEnumsDlg::on_EnumValues_itemDoubleClicked(QListWidgetItem* item)
{
  const xiiInt32 idx = EnumValues->currentIndex().row();

  if (idx < 0 || idx >= (xiiInt32)m_Values.GetCount())
    return;

  if (!EditItem(m_Values[idx]))
    return;

  FillList();
  EnumValues->setCurrentRow(idx);
}
