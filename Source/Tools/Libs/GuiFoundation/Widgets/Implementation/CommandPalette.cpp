/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Widgets/CommandPalette.moc.h>
#include <QAction>
#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>

xiiQtCommandPalette::xiiQtCommandPalette(QWidget* pParent, const QList<QAction*>& actions) :
  QDialog(pParent)
{
  setObjectName(QStringLiteral("EditorCommandPalette"));
  setWindowTitle(tr("Find a command"));
  resize(640, 440);
  auto* pLayout = new QVBoxLayout(this);
  pLayout->setContentsMargins(12, 12, 12, 12);
  pLayout->setSpacing(8);
  m_pSearch = new QLineEdit(this);
  m_pSearch->setPlaceholderText(tr("Search commands..."));
  m_pSearch->setAccessibleName(tr("Search commands"));
  m_pSearch->setClearButtonEnabled(true);
  m_pSearch->installEventFilter(this);
  pLayout->addWidget(m_pSearch);
  m_pResults = new QTreeWidget(this);
  m_pResults->setHeaderLabels({tr("Command"), tr("Shortcut")});
  m_pResults->setRootIsDecorated(false);
  m_pResults->setUniformRowHeights(true);
  m_pResults->setSelectionMode(QAbstractItemView::SingleSelection);
  m_pResults->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_pResults->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_pResults->setAccessibleName(tr("Matching commands"));
  pLayout->addWidget(m_pResults, 1);
  pLayout->addWidget(new QLabel(tr("Enter to run   |   Up / Down to navigate   |   Esc to close"), this));
  for (auto* pAction : actions)
    if (pAction && !pAction->isSeparator())
      m_Actions.push_back(pAction);
  std::sort(m_Actions.begin(), m_Actions.end(), [](const auto& a, const auto& b) { return QString::localeAwareCompare(a->text(), b->text()) < 0; });
  connect(m_pSearch, &QLineEdit::textChanged, this, &xiiQtCommandPalette::UpdateResults);
  connect(m_pSearch, &QLineEdit::returnPressed, this, &xiiQtCommandPalette::ExecuteCurrent);
  connect(m_pResults, &QTreeWidget::itemActivated, this, &xiiQtCommandPalette::ExecuteCurrent);
  for (const auto& pAction : m_Actions)
  {
    connect(pAction, &QAction::changed, this, &xiiQtCommandPalette::UpdateResults);
    connect(pAction, &QObject::destroyed, this, &xiiQtCommandPalette::UpdateResults);
  }
  UpdateResults();
  m_pSearch->setFocus();
}

void xiiQtCommandPalette::UpdateResults()
{
  m_pResults->clear();
  const auto words = m_pSearch->text().simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
  for (qsizetype i = 0; i < m_Actions.size(); ++i)
  {
    const auto& pAction = m_Actions[i];
    if (!pAction || !pAction->isVisible())
      continue;
    const QString sText       = pAction->text().remove(QLatin1Char('&'));
    const QString sSearchable = sText + QLatin1Char(' ') + pAction->toolTip();
    if (!std::all_of(words.begin(), words.end(), [&](const QString& word) { return sSearchable.contains(word, Qt::CaseInsensitive); }))
      continue;
    auto* pItem = new QTreeWidgetItem(m_pResults, {sText, pAction->shortcut().toString(QKeySequence::NativeText)});
    pItem->setIcon(0, pAction->icon());
    pItem->setToolTip(0, pAction->toolTip());
    pItem->setData(0, Qt::UserRole, i);
    if (pAction->isCheckable())
      pItem->setCheckState(0, pAction->isChecked() ? Qt::Checked : Qt::Unchecked);
    pItem->setFlags(pItem->flags() & ~Qt::ItemIsUserCheckable);
    pItem->setDisabled(!pAction->isEnabled());
  }
  for (int i = 0; i < m_pResults->topLevelItemCount(); ++i)
    if (!m_pResults->topLevelItem(i)->isDisabled())
    {
      m_pResults->setCurrentItem(m_pResults->topLevelItem(i));
      break;
    }
  if (m_pResults->topLevelItemCount() == 0)
  {
    auto* pEmpty = new QTreeWidgetItem(m_pResults, {tr("No matching commands")});
    pEmpty->setFlags(Qt::NoItemFlags);
  }
}

void xiiQtCommandPalette::ExecuteCurrent()
{
  auto* pItem = m_pResults->currentItem();
  if (!pItem || pItem->isDisabled() || !pItem->data(0, Qt::UserRole).isValid())
    return;
  const auto pAction = m_Actions[pItem->data(0, Qt::UserRole).toInt()];
  if (!pAction || !pAction->isEnabled() || !pAction->isVisible())
    return;
  accept();
  // Run after the modal dialog has closed, with the originating action as cancellation context.
  QTimer::singleShot(0, pAction, [pAction]() {
    if (pAction && pAction->isEnabled() && pAction->isVisible())
      pAction->trigger();
  });
}

bool xiiQtCommandPalette::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pObject == m_pSearch && pEvent->type() == QEvent::KeyPress)
  {
    auto* pKey = static_cast<QKeyEvent*>(pEvent);
    if (pKey->key() == Qt::Key_Up || pKey->key() == Qt::Key_Down || pKey->key() == Qt::Key_PageUp || pKey->key() == Qt::Key_PageDown)
    {
      QApplication::sendEvent(m_pResults, pEvent);
      return true;
    }
  }
  return QDialog::eventFilter(pObject, pEvent);
}
