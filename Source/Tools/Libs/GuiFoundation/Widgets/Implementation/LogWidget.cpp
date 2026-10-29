/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>
#include <QClipboard>
#include <QKeyEvent>
#include <QLabel>
#include <QScrollBar>
#include <QShortcut>
#include <QToolButton>

xiiMap<xiiString, xiiQtLogWidget::LogItemContextActionCallback> xiiQtLogWidget::s_LogCallbacks;

xiiQtLogWidget::xiiQtLogWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);
  Search->SetCaptureTabKey(false);

  m_pLog                   = new xiiQtLogModel(this);
  m_pDiagnostics           = new QWidget(this);
  auto* pDiagnosticsLayout = new QHBoxLayout(m_pDiagnostics);
  pDiagnosticsLayout->setContentsMargins(4, 4, 4, 4);
  pDiagnosticsLayout->setSpacing(4);
  m_pErrors = new QToolButton(m_pDiagnostics);
  m_pErrors->setObjectName(QStringLiteral("LogErrors"));
  m_pErrors->setCheckable(true);
  m_pErrors->setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Flag_Red.svg")));
  m_pErrors->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_pErrors->setToolTip(tr("Show errors; click again to return to information messages"));
  pDiagnosticsLayout->addWidget(m_pErrors);
  m_pWarnings = new QToolButton(m_pDiagnostics);
  m_pWarnings->setObjectName(QStringLiteral("LogWarnings"));
  m_pWarnings->setCheckable(true);
  m_pWarnings->setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Flag_Yellow.svg")));
  m_pWarnings->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_pWarnings->setToolTip(tr("Show warnings and errors"));
  pDiagnosticsLayout->addWidget(m_pWarnings);
  m_pMessageCount = new QLabel(m_pDiagnostics);
  pDiagnosticsLayout->addWidget(m_pMessageCount, 1);
  m_pFollow = new QToolButton(m_pDiagnostics);
  m_pFollow->setObjectName(QStringLiteral("LogFollow"));
  m_pFollow->setText(tr("Follow"));
  m_pFollow->setCheckable(true);
  m_pFollow->setChecked(true);
  m_pFollow->setToolTip(tr("Follow new messages. Scrolling manually pauses following."));
  pDiagnosticsLayout->addWidget(m_pFollow);
  verticalLayout->insertWidget(1, m_pDiagnostics);
  ListViewLog->setModel(m_pLog);
  ListViewLog->setUniformItemSizes(true);
  ListViewLog->installEventFilter(this);
  ListViewLog->viewport()->installEventFilter(this);
  connect(m_pLog, &QAbstractItemModel::rowsInserted, this, &xiiQtLogWidget::FollowNewMessages);
  connect(ListViewLog, &QAbstractItemView::doubleClicked, this, &xiiQtLogWidget::OnItemDoubleClicked);
  connect(m_pErrors, &QToolButton::clicked, this, [this](bool bChecked) { SetLogLevel(bChecked ? xiiLogMsgType::ErrorMsg : xiiLogMsgType::InfoMsg); });
  connect(m_pWarnings, &QToolButton::clicked, this, [this](bool bChecked) { SetLogLevel(bChecked ? xiiLogMsgType::WarningMsg : xiiLogMsgType::InfoMsg); });
  connect(m_pFollow, &QToolButton::toggled, this, [this](bool bFollow) { if (bFollow) ListViewLog->scrollToBottom(); });
  connect(ListViewLog->verticalScrollBar(), &QScrollBar::actionTriggered, this, [this]() { m_pFollow->setChecked(false); });
  connect(m_pLog, &QAbstractItemModel::rowsInserted, this, &xiiQtLogWidget::UpdateDiagnostics);
  connect(m_pLog, &QAbstractItemModel::modelReset, this, &xiiQtLogWidget::UpdateDiagnostics);
  connect(m_pLog, &xiiQtLogModel::NewErrorsOrWarnings, this, &xiiQtLogWidget::UpdateDiagnostics);
  auto* pFind = new QShortcut(QKeySequence::Find, this);
  pFind->setContext(Qt::WidgetWithChildrenShortcut);
  connect(pFind, &QShortcut::activated, Search, [this]() { Search->setFocus(); Search->selectAll(); });

  const int logIndex = ((int)xiiLogMsgType::All - (int)xiiLogMsgType::InfoMsg);
  ComboFilter->setCurrentIndex(logIndex);
  UpdateDiagnostics();
}

xiiQtLogWidget::~xiiQtLogWidget() = default;

void xiiQtLogWidget::ShowControls(bool bShow)
{
  ButtonClearLog->setVisible(bShow);
  ComboFilter->setVisible(bShow);
  Search->setVisible(bShow);
  m_pDiagnostics->setVisible(bShow);
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
  if (pObject == ListViewLog->viewport() && pEvent->type() == QEvent::Wheel)
    m_pFollow->setChecked(false);
  if (pObject == ListViewLog)
  {
    if (pEvent->type() == QEvent::ShortcutOverride)
    {
      // Intercept copy
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
      if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_PageUp || keyEvent->key() == Qt::Key_Home)
        m_pFollow->setChecked(false);
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

        QStringList sTemp;
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

bool xiiQtLogWidget::AddLogItemContextActionCallback(const xiiStringView& sName, const LogItemContextActionCallback& logCallback)
{
  if (sName.IsEmpty())
    return false;

  if (s_LogCallbacks.Contains(sName))
    return false;

  s_LogCallbacks[sName] = logCallback;
  return true;
}

bool xiiQtLogWidget::RemoveLogItemContextActionCallback(const xiiStringView& sName)
{
  if (sName.IsEmpty())
    return false;

  return s_LogCallbacks.Remove(sName);
}

void xiiQtLogWidget::FollowNewMessages()
{
  if (m_pFollow->isChecked())
    ListViewLog->scrollToBottom();
}

void xiiQtLogWidget::UpdateDiagnostics()
{
  m_pErrors->setText(tr("%1 errors").arg(m_pLog->GetNumErrors()));
  m_pWarnings->setText(tr("%1 warnings").arg(m_pLog->GetNumWarnings() + m_pLog->GetNumSeriousWarnings()));
  m_pMessageCount->setText(tr("%1 shown").arg(m_pLog->rowCount()));
  m_pErrors->setChecked(GetLogLevel() == xiiLogMsgType::ErrorMsg);
  m_pWarnings->setChecked(GetLogLevel() == xiiLogMsgType::WarningMsg);
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
  UpdateDiagnostics();
}

void xiiQtLogWidget::OnItemDoubleClicked(QModelIndex idx)
{
  const xiiString sLine(m_pLog->data(idx, Qt::DisplayRole).toString().toUtf8().data());

  for (auto const& callback : s_LogCallbacks)
  {
    callback.Value()(sLine);
  }
}
