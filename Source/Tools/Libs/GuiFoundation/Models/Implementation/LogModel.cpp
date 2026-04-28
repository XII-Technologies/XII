/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <QColor>
#include <QThread>


xiiQtLogModel::xiiQtLogModel(QObject* pParent) :
  QAbstractItemModel(pParent)
{
  m_bIsValid = true;
  m_LogLevel = xiiLogMsgType::InfoMsg;
}

void xiiQtLogModel::Invalidate()
{
  if (!m_bIsValid)
    return;

  beginResetModel();
  m_bIsValid = false;
  endResetModel();
}

void xiiQtLogModel::Clear()
{
  if (m_AllMessages.IsEmpty())
    return;

  {
    XII_LOCK(m_NewMessagesMutex);

    m_uiNumErrors          = 0;
    m_uiNumSeriousWarnings = 0;
    m_uiNumWarnings        = 0;

    m_AllMessages.Clear();
    m_VisibleMessages.Clear();
    m_BlockQueue.Clear();
    Invalidate();
    m_bIsValid = true;
  }

  Q_EMIT NewErrorsOrWarnings(nullptr, false);
}

void xiiQtLogModel::SetLogLevel(xiiLogMsgType::Enum logLevel)
{
  if (m_LogLevel == logLevel)
    return;

  m_LogLevel = logLevel;
  Invalidate();
}

void xiiQtLogModel::SetSearchText(xiiStringView sText)
{
  if (m_sSearchText == sText)
    return;

  m_sSearchText = sText;
  Invalidate();
}

void xiiQtLogModel::AddLogMsg(const xiiLogEntry& msg)
{
  {
    XII_LOCK(m_NewMessagesMutex);
    m_NewMessages.PushBack(msg);
  }

  // Always queue the message processing, otherwise it can happen that an error during this
  // triggers recursive logging, which is forbidden
  QMetaObject::invokeMethod(this, "ProcessNewMessages", Qt::ConnectionType::QueuedConnection);

  return;
}

bool xiiQtLogModel::IsFiltered(const xiiLogEntry& lm) const
{
  if (lm.m_Type < xiiLogMsgType::None)
    return false;

  if (lm.m_Type > m_LogLevel)
    return true;

  if (m_sSearchText.IsEmpty())
    return false;

  if (lm.m_sMsg.FindSubString_NoCase(m_sSearchText.GetData()))
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////
// xiiQtLogModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant xiiQtLogModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  UpdateVisibleEntries();

  const xiiInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (xiiInt32)m_VisibleMessages.GetCount())
    return QVariant();

  const xiiLogEntry& msg = *m_VisibleMessages[iRow];

  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      if (msg.m_sMsg.FindSubString("\n") != nullptr)
      {
        xiiStringBuilder sTemp = msg.m_sMsg;
        sTemp.ReplaceAll("\n", " ");
        return xiiMakeQString(sTemp);
      }
      return xiiMakeQString(msg.m_sMsg);
    }
    case Qt::ToolTipRole:
    {
      return xiiMakeQString(msg.m_sMsg);
    }
    case Qt::ForegroundRole:
    {
      switch (msg.m_Type)
      {
        case xiiLogMsgType::BeginGroup:
          return xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Gray));
        case xiiLogMsgType::EndGroup:
          return xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Gray));
        case xiiLogMsgType::ErrorMsg:
          return xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Red));
        case xiiLogMsgType::SeriousWarningMsg:
          return xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Orange));
        case xiiLogMsgType::WarningMsg:
          return xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Yellow));
        case xiiLogMsgType::SuccessMsg:
          return xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Green));
        case xiiLogMsgType::DevMsg:
          return xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Blue));
        case xiiLogMsgType::DebugMsg:
          return xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Cyan));
        default:
          return QVariant();
      }
    }

    default:
      return QVariant();
  }
}

Qt::ItemFlags xiiQtLogModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant xiiQtLogModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  return QVariant();
}

QModelIndex xiiQtLogModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn, iRow);
}

QModelIndex xiiQtLogModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int xiiQtLogModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  UpdateVisibleEntries();

  return (int)m_VisibleMessages.GetCount();
}

int xiiQtLogModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}


void xiiQtLogModel::ProcessNewMessages()
{
  bool             bNewErrors = false;
  xiiStringBuilder sLatestWarning;
  xiiStringBuilder sLatestError;

  {
    XII_LOCK(m_NewMessagesMutex);
    xiiStringBuilder s;
    for (const auto& msg : m_NewMessages)
    {
      m_AllMessages.PushBack(msg);

      if (msg.m_Type == xiiLogMsgType::BeginGroup || msg.m_Type == xiiLogMsgType::EndGroup)
      {
        s.SetPrintf("%*s<<< %s", msg.m_uiIndentation, "", msg.m_sMsg.GetData());

        if (msg.m_Type == xiiLogMsgType::EndGroup)
        {
          s.AppendFormat(" ({0} sec) >>>", xiiArgF(msg.m_fSeconds, 3));
        }
        else if (!msg.m_sTag.IsEmpty())
        {
          s.Append(" (", msg.m_sTag, ") >>>");
        }
        else
        {
          s.Append(" >>>");
        }

        m_AllMessages.PeekBack().m_sMsg = s;
      }
      else
      {
        s.SetPrintf("%*s%s", 4 * msg.m_uiIndentation, "", msg.m_sMsg.GetData());
        m_AllMessages.PeekBack().m_sMsg = s;

        if (msg.m_Type == xiiLogMsgType::ErrorMsg)
        {
          sLatestError = msg.m_sMsg;
          bNewErrors   = true;
          ++m_uiNumErrors;
        }
        else if (msg.m_Type == xiiLogMsgType::SeriousWarningMsg)
        {
          sLatestWarning = msg.m_sMsg;
          bNewErrors     = true;
          ++m_uiNumSeriousWarnings;
        }
        else if (msg.m_Type == xiiLogMsgType::WarningMsg)
        {
          sLatestWarning = msg.m_sMsg;
          bNewErrors     = true;
          ++m_uiNumWarnings;
        }
      }


      // if the message would not be shown anyway, don't trigger an update
      if (IsFiltered(msg))
        continue;

      if (msg.m_Type == xiiLogMsgType::BeginGroup)
      {
        m_BlockQueue.PushBack(&m_AllMessages.PeekBack());
        continue;
      }
      else if (msg.m_Type == xiiLogMsgType::EndGroup)
      {
        if (!m_BlockQueue.IsEmpty())
        {
          m_BlockQueue.PopBack();
          continue;
        }
      }

      for (auto pMsg : m_BlockQueue)
      {
        beginInsertRows(QModelIndex(), m_VisibleMessages.GetCount(), m_VisibleMessages.GetCount());
        m_VisibleMessages.PushBack(pMsg);
        endInsertRows();
      }

      m_BlockQueue.Clear();

      beginInsertRows(QModelIndex(), m_VisibleMessages.GetCount(), m_VisibleMessages.GetCount());
      m_VisibleMessages.PushBack(&m_AllMessages.PeekBack());
      endInsertRows();
    }

    m_NewMessages.Clear();
  }

  if (bNewErrors)
  {
    if (!sLatestError.IsEmpty())
    {
      Q_EMIT NewErrorsOrWarnings(sLatestError, true);
    }
    else
    {
      Q_EMIT NewErrorsOrWarnings(sLatestWarning, false);
    }
  }
}

void xiiQtLogModel::UpdateVisibleEntries() const
{
  if (m_bIsValid)
    return;


  m_bIsValid = true;
  m_VisibleMessages.Clear();
  for (const auto& msg : m_AllMessages)
  {
    if (IsFiltered(msg))
      continue;

    if (msg.m_Type == xiiLogMsgType::EndGroup)
    {
      if (!m_VisibleMessages.IsEmpty())
      {
        if (m_VisibleMessages.PeekBack()->m_Type == xiiLogMsgType::BeginGroup)
          m_VisibleMessages.PopBack();
        else
          m_VisibleMessages.PushBack(&msg);
      }
    }
    else
    {
      m_VisibleMessages.PushBack(&msg);
    }
  }
}
