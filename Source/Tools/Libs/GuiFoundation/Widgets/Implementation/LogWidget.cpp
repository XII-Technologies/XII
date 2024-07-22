#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>
#include <QClipboard>
#include <QKeyEvent>

xiiQtLogWidget::xiiQtLogWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);

  m_pLog = new xiiQtLogModel(this);
  ListViewLog->setModel(m_pLog);
  ListViewLog->setUniformItemSizes(true);
  ListViewLog->installEventFilter(this);
  connect(m_pLog, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex& parent, int iFirst, int iLast) { ScrollToBottomIfAtEnd(iFirst); });

  const int logIndex = ((int)xiiLogMsgType::All - (int)xiiLogMsgType::InfoMsg);
  ComboFilter->setCurrentIndex(logIndex);
}

xiiQtLogWidget::~xiiQtLogWidget() = default;

void xiiQtLogWidget::ShowControls(bool bShow)
{
  ButtonClearLog->setVisible(bShow);
  ComboFilter->setVisible(bShow);
  Search->setVisible(bShow);
}

xiiQtLogModel* xiiQtLogWidget::GetLog()
{
  return m_pLog;
}

xiiQtSearchWidget* xiiQtLogWidget::GetSearchWidget()
{
  return Search;
}

void xiiQtLogWidget::SetLogLevel(xiiLogMsgType::Enum logLevel)
{
  XII_ASSERT_DEBUG(logLevel >= (int)xiiLogMsgType::ErrorMsg && logLevel <= xiiLogMsgType::All, "Invalid log level set.");
  ComboFilter->setCurrentIndex((int)xiiLogMsgType::All - (int)logLevel);
}

xiiLogMsgType::Enum xiiQtLogWidget::GetLogLevel() const
{
  int index = ComboFilter->currentIndex();
  return (xiiLogMsgType::Enum)((int)xiiLogMsgType::All - index);
}

bool xiiQtLogWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pObject == ListViewLog)
  {
    if (pEvent->type() == QEvent::ShortcutOverride)
    {
      // Intercept copy
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
      if (keyEvent->matches(QKeySequence::StandardKey::Copy))
      {
        keyEvent->accept();
        return true;
      }
    }
    else if (pEvent->type() == QEvent::KeyPress)
    {
      // Copy entire selection
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
      if (keyEvent->matches(QKeySequence::StandardKey::Copy))
      {
        QModelIndexList selection = ListViewLog->selectionModel()->selectedRows(0);
        std::sort(selection.begin(), selection.end());

        QStringList     sTemp;
        sTemp.reserve(selection.count());
        for (const QModelIndex& index : selection)
        {
          QString sLine = m_pLog->data(index, Qt::DisplayRole).toString();
          sTemp.push_back(sLine);
        }

        QString sFullText = sTemp.join(QStringLiteral("\n"));
        QApplication::clipboard()->setText(sFullText);
        keyEvent->accept();
        return true;
      }
    }
  }

  return false;
}

void xiiQtLogWidget::ScrollToBottomIfAtEnd(int iNumElements)
{
  if (ListViewLog->selectionModel()->hasSelection())
  {
    if (ListViewLog->selectionModel()->selectedIndexes()[0].row() + 1 >= iNumElements)
    {
      ListViewLog->selectionModel()->clearSelection();
      ListViewLog->scrollToBottom();
    }
  }
  else
    ListViewLog->scrollToBottom();
}

void xiiQtLogWidget::on_ButtonClearLog_clicked()
{
  m_pLog->Clear();
}

void xiiQtLogWidget::on_Search_textChanged(const QString& text)
{
  m_pLog->SetSearchText(text.toUtf8().data());
}

void xiiQtLogWidget::on_ComboFilter_currentIndexChanged(int index)
{
  const xiiLogMsgType::Enum LogLevel = (xiiLogMsgType::Enum)((int)xiiLogMsgType::All - index);
  m_pLog->SetLogLevel(LogLevel);
}
