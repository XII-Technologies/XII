/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Dialogs/ShortcutEditorPage.moc.h>
#include <QKeySequenceEdit>
#include <QTableWidget>
#include <QTreeWidget>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

xiiQtShortcutEditorPage::xiiQtShortcutEditorPage(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);

  XII_VERIFY(connect(Shortcuts, SIGNAL(itemSelectionChanged()), this, SLOT(SlotSelectionChanged())) != nullptr, "signal/slot connection failed");

  m_iSelectedAction = -1;
  KeyEditor->setEnabled(false);

  xiiMap<xiiString, xiiMap<xiiString, xiiInt32>> SortedItems;

  {
    auto itActions = xiiActionManager::GetActionIterator();

    while (itActions.IsValid())
    {
      if (itActions.Value()->m_Type == xiiActionType::Action)
      {
        SortedItems[itActions.Value()->m_sCategoryPath][itActions.Value()->m_sActionName] = m_ActionDescs.GetCount();
        m_ActionDescs.PushBack(itActions.Value());
        m_Shortcuts.PushBack(itActions.Value()->m_sShortcut);
      }

      itActions.Next();
    }
  }

  {
    xiiQtScopedBlockSignals    bs(Shortcuts);
    xiiQtScopedUpdatesDisabled ud(Shortcuts);

    Shortcuts->setAlternatingRowColors(true);
    Shortcuts->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
    Shortcuts->setExpandsOnDoubleClick(true);

    xiiStringBuilder sTemp;

    for (auto it = SortedItems.GetIterator(); it.IsValid(); ++it)
    {
      auto pParent = new QTreeWidgetItem();
      pParent->setData(0, Qt::DisplayRole, it.Key().GetData());
      Shortcuts->addTopLevelItem(pParent);

      pParent->setExpanded(true);
      pParent->setFirstColumnSpanned(true);
      pParent->setFlags(Qt::ItemFlag::ItemIsEnabled);

      QFont font = pParent->font(0);
      font.setBold(true);

      pParent->setFont(0, font);

      for (auto it2 : it.Value())
      {
        const auto& item  = m_ActionDescs[it2.Value()];
        auto        pItem = new QTreeWidgetItem(pParent);

        /// \todo Instead of removing &, replace it by underlined text (requires formatted text output)
        sTemp = xiiTranslate(item->m_sActionName);
        sTemp.ReplaceAll("&", "");

        pItem->setData(0, Qt::UserRole, it2.Value());
        pItem->setData(0, Qt::DisplayRole, item->m_sActionName.GetData());
        pItem->setData(1, Qt::DisplayRole, sTemp.GetData());
        pItem->setData(2, Qt::DisplayRole, item->m_sShortcut.GetData());
        pItem->setData(3, Qt::DisplayRole, xiiMakeQString(xiiTranslateTooltip(item->m_sActionName)));

        if (item->m_sShortcut == item->m_sDefaultShortcut)
          pItem->setBackground(2, QBrush());
        else
          pItem->setBackground(2, Qt::darkYellow);

        sTemp.Set("Default: ", item->m_sDefaultShortcut.IsEmpty() ? "<none>" : item->m_sDefaultShortcut.GetData());

        pItem->setToolTip(2, xiiMakeQString(sTemp.GetView()));
      }
    }

    Shortcuts->resizeColumnToContents(0);
    Shortcuts->resizeColumnToContents(2);
  }

  ButtonAssign->setEnabled(false);
  ButtonRemove->setEnabled(false);
  ButtonReset->setEnabled(false);
}

xiiQtShortcutEditorPage::~xiiQtShortcutEditorPage() = default;

xiiResult xiiQtShortcutEditorPage::ApplyChanges()
{
  xiiMap<xiiString, xiiString> assignments;
  for (xiiUInt32 i = 0; i < m_ActionDescs.GetCount(); ++i)
    if (m_ActionDescs[i]->m_sShortcut != m_Shortcuts[i])
      assignments.Insert(m_ActionDescs[i]->m_sActionName, m_Shortcuts[i]);
  if (assignments.IsEmpty())
    return XII_SUCCESS;
  XII_SUCCEED_OR_RETURN(xiiActionManager::SaveShortcutAssignment(assignments));
  for (xiiUInt32 i = 0; i < m_ActionDescs.GetCount(); ++i)
  {
    if (m_ActionDescs[i]->m_sShortcut == m_Shortcuts[i])
      continue;
    m_ActionDescs[i]->m_sShortcut = m_Shortcuts[i];
    m_ActionDescs[i]->UpdateExistingActions();
  }
  return XII_SUCCESS;
}

void xiiQtShortcutEditorPage::UpdateTable()
{
  for (xiiInt32 iTop = 0; iTop < Shortcuts->topLevelItemCount(); ++iTop)
  {
    auto pTopItem = Shortcuts->topLevelItem(iTop);

    for (xiiInt32 iChild = 0; iChild < pTopItem->childCount(); ++iChild)
    {
      auto pChild = pTopItem->child(iChild);

      xiiInt32 idx = pChild->data(0, Qt::UserRole).toInt();

      const auto& item = m_ActionDescs[idx];

      pChild->setData(2, Qt::DisplayRole, QVariant(m_Shortcuts[idx].GetData()));

      if (m_Shortcuts[idx] == item->m_sDefaultShortcut)
        pChild->setBackground(2, QBrush());
      else
        pChild->setBackground(2, Qt::darkYellow);
    }
  }

  if (m_iSelectedAction >= 0)
  {
    ButtonRemove->setEnabled(!m_Shortcuts[m_iSelectedAction].IsEmpty());
    ButtonReset->setEnabled(m_Shortcuts[m_iSelectedAction] != m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut);
  }

  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  ButtonAssign->setEnabled(!sText.isEmpty());
}

void xiiQtShortcutEditorPage::SlotSelectionChanged()
{
  auto selection = Shortcuts->selectedItems();
  if (selection.size() == 1)
  {
    m_iSelectedAction = selection[0]->data(0, Qt::UserRole).toInt();
    KeyEditor->clear();
    KeyEditor->setEnabled(true);
    ButtonAssign->setEnabled(false);
    ButtonRemove->setEnabled(!m_Shortcuts[m_iSelectedAction].IsEmpty());
    ButtonReset->setEnabled(m_Shortcuts[m_iSelectedAction] != m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut);
  }
  else
  {
    m_iSelectedAction = -1;
    KeyEditor->clear();
    KeyEditor->setEnabled(false);
    ButtonAssign->setEnabled(false);
    ButtonRemove->setEnabled(false);
    ButtonReset->setEnabled(false);
  }
}

void xiiQtShortcutEditorPage::on_KeyEditor_editingFinished()
{
  UpdateKeyEdit();
}

void xiiQtShortcutEditorPage::UpdateKeyEdit()
{
  if (m_iSelectedAction < 0)
    return;

  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  ButtonAssign->setEnabled(!sText.isEmpty());
}

void xiiQtShortcutEditorPage::on_KeyEditor_keySequenceChanged(const QKeySequence& keySequence)
{
  UpdateKeyEdit();
}

void xiiQtShortcutEditorPage::on_ButtonAssign_clicked()
{
  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  KeyEditor->clear();

  m_Shortcuts[m_iSelectedAction] = sText.toUtf8().data();

  UpdateTable();
}

void xiiQtShortcutEditorPage::on_ButtonRemove_clicked()
{
  KeyEditor->clear();

  m_Shortcuts[m_iSelectedAction].Clear();

  UpdateTable();
}

void xiiQtShortcutEditorPage::on_ButtonReset_clicked()
{
  KeyEditor->clear();

  m_Shortcuts[m_iSelectedAction] = m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut;

  UpdateTable();
}

void xiiQtShortcutEditorPage::on_Search_textChanged(const QString& sText)
{
  xiiSearchPatternFilter filter;
  filter.SetSearchText(sText.toUtf8().data());

  xiiQtScopedUpdatesDisabled ud(Shortcuts);

  for (xiiInt32 iTop = 0; iTop < Shortcuts->topLevelItemCount(); ++iTop)
  {
    auto pTopItem                 = Shortcuts->topLevelItem(iTop);
    bool bAnyCategoryChildVisible = false;

    for (xiiInt32 iChild = 0; iChild < pTopItem->childCount(); ++iChild)
    {
      auto            pChild      = pTopItem->child(iChild);
      const xiiString sActionName = pChild->data(0, Qt::DisplayRole).toString().toUtf8().data();
      const xiiString sShortcut   = pChild->data(2, Qt::DisplayRole).toString().toUtf8().data();

      const bool bVisible = filter.PassesFilters(sActionName) || filter.PassesFilters(sShortcut);
      pChild->setHidden(!bVisible);

      if (bVisible)
      {
        bAnyCategoryChildVisible = true;
      }
    }

    pTopItem->setHidden(!bAnyCategoryChildVisible);
  }
}
